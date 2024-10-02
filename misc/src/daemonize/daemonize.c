/*---------------------------------------------------------------------------*\
  NAME
        daemonize - run a command as a Unix daemon

  DESCRIPTION

        See accompanying man page for full details.

  LICENSE

        This source code is released under a BSD-style license. See the
        LICENSE file for details.

  Copyright (c) 2003-2010 Brian M. Clapper, bmc@clapper.org
\*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "config.h"
#include "version.h"

/*---------------------------------------------------------------------------*\
                                  Globals
\*---------------------------------------------------------------------------*/

static const char  *pid_file   = NULL;
static const char  *out_file   = NULL;
static const char  *err_file   = NULL;
static const char  *lock_file  = NULL;
static bool         be_verbose = FALSE;
static const char  *user       = NULL;
static char       **cmd        = NULL;
static const char  *cwd        = "/";
static int          null_fd    = -1;
static int          out_fd     = -1;
static int          err_fd     = -1;
static int          append     = 0;

/*---------------------------------------------------------------------------*\
                             Private Functions
\*---------------------------------------------------------------------------*/

/**
 * Akin to perl's die() function, this function prints the specified message
 * and exits the program with a non-zero error code.
 *
 * Parameters:
 *     format - a printf format string
 *     ...    - any additional arguments as necessary to satisfy the format
 */
static void die(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    exit(1);
}

/**
 * Emits a message to standard output if the be_verbose flag is set.
 *
 * Parameters:
 *     format - a printf format string
 *     ...    - any additional arguments as necessary to satisfy the format
 */
static void verbose(const char *format, ...)
{
    va_list ap;

    if (be_verbose)
    {
        va_start(ap, format);
        vfprintf(stdout, format, ap);
        va_end(ap);
    }
}

/**
 * Emit the usage message and abort the program with a non-zero exit code.
 *
 * Parameters:
 *     prog - name of program, from argv[0]
 */
static void usage(char *prog)
{
    static const char *USAGE[] = 
    {
"Usage: %s [OPTIONS] path [arg] ...",
"",
"OPTIONS",
"",
"-a             Append to, instead of overwriting, output files. Ignored ",
"               unless -e and/or -o are specified.",
"-c <dir>       Set daemon's working directory to <dir>.",
"-e <stderr>    Send daemon's stderr to file <stderr>, instead of /dev/null.",
"-E var=value   Pass environment setting to daemon. May appear multiple times.",
"-o <stdout>    Send daemon's stdout to file <stdout>, instead of /dev/null.",
"-p <pidfile>   Save PID to <pidfile>.",
"-u <user>      Run daemon as user <user>. Requires invocation as root.",
"-l <lockfile>  Single-instance checking using lockfile <lockfile>.",
"-v             Issue verbose messages to stdout while daemonizing."
    };

    int i;

    prog = basename(prog);
    fprintf(stderr, "%s, version %s\n", prog, VERSION);
    for (i = 0; i < sizeof(USAGE) / sizeof(const char *); i++)
    {
        fprintf(stderr, USAGE[i], prog);
        fputc('\n', stderr);
    }

    exit(1);
}

/**
 * Add a string of the name "name=value" to the environment. Aborts on error.
 *
 * Parameters:
 *     opt    - option character, for errors
 *     envvar - string of the form name=value
 */
static void add_to_env(char opt, const char *envvar)
{
    char *eq = strchr(envvar, '=');
    if (eq == NULL)
        die("Argument to -%c (\"%s\") is not of the form name=value.\n",
            opt, envvar);

    /* Split the string into its name/value substrings. */

    int name_len = (int) (eq - envvar);
    int val_len  = (strlen(envvar) - name_len) - 1;

    char *name = (char *) malloc(name_len + 1);
    char *value = (char *) malloc(val_len + 1);
    *name = '\0';
    *value = '\0';
    (void) strncat(name, envvar, name_len);
    eq++;
    (void) strncat(value, eq, val_len);
    setenv(name, value, 1);
    free(name);
    free(value);
}

/**
 * Parse the command-line parameters, setting the various globals that are
 * affected by them.
 *
 * Parameters:
 *     argc - argument count, as passed to main()
 *     argv - argument vector, as passed to main()
 */
