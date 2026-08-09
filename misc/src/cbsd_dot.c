// CBSD Project
// See notes in daemon.c for detail
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <err.h>
#include <getopt.h>

#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <sysexits.h>
#include <limits.h>
#include <signal.h>
#include <time.h>

#define FALSE 0
#define TRUE 1

/* List of all nodesql */
enum {
	C_FILE,
	C_TIMEOUT,
	C_STDOUT_TIMEOUT,
};

static int g_kq_out = -1;
static int g_kq_out_fd = -1;

static int
parse_nonneg_int(const char *s, int *out)
{
	char *end = NULL;
	long v;

	if (s == NULL || *s == '\0')
		return (-1);

	errno = 0;
	v = strtol(s, &end, 10);
	if (errno != 0 || end == s || *end != '\0')
		return (-1);
	if (v < 0 || v > INT_MAX)
		return (-1);

	*out = (int)v;
	return (0);
}

static int64_t
now_ms_monotonic(void)
{
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		return (-1);
	return ((int64_t)ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

static void
fatal_stdout_timeout(void)
{
	/*
	 * The requirement is to terminate if stdout can't be written
	 * within a timeout (e.g., sshd is not reading due to network stall).
	 * Use _exit to avoid stdio flush deadlocks.
	 */
	_exit(EX_IOERR);
}

static void
fatal_stdout_broken(void)
{
	_exit(EX_IOERR);
}

static void
stdout_write_init_or_die(int fd)
{
	struct kevent kev;

	if (g_kq_out != -1)
		return;

	g_kq_out = kqueue();
	if (g_kq_out == -1)
		fatal_stdout_broken();

	g_kq_out_fd = fd;
	EV_SET(&kev, fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, 0);
	if (kevent(g_kq_out, &kev, 1, NULL, 0, NULL) == -1)
		fatal_stdout_broken();
}

static void
timed_write_or_die(int fd, const char *buf, size_t len, int timeout_ms)
{
	int flags;
	size_t off = 0;
	int64_t deadline_ms = -1;
	struct kevent kev;

	if (timeout_ms > 0) {
		int64_t now = now_ms_monotonic();
		if (now < 0)
			fatal_stdout_timeout();
		deadline_ms = now + timeout_ms;
	}

	flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		fatal_stdout_broken();
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		fatal_stdout_broken();

	stdout_write_init_or_die(fd);
	if (g_kq_out_fd != fd)
		fatal_stdout_broken();

	while (off < len) {
		ssize_t n = write(fd, buf + off, len - off);
		if (n > 0) {
			off += (size_t)n;
			continue;
		}
		if (n == -1 && (errno == EINTR))
			continue;
		if (n == -1 && (errno == EPIPE || errno == ECONNRESET))
			fatal_stdout_broken();
		if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			struct timespec ts;
			struct timespec *tsp = NULL;

			if (timeout_ms > 0) {
				int64_t now = now_ms_monotonic();
				int64_t rem_ms;
				if (now < 0)
					fatal_stdout_timeout();
				rem_ms = deadline_ms - now;
				if (rem_ms <= 0)
					fatal_stdout_timeout();
				ts.tv_sec = rem_ms / 1000;
				ts.tv_nsec = (rem_ms % 1000) * 1000000;
				tsp = &ts;
			}

			memset(&kev, 0, sizeof(kev));
			int kr = kevent(g_kq_out, NULL, 0, &kev, 1, tsp);
			if (kr == 0)
				fatal_stdout_timeout();
			if (kr < 0) {
				if (errno == EINTR)
					continue;
				fatal_stdout_broken();
			}
			if (kev.flags & EV_ERROR)
				fatal_stdout_broken();
			continue;
		}
		fatal_stdout_broken();
	}

	(void)fcntl(fd, F_SETFL, flags);
}

int
cbsd_fwatch_usage(void)
{
	printf(
	    "Wait for local.sqlite modification and print this fact to output. Also print dot by timeout\n");
	printf("require: --file, --timeout\n");
	printf(
	    "usage: cbsd_dot --file=path_to_file --timeout=0 (in seconds, 0 is infinity)\n");
	printf(
	    "optional: --stdout-timeout=N (milliseconds, default 30000, 0 is infinity)\n");
	return (EX_USAGE);
}

int
main(int argc, char *argv[])
{
	int fd;
	int kq;
	int nev;
	struct kevent ev;
	//	static const struct timespec tout = { 1, 0 };

	int optcode = 0;
	int option_index = 0;
	struct timespec tv;
	char *watchfile = NULL;
	int timeout = 60;
	int stdout_timeout_ms = 30000;
	char cmd[256];

	struct option long_options[] = { { "file", required_argument, 0,
					     C_FILE },
		{ "timeout", required_argument, 0, C_TIMEOUT },
		{ "stdout-timeout", required_argument, 0, C_STDOUT_TIMEOUT },
		/* End of options marker */
		{ 0, 0, 0, 0 } };

	/* Prefer controlled handling instead of SIGPIPE termination. */
	(void)signal(SIGPIPE, SIG_IGN);
	stdout_write_init_or_die(STDOUT_FILENO);

	if (argc < 2) {
		cbsd_fwatch_usage();
		return 0;
	}

	while (TRUE) {
		optcode = getopt_long(argc, argv, "", long_options,
		    &option_index);
		if (optcode == -1) {
			break;
		}
		switch (optcode) {
		case C_FILE:
			free(watchfile);
			watchfile = strdup(optarg);
			if (watchfile == NULL)
				err(EX_OSERR, "strdup");
			break;
		case C_TIMEOUT:
			if (parse_nonneg_int(optarg, &timeout) != 0)
				errx(EX_USAGE, "invalid --timeout value");
			break;
		case C_STDOUT_TIMEOUT:
			if (parse_nonneg_int(optarg, &stdout_timeout_ms) != 0)
				errx(EX_USAGE, "invalid --stdout-timeout value");
			break;
		}
	} // while

	// zero for getopt *variables for next execute
	optarg = NULL;
	optind = 0;
	optopt = 0;
	opterr = 0;
	optreset = 0;

	if (!watchfile) {
		cbsd_fwatch_usage();
		return 1;
	}

	if ((fd = open(watchfile, O_RDONLY)) == -1) {
		printf("Cannot open: %s\n", watchfile);
		exit(1);
	}

	if ((kq = kqueue()) == -1) {
		printf("Cannot create kqueue\n");
		close(fd);
		free(watchfile);
		exit(1);
	}

	EV_SET(&ev, fd, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR,
	    NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB | NOTE_LINK |
		NOTE_RENAME | NOTE_REVOKE,
	    0, 0);

	if (kevent(kq, &ev, 1, NULL, 0, NULL) == -1) {
		printf("kevent\n");
		close(fd);
		close(kq);
		free(watchfile);
		exit(1);
	}

	tv.tv_sec = timeout;
	tv.tv_nsec = 0;

	for (;;) {
		memset(cmd, 0, sizeof(cmd));

		if (timeout == 0) {
			nev = kevent(kq, NULL, 0, &ev, 1, NULL);
		} else {
			nev = kevent(kq, NULL, 0, &ev, 1, &tv);
		}

		if (nev == -1) {
			printf("kevent\n");
			exit(1);
		}

		if (nev == 0) {
			// timeout
			timed_write_or_die(STDOUT_FILENO, ".\r\n", 3,
			    stdout_timeout_ms);
			// continue;
		} else {
			size_t used = 0;
			if (ev.fflags & NOTE_DELETE)
				used += (size_t)snprintf(cmd + used,
				    sizeof(cmd) - used, "deleted ");
			if (ev.fflags & NOTE_WRITE)
				used += (size_t)snprintf(cmd + used,
				    sizeof(cmd) - used, "written ");
			if (ev.fflags & NOTE_EXTEND)
				used += (size_t)snprintf(cmd + used,
				    sizeof(cmd) - used, "extended ");
			if (ev.fflags & NOTE_ATTRIB)
				used += (size_t)snprintf(cmd + used,
				    sizeof(cmd) - used, "chmod/chown/utimes ");
			if (ev.fflags & NOTE_LINK)
				used += (size_t)snprintf(cmd + used,
				    sizeof(cmd) - used, "hardlinked ");
			if (ev.fflags & NOTE_RENAME)
				used += (size_t)snprintf(cmd + used,
				    sizeof(cmd) - used, "renamed ");
			if (ev.fflags & NOTE_REVOKE)
				used += (size_t)snprintf(cmd + used,
				    sizeof(cmd) - used, "revoked ");

			if (used == 0)
				used = (size_t)snprintf(cmd, sizeof(cmd), "changed ");

			/* Add CRLF as before. */
			if (used + 2 < sizeof(cmd)) {
				cmd[used++] = '\r';
				cmd[used++] = '\n';
				cmd[used] = '\0';
			} else {
				/* Fallback: ensure termination. */
				cmd[sizeof(cmd) - 3] = '\r';
				cmd[sizeof(cmd) - 2] = '\n';
				cmd[sizeof(cmd) - 1] = '\0';
				used = sizeof(cmd) - 1;
			}

			timed_write_or_die(STDOUT_FILENO, cmd, used,
			    stdout_timeout_ms);
		}
	}
}
