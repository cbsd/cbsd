/* cbsdlogd — one host daemon for jail syslog sockets and jail consoles.
 * See LOGGER.md. Control socket: one command per line, answer OK or ERR.
 *   OPEN <jail> <path> <mode>
 *   CLOSE <jail>
 *   CONSOLE <jail>     SCM_RIGHTS: stdout read fd, stderr read fd
 *   FOLLOW <jail>
 */

#ifdef __linux__
#define _GNU_SOURCE /* struct ucred for SO_PEERCRED */
#include <sys/inotify.h>
#else
#include <sys/event.h>
#endif
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "cbsdlog.h"

#ifndef LOG_CONSOLE
#define LOG_CONSOLE	(14 << 3)
#endif

#define MAX_CLIENTS	64
#define JAIL_GROW	16
#define CONSOLE_LINE	512
#define CONSOLE_BUF	(32 * 1024)
#define DGRAM_MAX	8192
#define CMD_MAX		4096

struct jsock {
	int fd;
	int wd; /* inotify watch descriptor (Linux) */
	mode_t mode;
	char path[104];
};

struct jent {
	int used;
	char name[CBSDLOG_NAME_MAX];
	struct jsock sock[2];
	int nsock;
	int out_fd;
	int err_fd;
	char outacc[CONSOLE_LINE];
	char erracc[CONSOLE_LINE];
	size_t outlen;
	size_t errlen;
	char *buf;
	char *start;
	char *end;
};

struct client {
	int used;
	int fd;
	int follow;
	int wake; /* console was closed; tell this client when it opens */
	int nfds;
	int fds[2];
	char buf[CMD_MAX];
	size_t len;
};

static struct jent *jails;
static int njails;
static struct client clients[MAX_CLIENTS];
static const char *ctl_path;
static const char *pid_path;
static const char *syslog_path;
static char ctl_buf[PATH_MAX];
static char pid_buf[PATH_MAX];
static char syslog_buf[PATH_MAX];
static int listen_fd = -1;
static int pid_fd = -1;
static int syslog_fd = -1;
static int kq_fd = -1;
static volatile sig_atomic_t stop;

_Static_assert(sizeof(((struct sockaddr_un *)0)->sun_path) >= 104,
    "unix socket path");
_Static_assert(sizeof(((struct jsock *)0)->path) <=
    sizeof(((struct sockaddr_un *)0)->sun_path),
    "jsock path fits sun_path");

static void client_close(struct client *);
static int console_held(const struct jent *);
static void console_end_followers(struct jent *);
static void console_wake(struct jent *);

static void
on_signal(int sig)
{
	(void)sig;
	stop = 1;
}

static void
set_signals(void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = on_signal;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
}

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

static void
replyf(int fd, const char *fmt, ...)
{
	char buf[512];
	va_list ap;
	int n;

	va_start(ap, fmt);
	n = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	if (n < 0)
		return;
	if ((size_t)n >= sizeof(buf))
		n = (int)sizeof(buf) - 1;
	if (n == 0 || buf[n - 1] != '\n') {
		if ((size_t)n + 1 < sizeof(buf))
			buf[n++] = '\n';
	}
	write_all(fd, buf, (size_t)n);
}

