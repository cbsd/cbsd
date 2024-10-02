/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */

/*---------------------------------------------------------------------------*\
  Site-specific configuration file for run-as utility

  Input to `autoheader': $Id$
\*---------------------------------------------------------------------------*/

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <sys/types.h>


/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
#define _ALL_SOURCE 1
#endif

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if the `getpgrp' function takes no argument.  */
#define GETPGRP_VOID 1

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef gid_t */

/* Define if you don't have vprintf but do have _doprnt.  */
/* #undef HAVE_DOPRNT */

/* Define if you support file names longer than 14 characters.  */
#define HAVE_LONG_FILE_NAMES 1

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#define HAVE_SYS_WAIT_H 1

/* Define if you have <vfork.h>.  */
/* #undef HAVE_VFORK_H */

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef mode_t */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define if the `setpgrp' function takes no argument.  */
#define SETPGRP_VOID 1

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef uid_t */

/* Define vfork as fork if vfork does not work.  */
/* #undef vfork */

/* define if your C compiler lacks a built-in `bool' type */
#define bool short

/* define to `int' if your system lacks `ssize_t' */
/* #undef ssize_t */

/*
   Define if you have the `sig_t' type in <signal.h> or <sys/signal.h>
   (e.g., FreeBSD)
*/
#define HAVE_SIG_T 1

/*
  Define if your compiler supports a native "byte" type that holds at least
  eight bits.
*/
/* #undef HAVE_BYTE_TYPE */

/* Define if you have the getpgrp() function. */
#define HAVE_GETPGRP 1

/* Define if you have the setpgrp() function. */
#define HAVE_SETPGRP 1

/* Define if you have the setsid() function. Almost everyone does. */
#define HAVE_SETSID 1

/* Define if you have the daemon() function. */
#define HAVE_DAEMON 1

/* Define if you have the basename() function. */
#define HAVE_BASENAME 1

/* Define if you have <libgen.h> */
#define HAVE_LIBGEN_H 1

/* Define if you have the strerror() function. */
#define HAVE_STRERROR 1

/* Define if you have initgroups() */
#define HAVE_INITGROUPS 1

/* Define if you have vfork() */
#define HAVE_VFORK 1

/* Define if you have vprintf() */
#define HAVE_VPRINTF 1

/* Define if you have the `pw_comment' field in `struct passwd' (pwd.h) */
/* #undef HAVE_PW_COMMENT */

/* Define if you have the `pw_gecos' field in `struct passwd' (pwd.h) */
#define HAVE_PW_GECOS 1

/* Define to `int' if you don't have `mode_t' */
/* #undef mode_t */

/* Define to `int' if you don't have `pid_t' */
/* #undef pid_t */

/* Define to `int' if you don't have `gid_t' and `uid_t' */
/* #undef uid_t */
/* #undef gid_t */

/* Define to `unsigned' if you don't have `size_t' */
/* #undef size_t */

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H 1

/* Define if you have the setenv(3) function. */
#define HAVE_SETENV 1

/* Define if you have the flock(3) function. */
#define HAVE_FLOCK 1

/*****************************************************************************\
                     DON'T TOUCH ANYTHING BELOW HERE!
\*****************************************************************************/

#ifndef HAVE_BYTE_TYPE
typedef unsigned char	byte;		     /* 8 bit unsigned critter */
#endif /* HAVE_BYTE_TYPE */

#ifndef HAVE_SIG_T
typedef void __sighandler_t (int);
typedef __sighandler_t  *sig_t;
#endif /* HAVE_SIG_T */

#ifndef HAVE_FALSE
#define FALSE (0)
#endif /* FALSE */

#ifndef HAVE_TRUE
#define TRUE (1)
#endif /* TRUE */

#ifndef HAVE_SETSID
#define setsid() ((pid_t) -1)
#endif /* HAVE_SETSID */

#ifndef HAVE_SYSCONF
#define sysconf(name) ((long) -1)
#endif /* HAVE_SYSCONF */

#ifndef HAVE_INITGROUPS
#define initgroups(name, gid) (0)
#endif

#ifndef HAVE_STRERROR
extern char *strerror (int errnum);
#endif /* HAVE_STRERROR */

extern int x_getopt (int argc, char **argv, const char *opts);
extern int x_opterr;
extern int x_optind;
extern int x_optopt;
extern char *x_optarg;

#ifndef HAVE_DAEMON
extern int daemon (int nochdir, int noclose);
#endif

#ifdef HAVE_VFORK
#ifdef HAVE_VFORK_H
#include <vfork.h>
#endif
#else /* HAVE_VFORK */
/* #undef vfork */
#define vfork fork
#endif /* HAVE_VFORK */

#ifdef HAVE_ERRNO_H
#include <errno.h>
#else
extern int errno;
#endif

#ifndef HAVE_SETENV
extern int setenv(const char *name, const char *value, int overwrite);
#endif

#ifndef STDC_HEADERS
#error "Must have standard C headers. Sorry."
#endif

/*
  Password field: If only one of `pw_comment' and `pw_gecos' is present,
  define the missing one in terms of the existing one. If neither is
  present, define both in terms of `pw_name'.
*/
#if !defined(HAVE_PW_COMMENT) && !defined(HAVE_PW_GECOS)
#define pw_comment pw_name
#define pw_gecos   pw_name
#else
#if defined(HAVE_PW_COMMENT) && !defined(HAVE_PW_GECOS)
#define pw_gecos pw_comment
#else
#if defined(HAVE_PW_GECOS) && !defined(HAVE_PW_COMMENT)
#define pw_comment pw_gecos
#endif
#endif
#endif

#include <sys/types.h>
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#ifndef HAVE_BASENAME
extern char *basename (char *path);
#endif

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#ifndef HAVE_FLOCK
#include "flock.h"
#endif

#endif /* _CONFIG_H_ */