static void parse_params(int argc, char **argv)
{
    int  opt;
    int  argsLeft;

    opterr = 0;

    /*
      NOTE: x_getopt() is the old public domain getopt(). The source lives
      in "getopt.c". The function's name has been changed to avoid
      conflicts with the native getopt() on the host operating system. So
      far, I've seen two kinds of conflicts:

      1. GNU getopt() (e.g., on Linux systems) behaves differently from the
         old getopt(), unless POSIXLY_CORRECT is defined in the
         environment. Specifically, it insists on processing options even
         after the first non-option argument has been seen on the command
         line. Initially, I got around this problem by forcing the use of
         the included public domain getopt() function.

      2. The types used in the included public domain getopt() conflict with
         the types of the native getopt() on some operating systems (e.g.,
         Solaris 8).

      Using x_getopt() ensures that daemonize uses its own version, which
      always behaves consistently.
    */
    while ( (opt = x_getopt(argc, argv, "ac:u:p:vo:e:E:l:")) != -1)
    {
        switch (opt)
        {
            case 'a':
                append = 1;
                break;

            case 'c':
                cwd = x_optarg;
                break;

            case 'p':
                pid_file = x_optarg;
                break;

            case 'v':
                be_verbose = TRUE;
                break;

            case 'u':
                user = x_optarg;
                break;

            case 'o':
                out_file = x_optarg;
                break;

            case 'e':
                err_file = x_optarg;
                break;

            case 'l':
                lock_file = x_optarg;
                break;

            case 'E':
                add_to_env('E', x_optarg);
                break;

            default:
                fprintf(stderr, "Bad option: -%c\n", x_optopt);
                usage(argv[0]);
        }
    }

    argsLeft = argc - x_optind;
    if (argsLeft < 1)
        usage(argv[0]);

    cmd  = &argv[x_optind];
    return;
}

/**
 * Switch the process's effective and real user IDs to the ID associated with
 * the specified user name. Also switches to that user's group ID to the
 * primary group associated with the user.
 *
 * This function calls die() if anything fails.
 *
 * Parameters:
 *     user_name - name of user to which to switch
 *     uid       - current process user ID
 *     pid_file  - if non-NULL, specifies PID file path, ownership of which
 *                 should be changed to the user
 */
static void switch_user(const char *user_name, uid_t uid, const char *pid_file)
{
    struct  passwd *pw;

    if (uid != 0)
        die("Must be root to specify a different user.\n");

    if ( (pw = getpwnam(user_name)) == NULL )
        die("Can't find user \"%s\" in password file.\n", user_name);

    if (setgid(pw->pw_gid) != 0)
        die("Can't set gid to %d: %s\n", pw->pw_gid, strerror (errno));

    /*
      For systems supporting multiple group memberships, make sure ALL
      groups are added to the process, just in case someone depends on them.

      Patch by Ken Farnen <kenf@14Cubed.COM>, 18 February 2010. Modified
      to put the #ifdef in the config.h header, rather than in here.
    */
    if (initgroups(pw->pw_name, pw->pw_gid) == -1)
        die("Can't initialize secondary groups for \"%s\": %s\n",
            pw->pw_name, strerror (errno));

    if (pid_file != NULL)
    {
        verbose("Changing ownership of PID file to \"%s\" (%d)\n",
                user, pw->pw_uid);
        if (chown(pid_file, pw->pw_uid, pw->pw_gid) == -1)
        {
            die("Can't change owner of PID file \"%s\" to \"%s\" (%d): %s\n",
                pid_file, user, pw->pw_uid, strerror (errno));
        }
    }

    if (setegid(pw->pw_gid) != 0)
        die("Can't set egid to %d: %s\n", pw->pw_gid, strerror (errno));

    if (setuid(pw->pw_uid) != 0)
        die("Can't set uid to %d: %s\n", pw->pw_uid, strerror (errno));

    if (seteuid(pw->pw_uid) != 0)
        die("Can't set euid to %d: %s\n", pw->pw_uid, strerror (errno));

    /*
      Initialize environment to match new username.
      Patch by Ken Farnen <kenf@14Cubed.COM>, 18 February 2010.
    */
    setenv("USER", pw->pw_name,1);
    setenv("LOGNAME", pw->pw_name,1);
    setenv("HOME", pw->pw_dir,1);
}

/**
 * Open the specified output file. If the global "append" variable is set,
 * the file is opened in append mode; otherwise, it is overwritten.
 *
 * Parameters:
 *     path - path to the output file
 *
 * Returns:
 *     the file descriptor for the open file, or -1 on error.
 */
static int open_output_file(const char *path)
{
    int flags = O_CREAT | O_WRONLY;

    if (append)
    {
        verbose("Appending to %s\n", path);
        flags |= O_APPEND;
    }

    else
    {
        verbose("Overwriting %s\n", path);
        flags |= O_TRUNC;
    }

    return open(path, flags, 0666);
}

/**
 * Opens all output files.
 */