/* Console lines are raw text. A log-socket datagram already carries this stamp. */
static void
fmt_3164(char *dst, size_t dstlen)
{
	static const char *mon[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	struct tm tm;
	time_t now = time(NULL);

	localtime_r(&now, &tm);
	snprintf(dst, dstlen, "%s %2d %02d:%02d:%02d",
	    mon[tm.tm_mon], tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

static int
is_3164(const char *s)
{
	static const char mon[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
	int i, ok = 0;

	if (strlen(s) < 15)
		return (0);
	for (i = 0; i < 12; i++) {
		if (memcmp(s, mon + i * 3, 3) == 0) {
			ok = 1;
			break;
		}
	}
	if (!ok || s[3] != ' ' || s[6] != ' ' || s[9] != ':' || s[12] != ':')
		return (0);
	if (!(s[4] == ' ' || isdigit((unsigned char)s[4])) ||
	    !isdigit((unsigned char)s[5]))
		return (0);
	if (!isdigit((unsigned char)s[7]) || !isdigit((unsigned char)s[8]) ||
	    !isdigit((unsigned char)s[10]) || !isdigit((unsigned char)s[11]) ||
	    !isdigit((unsigned char)s[13]) || !isdigit((unsigned char)s[14]))
		return (0);
	return (1);
}

/* Take one non-space token. start and len may be NULL to skip it. */
static int
next_token(const char *s, const char **start, size_t *len, const char **next)
{
	const char *b;

	while (*s == ' ')
		s++;
	if (*s == '\0')
		return (-1);
	b = s;
	while (*s != '\0' && *s != ' ')
		s++;
	if (start != NULL)
		*start = b;
	if (len != NULL)
		*len = (size_t)(s - b);
	*next = s;
	return (0);
}

/*
 * Strip a spoofed hostname and put the jail name in its place.
 * One datagram becomes one line: interior newlines are spaces.
 * Returns 0 when out holds a rewritten datagram. A datagram without
 * <pri> and an RFC 3164 or RFC 5424 header is ignored.
 */
static int
rewrite_syslog(const char *in, size_t inlen, const char *jail, char *out, size_t outsz)
{
	char buf[DGRAM_MAX];
	const char *p;
	const char *q;
	const char *body;
	const char *sp;
	const char *col;
	size_t n = inlen;
	size_t w = 0;
	int pri = 0;
	int v = 0;
	int digits = 0;

	if (n > sizeof(buf) - 1)
		n = sizeof(buf) - 1;
	while (n > 0 && (in[n - 1] == '\n' || in[n - 1] == '\r' || in[n - 1] == '\0'))
		n--;
	for (size_t i = 0; i < n && w + 1 < sizeof(buf); i++) {
		char c = in[i];

		if (c == '\0')
			break;
		if (c == '\n' || c == '\r')
			c = ' ';
		buf[w++] = c;
	}
	buf[w] = '\0';
	p = buf;

	if (*p != '<')
		return (-1);
	q = p + 1;
	while (isdigit((unsigned char)*q) && digits < 3) {
		v = v * 10 + (*q - '0');
		q++;
		digits++;
	}
	if (digits == 0 || *q != '>' || v > 191)
		return (-1);
	pri = v;
	p = q + 1;

	/* RFC 5424: <pri>VERSION SP TIMESTAMP SP HOSTNAME SP REST */
	if (*p >= '1' && *p <= '9') {
		const char *ver, *ts;
		size_t verlen, tslen;
		const char *np = p;

		if (next_token(np, &ver, &verlen, &np) == 0 &&
		    next_token(np, &ts, &tslen, &np) == 0 &&
		    next_token(np, NULL, NULL, &np) == 0) {
			while (*np == ' ')
				np++;
			snprintf(out, outsz, "<%d>%.*s %.*s %s%s%s",
			    pri, (int)verlen, ver, (int)tslen, ts, jail,
			    *np == '\0' ? "" : " ", np);
			return (0);
		}
	}

	while (*p == ' ')
		p++;
	if (!is_3164(p))
		return (-1);

	body = p + 15;
	if (*body == ' ')
		body++;
	sp = strchr(body, ' ');
	col = strchr(body, ':');
	/* A space before any ':' means the first word is a hostname. */
	if (sp != NULL && (col == NULL || sp < col)) {
		body = sp;
		while (*body == ' ')
			body++;
	}
	if (*body == '\0')
		snprintf(out, outsz, "<%d>%.*s %s", pri, 15, p, jail);
	else
		snprintf(out, outsz, "<%d>%.*s %s %s", pri, 15, p, jail, body);
	return (0);
}

static void
syslog_send(const char *msg)
{
	struct sockaddr_un un;

	if (syslog_fd < 0) {
		syslog_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
		if (syslog_fd < 0)
			return;
	}
	if (cbsdlog_set_un(&un, syslog_path) < 0)
		return;
	if (sendto(syslog_fd, msg, strlen(msg), 0,
	    (struct sockaddr *)&un, cbsdlog_un_len(&un)) < 0) {
		close(syslog_fd);
		syslog_fd = -1;
	}
}

static char *
console_next(struct jent *j, char *p)
{
	p++;
	if (p == j->buf + CONSOLE_BUF)
		return (j->buf);
	return (p);
}

static size_t
console_used(const struct jent *j)
{
	if (j->buf == NULL || j->start == j->end)
		return (0);
	if (j->start < j->end)
		return ((size_t)(j->end - j->start));
	return ((size_t)(j->buf + CONSOLE_BUF - j->start) +
	    (size_t)(j->end - j->buf));
}

static int
console_drop_line(struct jent *j)
{
	char *p;

	if (j->start == j->end)
		return (0);
	p = j->start;
	for (;;) {
		int nl = (*p == '\n');

		p = console_next(j, p);
		if (nl) {
			j->start = p;
			return (1);
		}
		if (p == j->end)
			return (0);
	}
}

static void
console_put(struct jent *j, const char *s, size_t n)
{
	while (n > 0) {
		size_t chunk = (size_t)(j->buf + CONSOLE_BUF - j->end);

		if (chunk > n)
			chunk = n;
		memcpy(j->end, s, chunk);
		j->end += chunk;
		if (j->end == j->buf + CONSOLE_BUF)
			j->end = j->buf;
		s += chunk;
		n -= chunk;
	}
}

/* Append one physical line. A tag byte marks stdout or stderr. */
static void
console_add(struct jent *j, int is_err, const char *line)
{
	char tag = is_err ? CBSDLOG_CONS_ERR : CBSDLOG_CONS_OUT;
	size_t n = strlen(line);
	size_t need;

	if (j->buf == NULL) {
		j->buf = malloc(CONSOLE_BUF);
		if (j->buf == NULL)
			return;
		j->start = j->end = j->buf;
	}
	if (n + 2 > CONSOLE_BUF - 1) {
		line += n - (CONSOLE_BUF - 3);
		n = CONSOLE_BUF - 3;
	}
	need = n + 2;
	while (CONSOLE_BUF - 1 - console_used(j) < need) {
		if (!console_drop_line(j)) {
			j->start = j->end = j->buf;
			break;
		}
	}
	console_put(j, &tag, 1);
	console_put(j, line, n);
	console_put(j, "\n", 1);
}

/* Replay stored lines. Wrapped data is start..end-of-buf then buf..end. */
static int
console_replay(struct jent *j, int fd)
{
	if (j->buf == NULL || j->start == j->end)
		return (0);
	if (j->start < j->end)
		return (write_all(fd, j->start, (size_t)(j->end - j->start)));
	if (write_all(fd, j->start, (size_t)(j->buf + CONSOLE_BUF - j->start)) < 0)
		return (-1);
	return (write_all(fd, j->buf, (size_t)(j->end - j->buf)));
}

static int
write_console_line(int fd, int is_err, const char *line)
{
	char tag = is_err ? CBSDLOG_CONS_ERR : CBSDLOG_CONS_OUT;

	if (write_all(fd, &tag, 1) < 0)
		return (-1);
	if (line[0] != '\0' && write_all(fd, line, strlen(line)) < 0)
		return (-1);
	return (write_all(fd, "\n", 1));
}

static void
fanout(struct jent *j, int is_err, const char *line)
{
	int idx = (int)(j - jails);
	int i;

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (!clients[i].used || clients[i].follow != idx)
			continue;
		if (write_console_line(clients[i].fd, is_err, line) < 0)
			client_close(&clients[i]);
	}
}

static void
emit_console(struct jent *j, int is_err, const char *line)
{
	char ts[32];
	char msg[DGRAM_MAX];
	int pri = is_err ? LOG_MAKEPRI(LOG_CONSOLE, LOG_ERR) :
	    LOG_MAKEPRI(LOG_CONSOLE, LOG_INFO);

	fmt_3164(ts, sizeof(ts));
	snprintf(msg, sizeof(msg), "<%d>%s %s console: %s", pri, ts, j->name, line);
	syslog_send(msg);
	console_add(j, is_err, line);
	fanout(j, is_err, line);
}

static void
client_close(struct client *c)
{
	int i;

	if (c->fd >= 0)
		close(c->fd);
	for (i = 0; i < c->nfds; i++) {
		if (c->fds[i] >= 0)
			close(c->fds[i]);
	}
	memset(c, 0, sizeof(*c));
	c->fd = -1;
	c->follow = -1;
	c->fds[0] = c->fds[1] = -1;
}

static void
drop_fds(struct client *c)
{
	int i;

	for (i = 0; i < c->nfds; i++) {
		if (c->fds[i] >= 0)
			close(c->fds[i]);
		c->fds[i] = -1;
	}
	c->nfds = 0;
}

static void
jail_clear(struct jent *j)
{
	int idx = (int)(j - jails);
	int i;

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (clients[i].used && clients[i].follow == idx)
			client_close(&clients[i]);
	}
	for (i = 0; i < j->nsock; i++) {
		if (j->sock[i].fd >= 0)
			close(j->sock[i].fd);
		if (j->sock[i].path[0] != '\0')
			unlink(j->sock[i].path);
	}
	if (j->out_fd >= 0)
		close(j->out_fd);
	if (j->err_fd >= 0)
		close(j->err_fd);
	free(j->buf);
	memset(j, 0, sizeof(*j));
	j->out_fd = j->err_fd = -1;
	j->sock[0].fd = j->sock[1].fd = -1;
}

static struct jent *
jail_find(const char *name)
{
	int i;

	for (i = 0; i < njails; i++) {
		if (jails[i].used && strcmp(jails[i].name, name) == 0)
			return (&jails[i]);
	}
	return (NULL);
}

/* follow stores an index. Realloc moves the array and leaves indexes valid. */
static int
jails_grow(void)
{
	struct jent *p;
	int n;
	int i;

	if (njails > INT_MAX / 2)
		return (-1);
	n = njails == 0 ? JAIL_GROW : njails * 2;
	if ((size_t)n > SIZE_MAX / sizeof(*jails))
		return (-1);
	p = realloc(jails, (size_t)n * sizeof(*jails));
	if (p == NULL)
		return (-1);
	for (i = njails; i < n; i++) {
		memset(&p[i], 0, sizeof(p[i]));
		p[i].out_fd = p[i].err_fd = -1;
		p[i].sock[0].fd = p[i].sock[1].fd = -1;
	}
	jails = p;
	njails = n;
	return (0);
}

static struct jent *
jail_add(const char *name)
{
	struct jent *j;
	int i;

	j = jail_find(name);
	if (j != NULL)
		return (j);
	for (;;) {
		for (i = 0; i < njails; i++) {
			if (jails[i].used)
				continue;
			memset(&jails[i], 0, sizeof(jails[i]));
			jails[i].used = 1;
			jails[i].out_fd = jails[i].err_fd = -1;
			jails[i].sock[0].fd = jails[i].sock[1].fd = -1;
			strlcpy(jails[i].name, name, sizeof(jails[i].name));
			return (&jails[i]);
		}
		if (jails_grow() < 0)
			return (NULL);
	}
}

static int
has_dotdot(const char *path)
{
	const char *p;

	for (p = path; *p != '\0'; p++) {
		if (p[0] == '.' && p[1] == '.' &&
		    (p == path || p[-1] == '/') &&
		    (p[2] == '/' || p[2] == '\0'))
			return (1);
	}
	return (0);
}

static int
check_path(const char *path, char *err, size_t errlen)
{
	if (path[0] != '/') {
		snprintf(err, errlen, "path is not absolute");
		return (-1);
	}
	if (strlen(path) >= sizeof(((struct sockaddr_un *)0)->sun_path)) {
		snprintf(err, errlen, "path too long");
		return (-1);
	}
	if (has_dotdot(path) || path[strlen(path) - 1] == '/') {
		snprintf(err, errlen, "path is not a socket path");
		return (-1);
	}
	if (strcmp(path, "/var/run/log") == 0 ||
	    strcmp(path, "/var/run/logpriv") == 0 ||
	    strcmp(path, "/dev/log") == 0 ||
	    strcmp(path, ctl_path) == 0) {
		snprintf(err, errlen, "path is reserved");
		return (-1);
	}
	return (0);
}

static int
parse_mode(const char *s, mode_t *mode)
{
	char *end;
	long v;

	if (s[0] < '0' || s[0] > '7')
		return (-1);
	v = strtol(s, &end, 8);
	if (*end != '\0' || (v != 0600 && v != 0666))
		return (-1);
	*mode = (mode_t)v;
	return (0);
}

/* Watch for the socket file being removed: cleanvar unlinks the name.
 * The fd stays open, but syslog looks up the path, so we rebind. */
static void
watch_sock(struct jsock *js)
{
#ifdef __linux__
	char dir[sizeof(js->path)];
	char *slash;
	int wd;

	if (kq_fd < 0 || js->fd < 0)
		return;
	strlcpy(dir, js->path, sizeof(dir));
	slash = strrchr(dir, '/');
	if (slash == NULL)
		return;
	if (slash == dir)
		dir[1] = '\0';
	else
		slash[0] = '\0';
	wd = inotify_add_watch(kq_fd, dir, IN_DELETE | IN_MOVED_FROM);
	if (wd < 0) {
		syslog(LOG_WARNING, "inotify_add_watch %s: %s", dir,
		    strerror(errno));
		return;
	}
	js->wd = wd;
#else
	struct kevent ev;

	if (kq_fd < 0 || js->fd < 0)
		return;
	EV_SET(&ev, (uintptr_t)js->fd, EVFILT_VNODE, EV_ADD | EV_CLEAR,
	    NOTE_DELETE | NOTE_REVOKE, 0, NULL);
	if (kevent(kq_fd, &ev, 1, NULL, 0, NULL) < 0)
		syslog(LOG_WARNING, "kevent add fd %d: %s", js->fd,
		    strerror(errno));
#endif
}

static int
bind_jail_sock(struct jent *j, const char *path, mode_t mode, char *err, size_t errlen)
{
	struct sockaddr_un un;
	struct stat st;
	int slot = -1;
	int is_new;
	int fd;
	int i;
	int saved;
	mode_t old;

	for (i = 0; i < j->nsock; i++) {
		if (j->sock[i].fd >= 0 && strcmp(j->sock[i].path, path) == 0)
			slot = i;
	}
	if (slot < 0) {
		for (i = 0; i < j->nsock; i++) {
			if (j->sock[i].fd < 0) {
				slot = i;
				break;
			}
		}
	}
	if (slot < 0) {
		if (j->nsock >= 2) {
			snprintf(err, errlen, "jail already has two sockets");
			return (-1);
		}
		slot = j->nsock;
	}
	is_new = (slot == j->nsock);
	if (!is_new && j->sock[slot].fd >= 0) {
		close(j->sock[slot].fd);
		unlink(j->sock[slot].path);
		j->sock[slot].fd = -1;
		j->sock[slot].path[0] = '\0';
	}

	if (lstat(path, &st) == 0) {
		if (!S_ISSOCK(st.st_mode)) {
			snprintf(err, errlen, "path exists and is not a socket");
			return (-1);
		}
		if (unlink(path) < 0) {
			snprintf(err, errlen, "unlink: %s", strerror(errno));
			return (-1);
		}
	}

	fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (fd < 0) {
		snprintf(err, errlen, "socket: %s", strerror(errno));
		return (-1);
	}
	if (cbsdlog_set_un(&un, path) < 0) {
		close(fd);
		snprintf(err, errlen, "path too long");
		return (-1);
	}
	old = umask(0);
	saved = 0;
	if (bind(fd, (struct sockaddr *)&un, cbsdlog_un_len(&un)) < 0)
		saved = errno;
	umask(old);
	if (saved != 0) {
		close(fd);
		snprintf(err, errlen, "bind: %s", strerror(saved));
		return (-1);
	}
	/* fchmod(2) on a socket fd succeeds on Linux but does not change
	 * the socket file mode, so chmod(2) the path as well. */
	if (fchmod(fd, mode) < 0 && errno != EINVAL)
		saved = errno;
	if (saved == 0 && chmod(path, mode) < 0)
		saved = errno;
	if (saved != 0) {
		close(fd);
		unlink(path);
		snprintf(err, errlen, "chmod: %s", strerror(saved));
		return (-1);
	}
	j->sock[slot].fd = fd;
	j->sock[slot].mode = mode;
	strlcpy(j->sock[slot].path, path, sizeof(j->sock[slot].path));
	if (is_new)
		j->nsock++;
	watch_sock(&j->sock[slot]);
	return (0);
}

static void
take_fds(struct client *c, struct msghdr *msg)
{
	struct cmsghdr *cmsg;

	for (cmsg = CMSG_FIRSTHDR(msg); cmsg != NULL; cmsg = CMSG_NXTHDR(msg, cmsg)) {
		int *fdsp;
		int nfd;
		int i;

		if (cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS)
			continue;
		nfd = (int)((cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(int));
		fdsp = (int *)CMSG_DATA(cmsg);
		for (i = 0; i < nfd; i++) {
			if (c->nfds < 2)
				c->fds[c->nfds++] = fdsp[i];
			else
				close(fdsp[i]);
		}
	}
}

static void
cmd_open(struct client *c, const char *name, const char *path, const char *modestr)
{
	struct jent *j;
	mode_t mode;
	char err[128];

	drop_fds(c);
	if (!cbsdlog_valid_jail(name)) {
		replyf(c->fd, "ERR bad jail name");
		client_close(c);
		return;
	}
	if (check_path(path, err, sizeof(err)) < 0) {
		replyf(c->fd, "ERR %s", err);
		client_close(c);
		return;
	}
	if (parse_mode(modestr, &mode) < 0) {
		replyf(c->fd, "ERR bad mode");
		client_close(c);
		return;
	}
	j = jail_add(name);
	if (j == NULL) {
		replyf(c->fd, "ERR out of memory");
		client_close(c);
		return;
	}
	if (bind_jail_sock(j, path, mode, err, sizeof(err)) < 0) {
		if (j->nsock == 0 && j->out_fd < 0)
			jail_clear(j);
		replyf(c->fd, "ERR %s", err);
		client_close(c);
		return;
	}
	replyf(c->fd, "OK");
	client_close(c);
}

static void
cmd_close(struct client *c, const char *name)
{
	struct jent *j;

	drop_fds(c);
	if (!cbsdlog_valid_jail(name)) {
		replyf(c->fd, "ERR bad jail name");
		client_close(c);
		return;
	}
	j = jail_find(name);
	if (j == NULL) {
		replyf(c->fd, "ERR no such jail");
		client_close(c);
		return;
	}
	jail_clear(j);
	replyf(c->fd, "OK");
	client_close(c);
}

static void
cmd_console(struct client *c, const char *name)
{
	struct jent *j;

	if (!cbsdlog_valid_jail(name)) {
		drop_fds(c);
		replyf(c->fd, "ERR bad jail name");
		client_close(c);
		return;
	}
	if (c->nfds != 2) {
		drop_fds(c);
		replyf(c->fd, "ERR need stdout and stderr fds");
		client_close(c);
		return;
	}
	j = jail_add(name);
	if (j == NULL) {
		drop_fds(c);
		replyf(c->fd, "ERR out of memory");
		client_close(c);
		return;
	}
	if (j->out_fd >= 0)
		close(j->out_fd);
	if (j->err_fd >= 0)
		close(j->err_fd);
	j->out_fd = c->fds[0];
	j->err_fd = c->fds[1];
	j->outlen = j->errlen = 0;
	c->fds[0] = c->fds[1] = -1;
	c->nfds = 0;
	console_wake(j);
	replyf(c->fd, "OK");
	client_close(c);
}

/* Console was closed at FOLLOW. One byte when it opens, before any new line. */
static void
console_wake(struct jent *j)
{
	char mark = CBSDLOG_CONS_WAKE;
	int idx = (int)(j - jails);
	int i;

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (!clients[i].used || clients[i].follow != idx || !clients[i].wake)
			continue;
		clients[i].wake = 0;
		if (write_all(clients[i].fd, &mark, 1) < 0)
			client_close(&clients[i]);
	}
}

/* Replay the buffer and stay. A closed console is reported, not a timer. */
static void
cmd_follow(struct client *c, const char *name)
{
	struct jent *j;
	int fl;
	size_t n;
	int held;

	drop_fds(c);
	if (!cbsdlog_valid_jail(name)) {
		replyf(c->fd, "ERR bad jail name");
		client_close(c);
		return;
	}
	j = jail_find(name);
	if (j == NULL) {
		replyf(c->fd, "ERR no such jail");
		client_close(c);
		return;
	}
	held = console_held(j);
	n = console_used(j);
	if (held)
		replyf(c->fd, "OK %zu", n);
	else
		replyf(c->fd, "OK %zu closed", n);
	if (n > 0 && console_replay(j, c->fd) < 0) {
		client_close(c);
		return;
	}
	fl = fcntl(c->fd, F_GETFL);
	if (fl >= 0)
		fcntl(c->fd, F_SETFL, fl | O_NONBLOCK);
	c->follow = (int)(j - jails);
	c->wake = !held;
}

static void
handle_line(struct client *c)
{
	char name[CBSDLOG_NAME_MAX];
	char path[104];
	char mode[16];
	char *nl;

	nl = strchr(c->buf, '\n');
	if (nl != NULL)
		*nl = '\0';
	if ((nl = strchr(c->buf, '\r')) != NULL)
		*nl = '\0';

	if (sscanf(c->buf, "OPEN %63s %103s %15s", name, path, mode) == 3) {
		cmd_open(c, name, path, mode);
		return;
	}
	if (sscanf(c->buf, "CLOSE %63s", name) == 1 &&
	    strncmp(c->buf, "CLOSE ", 6) == 0) {
		cmd_close(c, name);
		return;
	}
	if (sscanf(c->buf, "CONSOLE %63s", name) == 1 &&
	    strncmp(c->buf, "CONSOLE ", 8) == 0) {
		cmd_console(c, name);
		return;
	}
	if (sscanf(c->buf, "FOLLOW %63s", name) == 1 &&
	    strncmp(c->buf, "FOLLOW ", 7) == 0 &&
	    strchr(c->buf + 7, ' ') == NULL) {
		cmd_follow(c, name);
		return;
	}
	drop_fds(c);
	replyf(c->fd, "ERR unknown command");
	client_close(c);
}

static void
client_on_data(struct client *c)
{
	char ctrl[CMSG_SPACE(sizeof(int) * 8)];
	char tmp[1024];
	struct iovec iov;
	struct msghdr msg;
	ssize_t n;
	ssize_t i;

	memset(&msg, 0, sizeof(msg));
	iov.iov_base = tmp;
	iov.iov_len = sizeof(tmp);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = ctrl;
	msg.msg_controllen = sizeof(ctrl);

	n = recvmsg(c->fd, &msg, 0);
	if (n < 0) {
		if (errno == EINTR || errno == EAGAIN)
			return;
		client_close(c);
		return;
	}
	if (n == 0) {
		client_close(c);
		return;
	}
	if (msg.msg_flags & MSG_CTRUNC) {
		replyf(c->fd, "ERR control message truncated");
		client_close(c);
		return;
	}
	take_fds(c, &msg);
	for (i = 0; i < n; i++) {
		if (c->len + 1 >= sizeof(c->buf)) {
			replyf(c->fd, "ERR line too long");
			client_close(c);
			return;
		}
		c->buf[c->len++] = tmp[i];
		if (tmp[i] == '\n') {
			c->buf[c->len] = '\0';
			handle_line(c);
			return;
		}
	}
}

static int
console_held(const struct jent *j)
{
	return (j->out_fd >= 0 || j->err_fd >= 0);
}

/* Drop console followers once every writer of this jail's console has exited. */
static void
console_end_followers(struct jent *j)
{
	int idx;
	int i;

	if (console_held(j))
		return;
	idx = (int)(j - jails);
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (clients[i].used && clients[i].follow == idx)
			client_close(&clients[i]);
	}
}

static void
pipe_read(struct jent *j, int is_err)
{
	int *fdp = is_err ? &j->err_fd : &j->out_fd;
	char *acc = is_err ? j->erracc : j->outacc;
	size_t *len = is_err ? &j->errlen : &j->outlen;
	char tmp[1024];
	ssize_t n;
	ssize_t i;

	n = read(*fdp, tmp, sizeof(tmp));
	if (n < 0) {
		if (errno == EINTR || errno == EAGAIN)
			return;
		close(*fdp);
		*fdp = -1;
		*len = 0;
		console_end_followers(j);
		return;
	}
	if (n == 0) {
		if (*len > 0) {
			acc[*len] = '\0';
			emit_console(j, is_err, acc);
			*len = 0;
		}
		close(*fdp);
		*fdp = -1;
		console_end_followers(j);
		return;
	}
	for (i = 0; i < n; i++) {
		char ch = tmp[i];

		if (ch == '\n') {
			acc[*len] = '\0';
			emit_console(j, is_err, acc);
			*len = 0;
			continue;
		}
		if (*len + 1 >= CONSOLE_LINE) {
			acc[*len] = '\0';
			emit_console(j, is_err, acc);
			*len = 0;
		}
		acc[(*len)++] = ch;
	}
}

static void
dgram_read(struct jent *j, int slot)
{
	char buf[DGRAM_MAX];
	char out[DGRAM_MAX];
	ssize_t n;

	n = recv(j->sock[slot].fd, buf, sizeof(buf), 0);
	if (n <= 0)
		return;
	if (rewrite_syslog(buf, (size_t)n, j->name, out, sizeof(out)) == 0)
		syslog_send(out);
}

#ifdef __linux__
/* Linux has no getpeereid(3): use SO_PEERCRED. */
static int
getpeereid(int fd, uid_t *uid, gid_t *gid)
{
	struct ucred cr;
	socklen_t len = sizeof(cr);

	if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cr, &len) < 0)
		return (-1);
	*uid = cr.uid;
	*gid = cr.gid;
	return (0);
}
#endif

