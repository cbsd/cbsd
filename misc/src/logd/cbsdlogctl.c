/* cbsdlogctl — control tool for cbsdlogd. See LOGGER.md.
 *   enable <jail> <sockdir>
 *   disable <jail>
 *   create <jail>
 *   destroy <jail>
 *   exec <jail> -- <command> [args...]
 *   console [-C|--color auto|yes|no] [-f|--follow] [-t|--timeout seconds]
 *           [-W|--wait seconds] <jail>
 */

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "cbsdlog.h"

static const char *ctl_path;
static const char *pidfile;
static const char *snipdir;
static const char *logdir;
static char ctl_buf[PATH_MAX];
static char pid_buf[PATH_MAX];
static char snip_buf[PATH_MAX];
static char log_buf[PATH_MAX];

static int
write_all(int fd, const char *buf, size_t len)
{
	size_t off = 0;

	while (off < len) {
		ssize_t n = write(fd, buf + off, len - off);

		if (n < 0) {
			if (errno == EINTR)
				continue;
			return (-1);
		}
		if (n == 0)
			return (-1);
		off += (size_t)n;
	}
	return (0);
}

static int
read_line(int fd, char *buf, size_t buflen)
{
	size_t i = 0;

	while (i + 1 < buflen) {
		char c;
		ssize_t n = read(fd, &c, 1);

		if (n == 0)
			break;
		if (n < 0) {
			if (errno == EINTR)
				continue;
			return (-1);
		}
		buf[i++] = c;
		if (c == '\n')
			break;
	}
	buf[i] = '\0';
	return ((int)i);
}

static int
connect_ctl(void)
{
	struct sockaddr_un un;
	int fd;

	if (cbsdlog_set_un(&un, ctl_path) < 0)
		errx(1, "control socket path is too long");
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
		err(1, "socket");
	if (connect(fd, (struct sockaddr *)&un, cbsdlog_un_len(&un)) < 0)
		err(1, "connect %s", ctl_path);
	return (fd);
}

/* Send one command line. Returns 0 on OK. */
static int
call_cmd(const char *line)
{
	char buf[512];
	int fd;
	int n;

	fd = connect_ctl();
	if (write_all(fd, line, strlen(line)) < 0)
		err(1, "write");
	n = read_line(fd, buf, sizeof(buf));
	close(fd);
	if (n < 0)
		err(1, "read");
	if (n == 0)
		errx(1, "daemon closed the connection");
	if (strncmp(buf, "OK", 2) == 0)
		return (0);
	fprintf(stderr, "%s", buf);
	if (buf[strlen(buf) - 1] != '\n')
		fputc('\n', stderr);
	return (-1);
}

static int
ensure_dir(const char *path)
{
	struct stat st;

	if (stat(path, &st) == 0) {
		if (S_ISDIR(st.st_mode))
			return (0);
		errno = ENOTDIR;
		return (-1);
	}
	if (mkdir(path, 0755) < 0)
		return (-1);
	return (0);
}

static int
jail_conf_path(const char *jail, char *path, size_t pathlen)
{
	int n;

	n = snprintf(path, pathlen, "%s/%s.conf", snipdir, jail);
	if (n < 0 || (size_t)n >= pathlen)
		return (-1);
	return (0);
}

static int
log_path(const char *jail, char *path, size_t pathlen)
{
	int n;

	n = snprintf(path, pathlen, "%s/%s.log", logdir, jail);
	if (n < 0 || (size_t)n >= pathlen)
		return (-1);
	return (0);
}

static void
write_jail_conf(const char *jail, const char *logfile)
{
	char path[PATH_MAX];
	struct stat st;
	FILE *fp;

	if (jail_conf_path(jail, path, sizeof(path)) < 0)
		errx(1, "config path is too long");
	if (stat(path, &st) == 0)
		return;
	if (errno != ENOENT)
		err(1, "stat %s", path);
	fp = fopen(path, "w");
	if (fp == NULL)
		err(1, "fopen %s", path);
	if (fprintf(fp, "+%s\n*.*\t%s\n", jail, logfile) < 0) {
		fclose(fp);
		err(1, "write %s", path);
	}
	if (fclose(fp) != 0)
		err(1, "fclose %s", path);
}

static void
touch_log(const char *path)
{
	int fd;

	fd = open(path, O_CREAT | O_WRONLY, 0640);
	if (fd < 0)
		err(1, "open %s", path);
	close(fd);
}

