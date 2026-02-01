AC_INIT([dash],[0.5.13])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_SRCDIR([src/main.c])

AC_CONFIG_HEADERS(config.h)

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES(yes)])

dnl Checks for programs.
AC_PROG_CC
AC_USE_SYSTEM_EXTENSIONS

AC_MSG_CHECKING([for build system compiler])
if test "$cross_compiling" = yes; then
	CC_FOR_BUILD=${CC_FOR_BUILD-cc}
else
	CC_FOR_BUILD=${CC}
fi
AC_MSG_RESULT(${CC_FOR_BUILD})
AC_SUBST(CC_FOR_BUILD)

AC_MSG_CHECKING([for __attribute__((__alias__()))])
dash_cv_have_attribute_alias=no
AC_LINK_IFELSE([AC_LANG_PROGRAM([void t() {}
                                 void a() __attribute__((__alias__("t")));],
                                [a();])],
               [dash_cv_have_attribute_alias=yes])
AC_MSG_RESULT($dash_cv_have_attribute_alias)
if test "x$dash_cv_have_attribute_alias" = xyes; then
  AC_DEFINE([HAVE_ALIAS_ATTRIBUTE], 1,
            [Define if __attribute__((__alias__())) is supported])
fi

AC_ARG_ENABLE(static, AS_HELP_STRING(--enable-static, \
				     [Build statical linked program]))
if test "$enable_static" = "yes"; then
	export LDFLAGS="-static"
fi

AC_ARG_ENABLE(fnmatch, AS_HELP_STRING(--enable-fnmatch, \
				      [Use fnmatch(3) from libc]))
AC_ARG_ENABLE(glob, AS_HELP_STRING(--enable-glob, [Use glob(3) from libc]))

dnl Checks for libraries.

dnl Checks for header files.
AC_CHECK_HEADERS(alloca.h paths.h sys/wait.h)

dnl Check for declarations
AC_CHECK_DECL([_PATH_BSHELL],,AC_DEFINE_UNQUOTED([_PATH_BSHELL], "/bin/sh", [Define to system shell path]),[
#ifdef HAVE_PATHS_H
#include <paths.h>
#endif
])
AC_CHECK_DECL([_PATH_DEVNULL],,AC_DEFINE_UNQUOTED([_PATH_DEVNULL], "/dev/null", [Define to devnull device node path]),[
#ifdef HAVE_PATHS_H
#include <paths.h>
#endif
])
AC_CHECK_DECL([_PATH_TTY],,AC_DEFINE_UNQUOTED([_PATH_TTY], "/dev/tty", [Define to tty device node path]),[
#ifdef HAVE_PATHS_H
#include <paths.h>
#endif
])