static int
peer_ok(int fd)
{
	uid_t uid;
	gid_t gid;

	if (getpeereid(fd, &uid, &gid) < 0)
		return (0);
	if (geteuid() == 0)
		return (uid == 0);
	return (uid == geteuid());
}

static void
accept_client(void)
{
	struct client *c = NULL;
	int fd;
	int i;

	fd = accept(listen_fd, NULL, NULL);
	if (fd < 0)
		return;
	if (!peer_ok(fd)) {
		replyf(fd, "ERR permission denied");
		close(fd);
		return;
	}
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (!clients[i].used) {
			c = &clients[i];
			break;
		}
	}
	if (c == NULL) {
		replyf(fd, "ERR too many clients");
		close(fd);
		return;
	}
	memset(c, 0, sizeof(*c));
	c->used = 1;
	c->fd = fd;
	c->follow = -1;
	c->fds[0] = c->fds[1] = -1;
}

#ifndef __linux__
static struct jsock *
sock_by_fd(int fd, struct jent **jp)
{
	int ji;
	int s;

	for (ji = 0; ji < njails; ji++) {
		if (!jails[ji].used)
			continue;
		for (s = 0; s < jails[ji].nsock; s++) {
			if (jails[ji].sock[s].fd == fd) {
				*jp = &jails[ji];
				return (&jails[ji].sock[s]);
			}
		}
	}
	return (NULL);
}
#endif /* __linux__ */