static void
hup_syslogd(void)
{
	FILE *fp;
	long pid;

	fp = fopen(pidfile, "r");
	if (fp == NULL) {
		warnx("no %s; dedicated syslogd was not signalled", pidfile);
		return;
	}
	if (fscanf(fp, "%ld", &pid) != 1 || pid <= 0) {
		fclose(fp);
		errx(1, "bad pid file %s", pidfile);
	}
	fclose(fp);
	if (kill((pid_t)pid, SIGHUP) < 0) {
		if (errno == ESRCH) {
			warnx("syslogd pid %ld is not running", pid);
			return;
		}
		err(1, "kill %ld", pid);
	}
}

static int
cmd_enable(const char *jail, const char *sockdir)
{
	char path[PATH_MAX];
	char line[PATH_MAX + CBSDLOG_NAME_MAX];

	if (!cbsdlog_valid_jail(jail))
		errx(1, "bad jail name");
	if (sockdir[0] != '/')
		errx(1, "sockdir must be an absolute path");
	if (ensure_dir(sockdir) < 0)
		err(1, "mkdir %s", sockdir);

	if (snprintf(path, sizeof(path), "%s/log", sockdir) >= (int)sizeof(path))
		errx(1, "socket path is too long");
	if (snprintf(line, sizeof(line), "OPEN %s %s 666\n", jail, path) >=
	    (int)sizeof(line))
		errx(1, "command is too long");
	if (call_cmd(line) < 0)
		return (1);

	if (snprintf(path, sizeof(path), "%s/logpriv", sockdir) >= (int)sizeof(path))
		errx(1, "socket path is too long");
	if (snprintf(line, sizeof(line), "OPEN %s %s 600\n", jail, path) >=
	    (int)sizeof(line))
		errx(1, "command is too long");
	if (call_cmd(line) < 0)
		return (1);
	return (0);
}

static int
cmd_create(const char *jail)
{
	char logfile[PATH_MAX];

	if (!cbsdlog_valid_jail(jail))
		errx(1, "bad jail name");
	if (ensure_dir(logdir) < 0)
		err(1, "mkdir %s", logdir);
	if (ensure_dir(snipdir) < 0)
		err(1, "mkdir %s", snipdir);
	if (log_path(jail, logfile, sizeof(logfile)) < 0)
		errx(1, "log path is too long");
	write_jail_conf(jail, logfile);
	touch_log(logfile);
	hup_syslogd();
	return (0);
}

static int
cmd_destroy(const char *jail)
{
	char path[PATH_MAX];

	if (!cbsdlog_valid_jail(jail))
		errx(1, "bad jail name");
	if (jail_conf_path(jail, path, sizeof(path)) < 0)
		errx(1, "config path is too long");
	if (unlink(path) < 0 && errno != ENOENT)
		err(1, "unlink %s", path);
	hup_syslogd();
	return (0);
}

static int
cmd_disable(const char *jail)
{
	char line[CBSDLOG_NAME_MAX + 16];

	if (!cbsdlog_valid_jail(jail))
		errx(1, "bad jail name");
	snprintf(line, sizeof(line), "CLOSE %s\n", jail);
	return (call_cmd(line) < 0 ? 1 : 0);
}

static int
cmd_exec(const char *jail, char **argv)
{
	struct sockaddr_un un;
	struct msghdr msg;
	struct iovec iov;
	struct cmsghdr *cmsg;
	char cmd[CBSDLOG_NAME_MAX + 32];
	char cbuf[CMSG_SPACE(sizeof(int) * 2)];
	char reply[512];
	int outp[2], errp[2];
	int fd;
	int *fds;

	if (!cbsdlog_valid_jail(jail))
		errx(1, "bad jail name");
	if (argv[0] == NULL)
		errx(1, "exec needs a command");
	if (pipe(outp) < 0 || pipe(errp) < 0)
		err(1, "pipe");

	if (cbsdlog_set_un(&un, ctl_path) < 0)
		errx(1, "control socket path is too long");
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
		err(1, "socket");
	if (connect(fd, (struct sockaddr *)&un, cbsdlog_un_len(&un)) < 0)
		err(1, "connect %s", ctl_path);

	snprintf(cmd, sizeof(cmd), "CONSOLE %s\n", jail);
	memset(&msg, 0, sizeof(msg));
	memset(cbuf, 0, sizeof(cbuf));
	iov.iov_base = cmd;
	iov.iov_len = strlen(cmd);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = cbuf;
	msg.msg_controllen = sizeof(cbuf);
	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(int) * 2);
	fds = (int *)CMSG_DATA(cmsg);
	fds[0] = outp[0];
	fds[1] = errp[0];

	if (sendmsg(fd, &msg, 0) < 0)
		err(1, "sendmsg");
	if (read_line(fd, reply, sizeof(reply)) < 0)
		err(1, "read");
	if (strncmp(reply, "OK", 2) != 0) {
		fprintf(stderr, "%s", reply);
		if (reply[0] != '\0' && reply[strlen(reply) - 1] != '\n')
			fputc('\n', stderr);
		exit(1);
	}

	close(outp[0]);
	close(errp[0]);
	close(fd);
	if (dup2(outp[1], STDOUT_FILENO) < 0 || dup2(errp[1], STDERR_FILENO) < 0)
		err(1, "dup2");
	if (outp[1] != STDOUT_FILENO)
		close(outp[1]);
	if (errp[1] != STDERR_FILENO)
		close(errp[1]);
	execvp(argv[0], argv);
	err(127, "exec %s", argv[0]);
}

