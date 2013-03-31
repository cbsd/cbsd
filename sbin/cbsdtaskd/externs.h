/* reorder these #include's at your peril */

#if defined(SYSLOG)
# include <syslog.h>
#endif

#if defined(LOGIN_CAP)
# include <login_cap.h>
#endif /*LOGIN_CAP*/

#if defined(BSD_AUTH)
# include <bsd_auth.h>
#endif /*BSD_AUTH*/

#define DIR_T	struct dirent
#define WAIT_T	int
#define SIG_T	sig_t
#define TIME_T	time_t
#define PID_T	pid_t

#ifndef TZNAME_ALREADY_DEFINED
extern char *tzname[2];
#endif
#define TZONE(tm) tzname[(tm).tm_isdst]

#if (defined(BSD)) && (BSD >= 198606) || defined(__linux)
# define HAVE_FCHOWN
# define HAVE_FCHMOD
#endif

#if (defined(BSD)) && (BSD >= 199103) || defined(__linux)
# define HAVE_SAVED_UIDS
#endif

#define MY_UID(pw) getuid()
#define MY_GID(pw) getgid()

/* getopt() isn't part of POSIX.  some systems define it in <stdlib.h> anyway.
 * of those that do, some complain that our definition is different and some
 * do not.  to add to the misery and confusion, some systems define getopt()
 * in ways that we cannot predict or comprehend, yet do not define the adjunct
 * external variables needed for the interface.
 */
#if (!defined(BSD) || (BSD < 198911))
int	getopt(int, char * const *, const char *);
#endif

#if (!defined(BSD) || (BSD < 199103))
extern	char *optarg;
extern	int optind, opterr, optopt;
#endif

/* digital unix needs this but does not give us a way to identify it.
 */
extern	int		flock(int, int);

/* not all systems who provide flock() provide these definitions.
 */
#ifndef LOCK_SH
# define LOCK_SH 1
#endif
#ifndef LOCK_EX
# define LOCK_EX 2
#endif
#ifndef LOCK_NB
# define LOCK_NB 4
#endif
#ifndef LOCK_UN
# define LOCK_UN 8
#endif

#ifndef WCOREDUMP
# define WCOREDUMP(st)          (((st) & 0200) != 0)
#endif