/* The socket file name is gone. Bind it again before later rc scripts connect. */
static void
sockets_rebind(void)
{
	struct jent *j;
	struct jsock *js;
	char path[sizeof(jails[0].sock[0].path)];
	char err[128];
	mode_t mode;
#ifndef __linux__
	int fd;
#endif

#ifdef __linux__
	char evbuf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
	const struct inotify_event *ev;
	const char *base;
	ssize_t n;
	size_t off;
	int ji;
	int s;

	for (;;) {
		n = read(kq_fd, evbuf, sizeof(evbuf));
		if (n < 0) {
			if (errno == EINTR)
				continue;
			if (errno == EAGAIN)
				return;
			syslog(LOG_WARNING, "inotify read: %s", strerror(errno));
			/* A dead queue is always readable to poll. Drop it. */
			if (errno == EBADF)
				kq_fd = -1;
			return;
		}
		break;
	}
	for (off = 0; off + sizeof(*ev) <= (size_t)n;
	    off += sizeof(*ev) + ev->len) {
		ev = (const struct inotify_event *)(evbuf + off);
		if ((ev->mask & (IN_DELETE | IN_MOVED_FROM)) == 0 || ev->len == 0)
			continue;
		for (ji = 0; ji < njails; ji++) {
			if (!jails[ji].used)
				continue;
			for (s = 0; s < jails[ji].nsock; s++) {
				js = &jails[ji].sock[s];
				if (js->fd < 0 || js->wd != ev->wd)
					continue;
				base = strrchr(js->path, '/');
				base = (base != NULL) ? base + 1 : js->path;
				if (strcmp(base, ev->name) != 0)
					continue;
				j = &jails[ji];
				mode = js->mode;
				strlcpy(path, js->path, sizeof(path));
				if (bind_jail_sock(j, path, mode, err,
					sizeof(err)) < 0)
					syslog(LOG_WARNING, "rebind %s: %s",
					    path, err);
				else
					syslog(LOG_INFO, "rebound %s", path);
			}
		}
	}
#else
	struct kevent evs[16];
	struct timespec zero;
	int n;
	int i;

	memset(&zero, 0, sizeof(zero));
	for (;;) {
		n = kevent(kq_fd, NULL, 0, evs, 16, &zero);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			syslog(LOG_WARNING, "kevent: %s", strerror(errno));
			/* A dead queue is always readable to poll. Drop it. */
			if (errno == EBADF)
				kq_fd = -1;
			return;
		}
		if (n == 0)
			return;
		for (i = 0; i < n; i++) {
			if ((evs[i].flags & EV_ERROR) != 0)
				continue;
			if (evs[i].filter != EVFILT_VNODE)
				continue;
			fd = (int)evs[i].ident;
			js = sock_by_fd(fd, &j);
			if (js == NULL)
				continue;
			mode = js->mode;
			strlcpy(path, js->path, sizeof(path));
			if (bind_jail_sock(j, path, mode, err, sizeof(err)) < 0)
				syslog(LOG_WARNING, "rebind %s: %s", path, err);
			else
				syslog(LOG_INFO, "rebound %s", path);
		}
	}