dnl Some systems lack isblank
AC_CHECK_DECLS([isblank],,,[#include <ctype.h>])

dnl Check for sizes of types
AC_CHECK_SIZEOF([intmax_t])
AC_CHECK_SIZEOF([long long int])

dnl Select a fallback format string for intmax_t in case we don't find PRIdMAX
if test "x$ac_cv_sizeof_intmax_t" = "x$ac_cv_sizeof_long_long_int"; then
  intmax_fstr="lld"
else
  intmax_fstr="jd"
fi

dnl Check for PRIdMAX and define it to a fallback if not found
AC_CHECK_DECL([PRIdMAX],,
	[AC_DEFINE_UNQUOTED([PRIdMAX], "$intmax_fstr",
				       [Define to printf format string for intmax_t])],
        [
#include <inttypes.h>
])

dnl Checks for library functions.
AC_CHECK_FUNCS(bsearch faccessat getpwnam getrlimit isalpha killpg \
	       memfd_create memrchr mempcpy \
	       sigsetmask stpcpy strchrnul strsignal strtod strtoimax \
	       strtoumax sysconf tee wait3)

dnl Check whether it's worth working around FreeBSD PR kern/125009.
dnl The traditional behavior of access/faccessat is crazy, but
dnl POSIX.1-2008 explicitly allows those functions to misbehave.
dnl
dnl Unaffected kernels:
dnl
dnl - all versions of Linux
dnl - NetBSD sys/kern/vfs_subr.c 1.64, 1997-04-23
dnl - FreeBSD 9 (r212002), 2010-09-10
dnl - OpenBSD sys/kern/vfs_subr.c 1.166, 2008-06-09
dnl
dnl Also worked around in Debian's libc0.1 2.13-19 when using
dnl kFreeBSD 8.

AC_ARG_ENABLE(test-workaround, AS_HELP_STRING(--enable-test-workaround, \
	[Guard against faccessat(2) that tells root all files are executable]),,
	[enable_test_workaround=auto])

if test "enable_test_workaround" = "auto" &&
   test "$ac_cv_func_faccessat" = yes; then
	case `uname -s 2>/dev/null` in
	GNU/kFreeBSD | \
	FreeBSD)
		enable_test_workaround=yes
	esac
fi
if test "$enable_test_workaround" = "yes"; then
	AC_DEFINE([HAVE_TRADITIONAL_FACCESSAT], [1],
		[Define if your faccessat tells root all files are executable])
fi

if test "$enable_fnmatch" = yes; then
	use_fnmatch=
	AC_CHECK_FUNCS(fnmatch, use_fnmatch=yes)
fi

if test "$use_fnmatch" = yes && test "$enable_glob" = yes; then
	AC_CHECK_FUNCS(glob)
fi

dnl Check for klibc signal.
AC_CHECK_FUNC(signal)
if test "$ac_cv_func_signal" != yes; then
	AC_CHECK_FUNC(bsd_signal,
		      [AC_DEFINE(signal, bsd_signal,
				 [klibc has bsd_signal instead of signal])])
fi

dnl Check for stat64 (dietlibc/klibc).
dnl On FreeBSD and some other systems, stat64 doesn't exist as a separate function
dnl We check if the function exists by trying to link it, and if not, create aliases.
dnl Only apply the stat64->stat defines on BSD; on Linux (glibc) they cause
dnl "redefinition of struct stat" when system headers are included (e.g. sqlite3.h).
AC_MSG_CHECKING([for stat64 function])
AC_LINK_IFELSE([AC_LANG_PROGRAM([
#include <sys/stat.h>
], [
	struct stat st;
	if (stat64("/", &st) != 0) return 1;
])], [
	AC_MSG_RESULT([yes])
], [
	AC_MSG_RESULT([no])
	case `uname -s 2>/dev/null` in
	FreeBSD|DragonFly|OpenBSD|NetBSD)
		AC_DEFINE(fstat64, fstat, [64-bit operations are the same as 32-bit])
		AC_DEFINE(lstat64, lstat, [64-bit operations are the same as 32-bit])
		AC_DEFINE(stat64, stat, [64-bit operations are the same as 32-bit])
		;;
	esac
])

AC_CHECK_FUNC(glob64,, [
	AC_DEFINE(glob64_t, glob_t, [64-bit operations are the same as 32-bit])
	AC_DEFINE(glob64, glob, [64-bit operations are the same as 32-bit])
	AC_DEFINE(globfree64, globfree,
		  [64-bit operations are the same as 32-bit])
])

dnl OS X apparently has stat64 but not open64.
AC_CHECK_FUNC(open64,, [
	AC_DEFINE(open64, open, [64-bit operations are the same as 32-bit])
	AC_DEFINE(readdir64, readdir,
		  [64-bit operations are the same as 32-bit])
	AC_DEFINE(dirent64, dirent,
		  [64-bit operations are the same as 32-bit])
])

