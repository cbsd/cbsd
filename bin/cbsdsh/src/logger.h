// CBSD Project, 2018
// Syslog message maximum length generally defaults to 1024 bytes (older RFC 3164)
// RFC 5424 (Modern): Recommends systems handle at least 2048 bytes
#define LOG_MAX_LEN 1024 /* Default maximum length of syslog messages */

/* Log levels */
#define LL_DEBUG 0
#define LL_VERBOSE 1
#define LL_NOTICE 2
#define LL_WARNING 3
#define LL_RAW (1 << 10) /* Modifier to log without timestamp */
#define CONFIG_DEFAULT_VERBOSITY LL_NOTICE

/* Logging */
extern char *syslog_ident;  /* Syslog ident */
extern int syslog_facility; /* Syslog facility */

extern int verbosity;       /* Default Loglevel */
extern int syslog_enabled;  /* Is syslog enabled? */

extern char *cbsd_logfile;  /* CBSD logfile */

int cbsdloggercmd(int, char **);
void cbsdlog(int level, const char *fmt, ...);