#define COLOR_AUTO	0
#define COLOR_YES	1
#define COLOR_NO	2

static const char col_green[] = "\033[0;32m";
static const char col_red[] = "\033[0;31m";
static const char col_reset[] = "\033[0m";

/* A record is <tag><text>\n. The tag selects stdout or stderr and is not written. */
static int
console_write(const char *buf, size_t n, int *in_line, int *to_err, int *painted,
    int color_out, int color_err)
{
	size_t i = 0;

	while (i < n) {
		const char *nl;
		const char *color;
		int fd;
		size_t chunk;

		if (!*in_line) {
			*to_err = ((unsigned char)buf[i] == (unsigned char)CBSDLOG_CONS_ERR);
			*in_line = 1;
			*painted = 0;
			i++;
			fd = *to_err ? STDERR_FILENO : STDOUT_FILENO;
			if (*to_err ? color_err : color_out) {
				color = *to_err ? col_red : col_green;
				if (write_all(fd, color, strlen(color)) < 0)
					return (-1);
				*painted = 1;
			}
			continue;
		}
		nl = memchr(buf + i, '\n', n - i);
		fd = *to_err ? STDERR_FILENO : STDOUT_FILENO;
		if (nl == NULL) {
			if (n > i && write_all(fd, buf + i, n - i) < 0)
				return (-1);
			return (0);
		}
		chunk = (size_t)(nl - (buf + i));
		if (chunk > 0 && write_all(fd, buf + i, chunk) < 0)
			return (-1);
		if (write_all(fd, "\n", 1) < 0)
			return (-1);
		if (*painted && write_all(fd, col_reset, strlen(col_reset)) < 0)
			return (-1);
		i += chunk + 1;
		*in_line = 0;
		*painted = 0;
	}
	return (0);
}

static long
mono_left_ms(const struct timespec *deadline)
{
	struct timespec now;
	long left;

	if (clock_gettime(CLOCK_MONOTONIC, &now) < 0)
		err(1, "clock_gettime");
	left = (deadline->tv_sec - now.tv_sec) * 1000L +
	    (deadline->tv_nsec - now.tv_nsec) / 1000000L;
	return (left);
}

static void
mono_arm(struct timespec *deadline, int sec)
{
	if (clock_gettime(CLOCK_MONOTONIC, deadline) < 0)
		err(1, "clock_gettime");
	deadline->tv_sec += sec;
}

/*
 * The daemon answers FOLLOW with OK <nbytes> when the console is open, or
 * OK <nbytes> closed when it is not. A closed console stays on this connection
 * and sends one wake byte when it opens. --wait blocks only until that open,
 * not until output arrives. --timeout starts then and limits the read.
 */
