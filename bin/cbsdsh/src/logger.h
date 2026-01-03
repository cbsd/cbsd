// CBSD Project, 2018
// olevole@olevole.ru
#define LOG_MAX_LEN 1024 /* Default maximum length of syslog messages */

/* Log levels */
#define LL_DEBUG 0
#define LL_VERBOSE 1
#define LL_NOTICE 2
#define LL_WARNING 3
#define LL_RAW (1 << 10) /* Modifier to log without timestamp */
#define CONFIG_DEFAULT_VERBOSITY LL_NOTICE

/* Logging */
char *syslog_ident;  /* Syslog ident */
int syslog_facility; /* Syslog facility */

int verbosity = LL_NOTICE; /* Default Loglevel */
int syslog_enabled = 1;	   /* Is syslog enabled? */

char *cbsd_logfile = NULL; /* CBSD logfile */

int cbsdloggercmd(int, char **);
void cbsdlog(int level, const char *fmt, ...);
