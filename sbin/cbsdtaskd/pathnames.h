#define _PATHNAMES_H_

#include <paths.h>

#define CRONDIR		"/var/cron"
#define SPOOL_DIR	"tabs"

#define	CRON_ALLOW	"cron.allow"
#define	CRON_DENY	"cron.deny"

#define LOG_FILE	"log"

#define PIDFILE		"cbsdcron.pid"
#define PIDDIR	"/var/run"

			/* 4.3BSD-style crontab */
#define SYSCRONTAB	"/etc/crontab"

			/* what editor to use if no EDITOR or VISUAL
			 * environment variable specified.
			 */
#define EDITOR "/usr/bin/vi"

#define _PATH_DEFPATH "/usr/bin:/bin"

#define _PATH_DEVNULL "/dev/null"