static void open_output_files()
{
    /* open files for stdout/stderr */

    if ((out_file != NULL) || (err_file != NULL))
    {
        if ((null_fd = open("/dev/null", O_WRONLY)) == -1)
            die("Can't open /dev/null: %s\n", strerror (errno));

        close(STDIN_FILENO);
        dup2(null_fd, STDIN_FILENO);

        if (out_file != NULL)
        {
            if ((out_fd = open_output_file(out_file)) == -1)
            {
                die("Can't open \"%s\" for stdout: %s\n", 
                    out_file, strerror(errno));
            }
        }

        else 
        {
            out_fd = null_fd;
        }

        if (err_file != NULL)
        {
            if ((out_file != NULL) && (strcmp(err_file, out_file) == 0))
                err_fd = out_fd;

            else if ((err_fd = open_output_file(err_file)) == -1)
            {
                die("Can't open \"%s\" for stderr: %s\n", 
                    err_file, strerror (errno));
            }
        }

        else
        {
            err_fd = null_fd;
        }
    }
}

/**
 * Redirects standard output and error to someplace safe.
 */
static int redirect_stdout_stderr()
{
    int rc = 0;

    /* Redirect stderr/stdout */

    if ((out_file != NULL) || (err_file != NULL))
    {
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        dup2(null_fd, STDIN_FILENO);
        dup2(out_fd, STDOUT_FILENO);
        dup2(err_fd, STDERR_FILENO);
        rc = 1;
    }

    return rc;
}

/*---------------------------------------------------------------------------*\
                               Main Program
\*---------------------------------------------------------------------------*/

int main(int argc, char **argv)
{
    uid_t  uid = getuid();
    int    noclose = 0;
    int    lockFD;
    struct stat st;

    if (geteuid() != uid)
        die("This executable is too dangerous to be setuid.\n");

    parse_params(argc, argv);

    if (cmd[0][0] != '/')
        die("The 'path' parameter must be an absolute path name.\n");

    /* Verify that the path to the command points to an existing file. */

    if (access(cmd[0], X_OK) == -1)
        die("File \"%s\" is not executable.\n", cmd[0]);

    /*
      Note: A directory will also pass the X_OK test, so test further with
      stat().
    */
    if (stat(cmd[0], &st))
        die("File \"%s\" does not exist.\n", cmd[0]);

    if(! S_ISREG(st.st_mode))
        die("File \"%s\" is not regular file.\n", cmd[0]);

    if (lock_file)
    {
        lockFD = open(lock_file, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
        if (lockFD < 0)
            die("Can't create lock file \"%s\": %s\n",
                lock_file, strerror (errno));
        if (flock(lockFD, LOCK_EX | LOCK_NB) != 0)
            die("Can't lock the lock file \"%s\". "
                "Is another instance running?\n",
                lock_file);
    }

    if (pid_file != NULL)
    {
        int fd;

        verbose("Creating PID file \"%s\".\n", pid_file);
        fd = open(pid_file, O_CREAT | O_WRONLY,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd < 0)
        {
            die ("Can't create PID file \"%s\": %s\n",
                 pid_file, strerror (errno));
        }

        close(fd);
    }

    if (user != NULL)
        switch_user(user, uid, pid_file);

    open_output_files();

    if (chdir (cwd) != 0)
    {
        die("Can't change working directory to \"%s\": %s\n",
            cwd, strerror (errno));
    }

    verbose("Daemonizing...");

    if (redirect_stdout_stderr())
        noclose = 1;

    if (daemon (1, noclose) != 0)
        die("Can't daemonize: %s\n", strerror (errno));

    if (chdir(cwd) != 0)
    {
        die("Can't change working directory to \"%s\": %s\n",
            cwd, strerror (errno));
    }

    if (pid_file != NULL)
    {
        FILE  *fPid = NULL;

        verbose("Writing process ID to \"%s\".\n", pid_file);
        if ( (fPid = fopen (pid_file, "w")) == NULL )
        {
            die("Can't open PID file \"%s\": %s\n",
                pid_file, strerror (errno));
        }

        fprintf(fPid, "%d\n", getpid());
        fclose(fPid);
    }

    /*
      Make sure we have a relatively sane environment
      Patch by Ken Farnen <kenf@14Cubed.COM>, 18 February 2010.
    */
    if (getenv("IFS") == NULL)
        setenv("IFS"," \t\n",1);

    if (getenv("PATH") == NULL)
        setenv("PATH","/usr/local/sbin:/sbin:/bin:/usr/sbin:/usr/bin", 1);

    execvp(cmd[0], cmd);

    die("Can't exec \"%s\": %s\n", cmd[0], strerror (errno));
}

/*  vim: set et sw=4 sts=4 : */