dnl Check if struct stat has st_mtim.
AC_MSG_CHECKING(for stat::st_mtim)
AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM([#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>],
[struct stat foo; return sizeof(foo.st_mtim.tv_sec)])],
have_st_mtim=yes, have_st_mtim=no)
AC_MSG_RESULT($have_st_mtim)
if test "$have_st_mtim" = "yes"; then
	AC_DEFINE([HAVE_ST_MTIM], [1],
		[Define if your `struct stat' has `st_mtim'])
fi

dnl F_DUPFD_CLOEXEC is a mandatory part of POSIX since Issue 7
AC_MSG_CHECKING(for F_DUPFD_CLOEXEC)
AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM([#include <unistd.h>
#include <fcntl.h>],
[return fcntl(0, F_DUPFD_CLOEXEC, 0)])],
have_dupfd_cloexec=1, have_dupfd_cloexec=0)
AC_MSG_RESULT($(expr yes \& $have_dupfd_cloexec \| no))
AC_DEFINE_UNQUOTED([HAVE_F_DUPFD_CLOEXEC], [$have_dupfd_cloexec],
	[Define to 1 your system supports F_DUPFD_CLOEXEC])

AC_ARG_WITH(libedit, AS_HELP_STRING(--with-libedit, [Compile with libedit support]))
use_libedit=
if test "$with_libedit" = "yes"; then
	AC_CHECK_LIB(edit, history_init, [
		AC_CHECK_HEADER([histedit.h], [use_libedit="yes"],
				AC_MSG_ERROR(
					[Can't find required header files.]))], [
		AC_MSG_ERROR([Can't find libedit.])])
fi
if test "$use_libedit" != "yes"; then
	AC_DEFINE([SMALL], 1, [Define if you build with -DSMALL])
else
	export LIBS="$LIBS -ledit"
fi
AC_ARG_WITH(libsqlite3, AS_HELP_STRING(--with-libsqlite3, [Compile with libsqlite3 support]))
use_libsqlite3=
if test "$with_libsqlite3" = "yes"; then
	AC_CHECK_LIB(sqlite3, sqlite3_initialize, [
		AC_CHECK_HEADER([sqlite3.h], [use_libsqlite3="yes"],
				AC_MSG_ERROR(
					[Can't find required header files.]))], [
		AC_MSG_ERROR([Can't find libsqlite3.])])
fi
if test "$use_libsqlite3" != "yes"; then
	AC_DEFINE([SMALL], 1, [Define if you build with -DSMALL])
else
	export LIBS="$LIBS -lsqlite3"
fi
AC_ARG_WITH(libjail, AS_HELP_STRING(--with-libjail, [Compile with libjail support]))
use_libjail=
if test "$with_libjail" = "yes"; then
	AC_CHECK_LIB(jail, jail_getid, [
		AC_CHECK_HEADER([jail.h], [use_libjail="yes"],
				AC_MSG_ERROR(
					[Can't find required header files.]))], [
		AC_MSG_ERROR([Can't find libjail.])])
fi
if test "$use_libjail" != "yes"; then
	AC_DEFINE([SMALL], 1, [Define if you build with -DSMALL])
else
	export LIBS="$LIBS -ljail"
fi
AM_CONDITIONAL([USE_LIBJAIL], [test "$use_libjail" = "yes"])

export LIBS="$LIBS -pthread"

dnl Detect platform for cbsd_fwatch (use CBSD_PLATFORM from build script if set)
AC_MSG_CHECKING([for operating system])
if test -n "${CBSD_PLATFORM}"; then
	case "${CBSD_PLATFORM}" in
		Linux)
			platform_fwatch="linux"
			;;
		FreeBSD|DragonFly|OpenBSD|NetBSD)
			platform_fwatch="freebsd"
			;;
		*)
			AC_MSG_WARN([Unknown CBSD_PLATFORM "${CBSD_PLATFORM}", defaulting to linux])
			platform_fwatch="linux"
			;;
	esac
else
	case `uname -s 2>/dev/null` in
		FreeBSD|DragonFly|OpenBSD|NetBSD)
			platform_fwatch="freebsd"
			;;
		Linux)
			platform_fwatch="linux"
			;;
		*)
			AC_MSG_WARN([Unknown OS, defaulting to Linux])
			platform_fwatch="linux"
			;;
	esac
fi
AC_MSG_RESULT([$platform_fwatch])
AM_CONDITIONAL([USE_FWATCH_LINUX], [test "$platform_fwatch" = "linux"])
AM_CONDITIONAL([USE_FWATCH_FREEBSD], [test "$platform_fwatch" = "freebsd"])

AC_ARG_ENABLE(lineno, AS_HELP_STRING(--disable-lineno, \
				     [Disable LINENO support]))
if test "$enable_lineno" != "no"; then
	AC_DEFINE([WITH_LINENO], 1, [Define if you build with -DWITH_LINENO])
fi
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
