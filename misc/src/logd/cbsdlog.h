#ifndef CBSDLOG_H
#define CBSDLOG_H

/*
 * Shared paths and helpers for cbsdlogd and cbsdlogctl.
 * The design lives in LOGGER.md at the root of the tree.
 */

#include <sys/socket.h>
#include <sys/un.h>

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Paths are relative to the CBSD workdir. */
#define CBSDLOG_REL_CTL		"var/run/cbsdlogd.ctl"
#define CBSDLOG_REL_PID		"var/run/cbsdlogd.pid"
#define CBSDLOG_REL_SYSLOG	"var/run/jail-syslog.sock"
#define CBSDLOG_REL_SYSLOG_PID	"var/run/jail-syslog.pid"
#define CBSDLOG_REL_LOG		"var/log/jails"

/*
 * The live etc/cbsdlog.conf is ${workdir}/etc/cbsdlog.conf. initenv expands
 * %%workdir%% there. Snippets are ${workdir}/etc/cbsdlog.d/<jail>.conf.
 */
#define CBSDLOG_REL_CONF	"etc/cbsdlog.conf"
#define CBSDLOG_REL_SNIPPET	"etc/cbsdlog.d"
#define CBSDLOG_DISTDIR_DEFAULT	"/usr/local/cbsd"

#define CBSDLOG_NAME_MAX	64
#define CBSDLOG_RC_CONF		"/etc/rc.conf"

/*
 * One assignment of key= on an rc.conf line. Returns 1 and writes the value,
 * 0 if this line is not that key, -1 if the value does not fit.
 */
static inline int
cbsdlog_rc_value(const char *line, const char *key, char *dst, size_t dstlen)
{
	const char *p = line;
	size_t klen = strlen(key);
	size_t n = 0;

	while (*p == ' ' || *p == '\t')
		p++;
	if (strncmp(p, "export", 6) == 0 && (p[6] == ' ' || p[6] == '\t')) {
		p += 6;
		while (*p == ' ' || *p == '\t')
			p++;
	}
	if (strncmp(p, key, klen) != 0 || p[klen] != '=')
		return (0);
	p += klen + 1;
	while (*p == ' ' || *p == '\t')
		p++;
	if (*p == '"' || *p == '\'') {
		char q = *p++;

		while (*p != '\0' && *p != q && *p != '\n' && *p != '\r') {
			if (*p == '\\' && q == '"' && p[1] != '\0')
				p++;
			if (n + 1 >= dstlen)
				return (-1);
			dst[n++] = *p++;
		}
	} else {
		while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '#' &&
		    *p != '\n' && *p != '\r') {
			if (n + 1 >= dstlen)
				return (-1);
			dst[n++] = *p++;
		}
	}
	dst[n] = '\0';
	return (n > 0 ? 1 : 0);
}

/* Last cbsd_workdir= in the file wins, same as sourcing it. */
static inline int
cbsdlog_rc_workdir_file(const char *path, char *dst, size_t dstlen)
{
	FILE *fp;
	char line[1024];
	char val[PATH_MAX];
	int found = 0;

	fp = fopen(path, "r");
	if (fp == NULL)
		return (-1);
	dst[0] = '\0';
	while (fgets(line, sizeof(line), fp) != NULL) {
		if (cbsdlog_rc_value(line, "cbsd_workdir", val, sizeof(val)) == 1 &&
		    val[0] == '/') {
			snprintf(dst, dstlen, "%s", val);
			found = 1;
		}
	}
	fclose(fp);
	return (found ? 0 : -1);
}

/*
 * -w, else $workdir, else $cbsd_workdir, else cbsd_workdir from /etc/rc.conf.
 * That is the order in cbsd.conf.
 */
static inline const char *
cbsdlog_workdir(const char *flag)
{
	static char rc_buf[PATH_MAX];
	static int rc_tried;
	const char *w;

	if (flag != NULL && flag[0] != '\0')
		return (flag);
	w = getenv("workdir");
	if (w != NULL && w[0] != '\0')
		return (w);
	w = getenv("cbsd_workdir");
	if (w != NULL && w[0] != '\0')
		return (w);
	if (!rc_tried) {
		rc_tried = 1;
		if (cbsdlog_rc_workdir_file(CBSDLOG_RC_CONF, rc_buf, sizeof(rc_buf)) != 0)
			rc_buf[0] = '\0';
	}
	if (rc_buf[0] != '\0')
		return (rc_buf);
	return (NULL);
}

/* $CIX_DISTDIR, else the default install prefix. Same default as cbsd.conf. */
static inline const char *
cbsdlog_distdir(void)
{
	const char *d;

	d = getenv("CIX_DISTDIR");
	if (d != NULL && d[0] == '/')
		return (d);
	return (CBSDLOG_DISTDIR_DEFAULT);
}

static inline int
cbsdlog_under(char *dst, size_t dstlen, const char *workdir, const char *rel)
{
	size_t n;
	int wrote;

	if (workdir == NULL || workdir[0] != '/' || rel == NULL || rel[0] == '\0')
		return (-1);
	n = strlen(workdir);
	while (n > 1 && workdir[n - 1] == '/')
		n--;
	wrote = snprintf(dst, dstlen, "%.*s/%s", (int)n, workdir, rel);
	if (wrote < 0 || (size_t)wrote >= dstlen)
		return (-1);
	return (0);
}

static inline int
cbsdlog_valid_jail(const char *name)
{
	size_t i, n;

	if (name == NULL)
		return (0);
	n = strlen(name);
	if (n == 0 || n >= CBSDLOG_NAME_MAX)
		return (0);
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return (0);
	for (i = 0; i < n; i++) {
		unsigned char c = (unsigned char)name[i];

		if (!(isalnum(c) || c == '.' || c == '_' || c == '-'))
			return (0);
	}
	return (1);
}

static inline int
cbsdlog_set_un(struct sockaddr_un *un, const char *path)
{
	if (strlen(path) >= sizeof(un->sun_path))
		return (-1);
	memset(un, 0, sizeof(*un));
	un->sun_family = AF_UNIX;
	strlcpy(un->sun_path, path, sizeof(un->sun_path));
#ifdef SUN_LEN
	un->sun_len = SUN_LEN(un);
#endif
	return (0);
}

static inline socklen_t
cbsdlog_un_len(const struct sockaddr_un *un)
{
#ifdef SUN_LEN
	return (SUN_LEN(un));
#else
	(void)un;
	return (sizeof(*un));
#endif
}

#endif