static int
cmd_console(const char *jail, int follow, int timeout_sec, int wait_mode,
    int wait_sec, int color_mode)
{
	struct timespec show_dl, wait_dl;
	char line[CBSDLOG_NAME_MAX + 32];
	char reply[512];
	char word[16];
	char buf[4096];
	unsigned long nbody;
	int fd = -1;
	int closed = 0;
	int timing = 0;
	int in_line = 0;
	int to_err = 0;
	int painted = 0;
	int color_out;
	int color_err;
	int rc = 0;

	if (color_mode == COLOR_YES) {
		color_out = color_err = 1;
	} else if (color_mode == COLOR_NO) {
		color_out = color_err = 0;
	} else {
		color_out = isatty(STDOUT_FILENO);
		color_err = isatty(STDERR_FILENO);
	}

	if (!cbsdlog_valid_jail(jail))
		errx(1, "bad jail name");
	snprintf(line, sizeof(line), "FOLLOW %s\n", jail);
	if (wait_mode && wait_sec > 0)
		mono_arm(&wait_dl, wait_sec);
	fd = connect_ctl();
	if (write_all(fd, line, strlen(line)) < 0)
		err(1, "write");
	if (read_line(fd, reply, sizeof(reply)) < 0)
		err(1, "read");
	if (strncmp(reply, "OK", 2) != 0) {
		fprintf(stderr, "%s", reply);
		if (reply[0] != '\0' && reply[strlen(reply) - 1] != '\n')
			fputc('\n', stderr);
		rc = 1;
		goto out;
	}
	word[0] = '\0';
	if (sscanf(reply, "OK %lu %15s", &nbody, word) < 1)
		errx(1, "bad console reply");
	if (strcmp(word, "closed") == 0)
		closed = 1;
	while (nbody > 0) {
		size_t chunk = nbody > sizeof(buf) ? sizeof(buf) : (size_t)nbody;
		ssize_t n = read(fd, buf, chunk);

		if (n < 0) {
			if (errno == EINTR)
				continue;
			err(1, "read");
		}
		if (n == 0)
			errx(1, "short console reply");
		if (console_write(buf, (size_t)n, &in_line, &to_err, &painted,
		    color_out, color_err) < 0)
			err(1, "write");
		nbody -= (unsigned long)n;
	}
	if (closed) {
		ssize_t got;

		if (!wait_mode)
			goto out;
		for (;;) {
			struct pollfd pfd;
			long left;
			int ms;

			if (wait_sec <= 0) {
				ms = 0;
			} else {
				left = mono_left_ms(&wait_dl);
				if (left <= 0) {
					rc = 2;
					goto out;
				}
				ms = left > 1000000L ? 1000000 : (int)left;
			}
			pfd.fd = fd;
			pfd.events = POLLIN;
			if (poll(&pfd, 1, ms) < 0) {
				if (errno == EINTR)
					continue;
				err(1, "poll");
			}
			if ((pfd.revents & (POLLIN | POLLHUP | POLLERR)) == 0) {
				rc = 2;
				goto out;
			}
			got = read(fd, buf, sizeof(buf));
			if (got < 0) {
				if (errno == EINTR)
					continue;
				err(1, "read");
			}
			if (got == 0) {
				rc = 2;
				goto out;
			}
			if (buf[0] == CBSDLOG_CONS_WAKE) {
				memmove(buf, buf + 1, (size_t)got - 1);
				got--;
			}
			if (timeout_sec > 0 && !timing) {
				mono_arm(&show_dl, timeout_sec);
				timing = 1;
			}
			if (got > 0 && console_write(buf, (size_t)got, &in_line,
			    &to_err, &painted, color_out, color_err) < 0)
				err(1, "write");
			break;
		}
	} else if (timeout_sec > 0 && !timing) {
		mono_arm(&show_dl, timeout_sec);
		timing = 1;
	}
	if (!follow && timeout_sec <= 0)
		goto out;
	for (;;) {
		struct pollfd pfd;
		int ms = -1;
		ssize_t got;
		long left;

		if (timing) {
			left = mono_left_ms(&show_dl);
			if (left <= 0)
				goto out;
			ms = left > 1000000L ? 1000000 : (int)left;
		}
		pfd.fd = fd;
		pfd.events = POLLIN;
		if (poll(&pfd, 1, ms) < 0) {
			if (errno == EINTR)
				continue;
			err(1, "poll");
		}
		if ((pfd.revents & (POLLIN | POLLHUP | POLLERR)) == 0)
			goto out;
		got = read(fd, buf, sizeof(buf));
		if (got < 0) {
			if (errno == EINTR)
				continue;
			err(1, "read");
		}
		if (got == 0)
			goto out;
		if (console_write(buf, (size_t)got, &in_line, &to_err, &painted,
		    color_out, color_err) < 0)
			err(1, "write");
	}
out:
	if (painted) {
		int outfd = to_err ? STDERR_FILENO : STDOUT_FILENO;

		write_all(outfd, col_reset, strlen(col_reset));
	}
	if (fd >= 0)
		close(fd);
	return (rc);
}

static void usage(void);

static int
parse_color(const char *s)
{
	if (strcmp(s, "auto") == 0)
		return (COLOR_AUTO);
	if (strcmp(s, "yes") == 0)
		return (COLOR_YES);
	if (strcmp(s, "no") == 0)
		return (COLOR_NO);
	errx(1, "bad color");
}

static int
parse_sec(const char *s, const char *what)
{
	char *end;
	long val;

	errno = 0;
	val = strtol(s, &end, 10);
	if (end == s || *end != '\0' || errno != 0 || val < 0 || val > 86400)
		errx(1, "bad %s", what);
	return ((int)val);
}