#endif
}

static void
loop(void)
{
	while (!stop) {
		struct pollfd pfd[2 + MAX_CLIENTS + njails * 4];
		int slot_client[2 + MAX_CLIENTS + njails * 4];
		int slot_jail[2 + MAX_CLIENTS + njails * 4];
		int slot_aux[2 + MAX_CLIENTS + njails * 4];
		int np = 0;
		int kq_ready = 0;
		int i;
		int n;

		pfd[np].fd = listen_fd;
		pfd[np].events = POLLIN;
		slot_client[np] = -1;
		slot_jail[np] = -1;
		slot_aux[np] = 0;
		np++;

		if (kq_fd >= 0) {
			pfd[np].fd = kq_fd;
			pfd[np].events = POLLIN;
			slot_client[np] = -1;
			slot_jail[np] = -1;
			slot_aux[np] = -3;
			np++;
		}

		for (i = 0; i < MAX_CLIENTS; i++) {
			if (!clients[i].used)
				continue;
			pfd[np].fd = clients[i].fd;
			pfd[np].events = POLLIN;
			slot_client[np] = i;
			slot_jail[np] = -1;
			slot_aux[np] = 0;
			np++;
		}
		for (i = 0; i < njails; i++) {
			int s;

			if (!jails[i].used)
				continue;
			for (s = 0; s < jails[i].nsock; s++) {
				if (jails[i].sock[s].fd < 0)
					continue;
				pfd[np].fd = jails[i].sock[s].fd;
				pfd[np].events = POLLIN;
				slot_client[np] = -1;
				slot_jail[np] = i;
				slot_aux[np] = s;
				np++;
			}
			if (jails[i].out_fd >= 0) {
				pfd[np].fd = jails[i].out_fd;
				pfd[np].events = POLLIN;
				slot_client[np] = -1;
				slot_jail[np] = i;
				slot_aux[np] = -1;
				np++;
			}
			if (jails[i].err_fd >= 0) {
				pfd[np].fd = jails[i].err_fd;
				pfd[np].events = POLLIN;
				slot_client[np] = -1;
				slot_jail[np] = i;
				slot_aux[np] = -2;
				np++;
			}
		}

		n = poll(pfd, (nfds_t)np, -1);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			err(1, "poll");
		}

		for (i = 1; i < np; i++) {
			if (pfd[i].revents == 0)
				continue;
			if (slot_aux[i] == -3) {
				if ((pfd[i].revents & POLLIN) != 0)
					kq_ready = 1;
				/*
				 * fork(2) leaves this index closed. poll
				 * reports POLLNVAL and returns at once.
				 */
				if ((pfd[i].revents & (POLLERR | POLLNVAL)) != 0) {
					syslog(LOG_WARNING,
					    "kqueue fd %d is not usable", kq_fd);
					kq_fd = -1;
				}
				continue;
			}
			if (slot_client[i] >= 0) {
				struct client *c = &clients[slot_client[i]];

				if (!c->used)
					continue;
				if (c->follow >= 0) {
					char dump[64];
					ssize_t got = read(c->fd, dump, sizeof(dump));

					if (got < 0 && (errno == EINTR || errno == EAGAIN))
						continue;
					client_close(c);
					continue;
				}
				if (pfd[i].revents & (POLLERR | POLLNVAL)) {
					client_close(c);
					continue;
				}
				client_on_data(c);
				continue;
			}
			if (slot_jail[i] >= 0 && jails[slot_jail[i]].used) {
				struct jent *j = &jails[slot_jail[i]];

				if (slot_aux[i] >= 0)
					dgram_read(j, slot_aux[i]);
				else if (slot_aux[i] == -1 && j->out_fd >= 0)
					pipe_read(j, 0);
				else if (slot_aux[i] == -2 && j->err_fd >= 0)
					pipe_read(j, 1);
			}
		}
		if (pfd[0].revents & POLLIN)
			accept_client();
		if (kq_ready)
			sockets_rebind();
	}
}

