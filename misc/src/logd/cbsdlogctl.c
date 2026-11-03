/* cbsdlogctl — control tool for cbsdlogd. See LOGGER.md.
 *   enable <jail> <sockdir>
 *   disable <jail>
 *   create <jail>
 *   destroy <jail>
 *   exec <jail> -- <command> [args...]
 *   console [-t seconds] [--wait [seconds]] <jail>
 */

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
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

/*
 * timeout_sec 0 follows until the console closes.
 * wait_mode asks the daemon to stay if the console is not open yet.
 * wait_sec is how long to wait for it to open; 0 waits without a limit.
 * After it opens, timeout_sec applies. Returns 2 if the open wait expires.
 */
static int
cmd_console(const char *jail, int timeout_sec, int wait_mode, int wait_sec)
{
	struct timespec deadline, open_deadline;
	char line[CBSDLOG_NAME_MAX + 32];
	char reply[512];
	char buf[4096];
	int fd;
	int limited = timeout_sec > 0;
	int open_wait = 0;
	int strip_mark = 0;

	if (!cbsdlog_valid_jail(jail))
		errx(1, "bad jail name");
	if (wait_mode)
		snprintf(line, sizeof(line), "FOLLOW %s wait\n", jail);
	else
		snprintf(line, sizeof(line), "FOLLOW %s\n", jail);
	fd = connect_ctl();
	if (write_all(fd, line, strlen(line)) < 0)
		err(1, "write");
	if (read_line(fd, reply, sizeof(reply)) < 0)
		err(1, "read");
	if (strncmp(reply, "OK", 2) != 0) {
		fprintf(stderr, "%s", reply);
		if (reply[0] != '\0' && reply[strlen(reply) - 1] != '\n')
			fputc('\n', stderr);
		close(fd);
		return (1);
	}
	if (wait_mode && strncmp(reply, "OK wait", 7) == 0) {
		open_wait = 1;
		strip_mark = 1;
		if (wait_sec > 0) {
			if (clock_gettime(CLOCK_MONOTONIC, &open_deadline) < 0)
				err(1, "clock_gettime");
			open_deadline.tv_sec += wait_sec;
		}
	} else if (wait_mode && strncmp(reply, "OK held", 7) != 0) {
		/* A daemon without FOLLOW wait answers OK and drops the client. */
		close(fd);
		return (2);
	}
	if (limited) {
		if (clock_gettime(CLOCK_MONOTONIC, &deadline) < 0)
			err(1, "clock_gettime");
		deadline.tv_sec += timeout_sec;
	}
	for (;;) {
		struct pollfd pfd;
		struct timespec now;
		int ms = -1;
		ssize_t got;

		if (open_wait && wait_sec > 0) {
			long left;

			if (clock_gettime(CLOCK_MONOTONIC, &now) < 0)
				err(1, "clock_gettime");
			left = (open_deadline.tv_sec - now.tv_sec) * 1000L +
			    (open_deadline.tv_nsec - now.tv_nsec) / 1000000L;
			if (left <= 0) {
				close(fd);
				return (2);
			}
			ms = left > 1000000L ? 1000000 : (int)left;
		} else if (limited) {
			long left;

			if (clock_gettime(CLOCK_MONOTONIC, &now) < 0)
				err(1, "clock_gettime");
			left = (deadline.tv_sec - now.tv_sec) * 1000L +
			    (deadline.tv_nsec - now.tv_nsec) / 1000000L;
			if (left <= 0)
				break;
			ms = left > 1000000L ? 1000000 : (int)left;
		}
		pfd.fd = fd;
		pfd.events = POLLIN;
		if (poll(&pfd, 1, ms) < 0) {
			if (errno == EINTR)
				continue;
			err(1, "poll");
		}
		if ((pfd.revents & POLLIN) == 0) {
			if (open_wait && wait_sec > 0) {
				close(fd);
				return (2);
			}
			break;
		}
		got = read(fd, buf, sizeof(buf));
		if (got < 0) {
			if (errno == EINTR)
				continue;
			err(1, "read");
		}
		if (got == 0)
			break;
		if (strip_mark && (unsigned char)buf[0] == 1) {
			memmove(buf, buf + 1, (size_t)got - 1);
			got--;
			strip_mark = 0;
		}
		if (got > 0)
			open_wait = 0;
		if (got > 0 && write_all(STDOUT_FILENO, buf, (size_t)got) < 0)
			err(1, "write");
	}
	close(fd);
	return (0);
}

static void usage(void);

/* console [-t seconds|--timeout seconds] [--wait [seconds]] <jail> */
static int
cmd_console_args(int argc, char **argv)
{
	int timeout_sec = 0;
	int wait_mode = 0;
	int wait_sec = 0;
	int i = 0;

	while (i < argc && argv[i][0] == '-') {
		char *end;
		long val;

		if (strcmp(argv[i], "-t") == 0 ||
		    strcmp(argv[i], "--timeout") == 0) {
			if (i + 1 >= argc)
				usage();
			errno = 0;
			val = strtol(argv[i + 1], &end, 10);
			if (end == argv[i + 1] || *end != '\0' || errno != 0 ||
			    val < 0 || val > 86400)
				errx(1, "bad timeout");
			timeout_sec = (int)val;
			i += 2;
			continue;
		}
		if (strcmp(argv[i], "--wait") == 0) {
			wait_mode = 1;
			i++;
			if (i < argc && argv[i][0] >= '0' && argv[i][0] <= '9') {
				errno = 0;
				val = strtol(argv[i], &end, 10);
				if (end == argv[i] || *end != '\0' || errno != 0 ||
				    val < 0 || val > 86400)
					errx(1, "bad wait");
				wait_sec = (int)val;
				i++;
			}
			continue;
		}
		usage();
	}
	if (argc - i != 1)
		usage();
	return (cmd_console(argv[i], timeout_sec, wait_mode, wait_sec));
}

static void
usage(void)
{
	fprintf(stderr,
	    "usage: cbsdlogctl [-w workdir] [-s sock] enable <jail> <sockdir>\n"
	    "       cbsdlogctl [-w workdir] [-s sock] disable <jail>\n"
	    "       cbsdlogctl [-w workdir] [-p pidfile] [-D dir] [-l logdir]\n"
	    "                  create <jail>\n"
	    "       cbsdlogctl [-w workdir] [-p pidfile] [-D dir] destroy <jail>\n"
	    "       cbsdlogctl [-w workdir] [-s sock] exec <jail> -- <command> [args...]\n"
	    "       cbsdlogctl [-w workdir] [-s sock] console [-t seconds] [--wait [seconds]] <jail>\n");
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
		return (cmd_console_args(argc - 1, argv + 1));
	usage();
	return (1);
}