/* argv[0] is the command name. -W is wait; -w is the global workdir. */
static int
cmd_console_args(int argc, char **argv)
{
	static const struct option longopts[] = {
		{ "follow", no_argument, NULL, 'f' },
		{ "timeout", required_argument, NULL, 't' },
		{ "wait", required_argument, NULL, 'W' },
		{ "color", required_argument, NULL, 'C' },
		{ NULL, 0, NULL, 0 }
	};
	int follow = 0;
	int timeout_sec = 0;
	int wait_mode = 0;
	int wait_sec = 0;
	int color_mode = COLOR_AUTO;
	int ch;

	optreset = 1;
	optind = 1;
	opterr = 0;
	while ((ch = getopt_long(argc, argv, "C:ft:W:", longopts, NULL)) != -1) {
		switch (ch) {
		case 'f':
			follow = 1;
			break;
		case 't':
			timeout_sec = parse_sec(optarg, "timeout");
			break;
		case 'W':
			wait_mode = 1;
			wait_sec = parse_sec(optarg, "wait");
			break;
		case 'C':
			if (optarg[0] == '\0')
				usage();
			color_mode = parse_color(optarg);
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 1)
		usage();
	return (cmd_console(argv[0], follow, timeout_sec, wait_mode, wait_sec,
	    color_mode));
}

static void
usage(void)
{
	fprintf(stderr,
	    "usage: cbsdlogctl [options] <command> [args]\n"
	    "\n"
	    "options:\n"
	    "       -w workdir\n"
	    "       -s sock\n"
	    "       -p pidfile\n"
	    "       -D dir\n"
	    "       -l logdir\n"
	    "\n"
	    "commands:\n"
	    "       enable <jail> <sockdir>\n"
	    "       disable <jail>\n"
	    "       create <jail>\n"
	    "       destroy <jail>\n"
	    "       exec <jail> -- <command> [args...]\n"
	    "       console [-C|--color auto|yes|no] [-f|--follow] [-t|--timeout seconds] [-W|--wait seconds] <jail>\n");
	exit(1);
}

static void
set_default_path(const char **slot, char *buf, size_t buflen, const char *wd,
    const char *rel)
{
	if (*slot != NULL)
		return;
	if (wd == NULL)
		errx(1, "no workdir");
	if (cbsdlog_under(buf, buflen, wd, rel) < 0)
		errx(1, "path too long");
	*slot = buf;
}

int
main(int argc, char **argv)
{
	const char *workdir_flag = NULL;
	const char *wd;
	int ch;

	signal(SIGPIPE, SIG_IGN);
	while ((ch = getopt(argc, argv, "w:s:p:D:l:")) != -1) {
		switch (ch) {
		case 'w':
			workdir_flag = optarg;
			break;
		case 's':
			ctl_path = optarg;
			break;
		case 'p':
			pidfile = optarg;
			break;
		case 'D':
			snipdir = optarg;
			break;
		case 'l':
			logdir = optarg;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc < 1)
		usage();
	wd = cbsdlog_workdir(workdir_flag);
	if (strcmp(argv[0], "enable") == 0) {
		if (argc != 3)
			usage();
		set_default_path(&ctl_path, ctl_buf, sizeof(ctl_buf), wd,
		    CBSDLOG_REL_CTL);
		return (cmd_enable(argv[1], argv[2]));
	}
	if (strcmp(argv[0], "create") == 0 || strcmp(argv[0], "destroy") == 0) {
		if (argc != 2)
			usage();
		set_default_path(&pidfile, pid_buf, sizeof(pid_buf), wd,
		    CBSDLOG_REL_SYSLOG_PID);
		set_default_path(&snipdir, snip_buf, sizeof(snip_buf), wd,
		    CBSDLOG_REL_SNIPPET);
		if (strcmp(argv[0], "create") == 0) {
			set_default_path(&logdir, log_buf, sizeof(log_buf), wd,
			    CBSDLOG_REL_LOG);
			return (cmd_create(argv[1]));
		}
		return (cmd_destroy(argv[1]));
	}
	set_default_path(&ctl_path, ctl_buf, sizeof(ctl_buf), wd, CBSDLOG_REL_CTL);

	if (strcmp(argv[0], "disable") == 0) {
		if (argc != 2)
			usage();
		return (cmd_disable(argv[1]));
	}
	if (strcmp(argv[0], "exec") == 0) {
		if (argc < 4 || strcmp(argv[2], "--") != 0)
			usage();
		return (cmd_exec(argv[1], argv + 3));
	}
	if (strcmp(argv[0], "console") == 0)
		return (cmd_console_args(argc, argv));
	usage();
	return (1);
}