static void
shutdown_all(void)
{
	int i;

	for (i = 0; i < njails; i++) {
		if (jails[i].used)
			jail_clear(&jails[i]);
	}
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (clients[i].used)
			client_close(&clients[i]);
	}
	if (listen_fd >= 0) {
		close(listen_fd);
		unlink(ctl_path);
		listen_fd = -1;
	}
	if (syslog_fd >= 0) {
		close(syslog_fd);
		syslog_fd = -1;
	}
	if (pid_fd >= 0) {
		close(pid_fd);
		unlink(pid_path);
		pid_fd = -1;
	}
}

static void
listen_ctl(void)
{
	struct sockaddr_un un;
	mode_t old;

	if (cbsdlog_set_un(&un, ctl_path) < 0)
		errx(1, "control socket path is too long");
	listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (listen_fd < 0)
		err(1, "socket");
	unlink(ctl_path);
	old = umask(077);
	if (bind(listen_fd, (struct sockaddr *)&un, cbsdlog_un_len(&un)) < 0)
		err(1, "bind %s", ctl_path);
	umask(old);
	if (fchmod(listen_fd, 0600) < 0 &&
	    (errno != EINVAL || chmod(ctl_path, 0600) < 0))
		err(1, "chmod %s", ctl_path);
	if (listen(listen_fd, 128) < 0)
		err(1, "listen");
}

