// CBSD Project, 2018
// olevole@olevole.ru
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "input.h"
#include "logger.h"
#include "output.h"
#include "var.h"

/* Low level logging. To use only for very big messages, otherwise
 * serverLog() is to prefer.
 */
void
serverLogRaw(int level, const char *msg)
{
	const int syslogLevelMap[] = { LOG_DEBUG, LOG_INFO, LOG_NOTICE,
		LOG_WARNING };
	const char *c = ".-*#";
	FILE *fp;
	char buf[64];
	int rawmode = (level & LL_RAW);

	level &= 0xff; /* clear flags */

	if (level < verbosity)
		return;

	if ((cbsd_logfile != NULL) && (strlen(cbsd_logfile) > 1)) {
		fp = fopen(cbsd_logfile, "a");
		if (!fp)
			return;

		if (rawmode) {
			fprintf(fp, "%s", msg);
		} else {
			int off;
			struct timeval tv;
			// pid_t pid = getpid();

			gettimeofday(&tv, NULL);
			off = strftime(buf, sizeof(buf), "%d %b %H:%M:%S.",
			    localtime(&tv.tv_sec));
			snprintf(buf + off, sizeof(buf) - off, "%03d",
			    (int)tv.tv_usec / 1000);
			fprintf(fp, "%d:%s %c %s\n", (int)getpid(), buf,
			    c[level], msg);
		}
		fflush(fp);
		fclose(fp);
	}

	if (syslog_enabled) {
		syslog(syslogLevelMap[level], "%s", msg);
	}
}

int
init_logvars()
{
	char *cbsd_syslog_enabled = NULL;
	char *cbsd_syslog_verbosity = NULL;

	cbsd_syslog_enabled = lookupvar("CBSD_SYSLOG_ENABLED");
	cbsd_syslog_verbosity = lookupvar("CBSD_SYSLOG_VERBOSITY");

	if (cbsd_syslog_enabled) {
		syslog_enabled = atoi(cbsd_syslog_enabled);
	}

	if (cbsd_syslog_verbosity) {
		if (!strcmp("DEBUG", cbsd_syslog_verbosity))
			verbosity = LL_DEBUG;
		else if (!strcmp("VERBOSE", cbsd_syslog_verbosity))
			verbosity = LL_VERBOSE;
		else if (!strcmp("NOTICE", cbsd_syslog_verbosity))
			verbosity = LL_NOTICE;
		else if (!strcmp("WARNING", cbsd_syslog_verbosity))
			verbosity = LL_WARNING;
		else {
			out1fmt(
			    "Unknown verbosity: %s. Should be: [DEBUG|VERBOSE|NOTICE|WARNING]\n",
			    cbsd_syslog_verbosity);
			return 1;
		}
	}

	cbsd_logfile = lookupvar("CBSD_LOGFILE");

	if ((!syslog_enabled) && (strlen(cbsd_logfile) < 2))
		return 1;

	return 0;
}

/* Like serverLogRaw() but with printf-alike support. This is the function that
 * is used across the code. The raw version is only used in order to dump
 * the INFO output on crash.
 */
int
cbsdloggercmd(int argc, char **argv)
{
	int level = 0;
	char msg[LOG_MAX_LEN];
	int i;
	int res = 0;

	i = init_logvars();
	if (i != 0)
		return 1;

	if (argc < 3) {
		out1fmt("cbsdlogger [DEBUG|VERBOSE|NOTICE|WARNING] msg\n");
		return 1;
	}

	if (!strcmp("DEBUG", argv[1]))
		level = LL_DEBUG;
	else if (!strcmp("VERBOSE", argv[1]))
		level = LL_VERBOSE;
	else if (!strcmp("NOTICE", argv[1]))
		level = LL_NOTICE;
	else if (!strcmp("WARNING", argv[1]))
		level = LL_WARNING;
	else {
		out1fmt("cbsdlogger [DEBUG|VERBOSE|NOTICE|WARNING] msg\n");
		return 1;
	}

	if ((level & 0xff) < verbosity)
		return 0;

	for (i = 2; i < argc; i++)
		res += strlen(argv[i]) + 1;
	if (res) {
		memset(msg, 0, sizeof(msg));
		for (i = 2; i < argc; i++) {
			strcat(msg, argv[i]);
			strcat(msg, " ");
		}
	}

	serverLogRaw(level, msg);
	return 0;
}

void
cbsdlog(int level, const char *fmt, ...)
{
	va_list ap;
	char msg[LOG_MAX_LEN];
	int i;

	i = init_logvars();
	if (i != 0)
		return;

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	serverLogRaw(level, msg);
}