static void
lock_pid(void)
{
	pid_fd = open(pid_path, O_CREAT | O_RDWR, 0644);
	if (pid_fd < 0)
		err(1, "open %s", pid_path);
	if (flock(pid_fd, LOCK_EX | LOCK_NB) < 0) {
		if (errno == EWOULDBLOCK)
			errx(1, "cbsdlogd is already running");
		err(1, "flock %s", pid_path);
	}
}

static void
write_pid(void)
{
	if (ftruncate(pid_fd, 0) < 0)
		err(1, "ftruncate %s", pid_path);
	if (dprintf(pid_fd, "%d\n", (int)getpid()) < 0)
		err(1, "write %s", pid_path);
}

static int
daemonize(void)
{
	int fd;
	pid_t pid;

	pid = fork();
	if (pid < 0)
		return (-1);
	if (pid > 0)
		_exit(0);
	if (setsid() < 0)
		return (-1);
	if (chdir("/") < 0)
		return (-1);
	fd = open("/dev/null", O_RDWR);
	if (fd >= 0) {
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		if (fd > STDERR_FILENO)
			close(fd);
	}
	return (0);
}

static void
usage(void)
{
	fprintf(stderr,
	    "usage: cbsdlogd [-d] [-w workdir] [-c ctl] [-s syslog] [-P pidfile]\n");
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
	int foreground = 0;
	int ch;
	int i;

	while ((ch = getopt(argc, argv, "dw:c:s:P:")) != -1) {
		switch (ch) {
		case 'd':
			foreground = 1;
			break;
		case 'w':
			workdir_flag = optarg;
			break;
		case 'c':
			ctl_path = optarg;
			break;
		case 's':
			syslog_path = optarg;
			break;
		case 'P':
			pid_path = optarg;
			break;
		default:
			usage();
		}
	}
	if (argc != optind)
		usage();
	wd = cbsdlog_workdir(workdir_flag);
	set_default_path(&ctl_path, ctl_buf, sizeof(ctl_buf), wd, CBSDLOG_REL_CTL);
	set_default_path(&pid_path, pid_buf, sizeof(pid_buf), wd, CBSDLOG_REL_PID);
	set_default_path(&syslog_path, syslog_buf, sizeof(syslog_buf), wd,
	    CBSDLOG_REL_SYSLOG);

	for (i = 0; i < MAX_CLIENTS; i++) {
		clients[i].fd = -1;
		clients[i].follow = -1;
		clients[i].fds[0] = clients[i].fds[1] = -1;
	}

	set_signals();
	openlog("cbsdlogd", LOG_PID | LOG_NDELAY, LOG_DAEMON);
	lock_pid();
	listen_ctl();
	if (!foreground) {
		if (daemonize() < 0)
			err(1, "daemonize");
	} else {
		fprintf(stderr, "cbsdlogd: listening on %s, syslog %s\n",
		    ctl_path, syslog_path);
	}
	/*
	 * The watch queue is created after fork(2) so the daemon owns it.
	 * On Linux, inotify is used for the same purpose: watching the
	 * socket file names for removal.
	 */
#ifdef __linux__
	kq_fd = inotify_init1(IN_CLOEXEC);
	if (kq_fd < 0)
		err(1, "inotify_init");
#else
	/*
	 * FreeBSD does not copy a kqueue into the child. The index stays
	 * closed, poll() returns POLLNVAL immediately, and kevent() is EBADF.
	 */
	kq_fd = kqueue();
	if (kq_fd < 0)
		err(1, "kqueue");
#endif
	write_pid();
	loop();
	shutdown_all();
	return (0);
}
