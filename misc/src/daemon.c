// CBSD Project
// Special version of daemon which execute the process but displays output data from it
// only if they do not start with a dot. This is necessary primarily for running on a
// remote server for cbsd_dot who writes in output except dots fact updates lokal.sqlite
// This is a temporary solution to send any signal when inventory of node is update.
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/wait.h>

#include "namespace.h"
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>

#include <libutil.h>
#include <login_cap.h>
#include <pwd.h>

#include <sys/stat.h>

#include "un-namespace.h"

#ifdef __DragonFly__
// sys/mman.h:
#define      MADV_PROTECT    10      /* protect process from pageout kill */
#endif

static void dummy_sighandler(int);
static void restrict_process(const char *);
static int  wait_child(pid_t pid, sigset_t *mask);
static void usage(void);
int touch(char *);

int
daemon(nochdir, noclose)
	int nochdir, noclose;
{
	struct sigaction osa, sa;
	int fd;
	pid_t newgrp;
	int oerrno;
	int osa_ok;

	/* A SIGHUP may be thrown when the parent exits below. */
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	osa_ok = _sigaction(SIGHUP, &sa, &osa);

	switch (fork()) {
	case -1:
		return (-1);
	case 0:
		break;
	default:
		/*
		 * A fine point:  _exit(0), not exit(0), to avoid triggering
		 * atexit(3) processing
		 */
		_exit(0);
	}

	newgrp = setsid();
	oerrno = errno;
	if (osa_ok != -1)
		_sigaction(SIGHUP, &osa, NULL);

	if (newgrp == -1) {
		errno = oerrno;
		return (-1);
	}

	if (!nochdir)
		(void)chdir("/");

	if (!noclose && (fd = _open(_PATH_DEVNULL, O_RDWR, 0)) != -1) {
		(void)_dup2(fd, STDIN_FILENO);
//		(void)_dup2(fd, STDOUT_FILENO);
		(void)_dup2(fd, STDERR_FILENO);
//		if (fd > 2)
//			(void)_close(fd);
	}
	return (0);
}

int
main(int argc, char *argv[])
{
	struct pidfh  *ppfh, *pfh;
	sigset_t mask, oldmask;
	int ch, nochdir, noclose, restart, serrno;
	const char *pidfile, *ppidfile,  *user;
	pid_t otherpid, pid;
	FILE *fp;
	int i=0;
	char buffer[1024];
	char line[20];

	char *workdir = NULL;
	char dbfile[]="/var/db/cbsdtaskd.sqlite";
	char *dbpath = NULL;

	if ((workdir = getenv("workdir")) == NULL) {
		printf("cbsd: No workdir defined\n");
		exit(1);
	}

	dbpath = malloc(strlen(workdir) + strlen(dbfile));
	sprintf(dbpath, "%s%s", workdir, dbfile);

	ppidfile = pidfile = user = NULL;

	while ((ch = getopt(argc, argv, "u:")) != -1) {
		switch (ch) {
			case 'u':
				user = optarg;
				break;
			default:
				usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	memset(buffer,0,sizeof(buffer));

	for ( i=0; i<argc; i++ ) {
		strncat( buffer, argv[i], strlen(argv[i]) );
		strncat( buffer, " ", 1 );
	}

	buffer[strlen(buffer)-1]='\0';

	nochdir = 1;
	noclose = 0;
	restart = 0;

	ppfh = pfh = NULL;
	/*
	 * Try to open the pidfile before calling daemon(3),
	 * to be able to report the error intelligently
	 */
	if (pidfile != NULL) {
		pfh = pidfile_open(pidfile, 0600, &otherpid);
		if (pfh == NULL) {
			if (errno == EEXIST) {
				errx(3, "process already running, pid: %d",
				    otherpid);
			}
			err(2, "pidfile ``%s''", pidfile);
		}
	}

	/* Do the same for actual daemon process. */
	if (ppidfile != NULL) {
		ppfh = pidfile_open(ppidfile, 0600, &otherpid);
		if (ppfh == NULL) {
			serrno = errno;
			pidfile_remove(pfh);
			errno = serrno;
			if (errno == EEXIST) {
				errx(3, "process already running, pid: %d",
				     otherpid);
			}
			err(2, "ppidfile ``%s''", ppidfile);
		}
	}

	if (daemon(nochdir, noclose) == -1) {
		warn("daemon");
		goto exit;
	}
	/* Write out parent pidfile if needed. */
	pidfile_write(ppfh);

	/*
	 * If the pidfile or restart option is specified the daemon
	 * executes the command in a forked process and wait on child
	 * exit to remove the pidfile or restart the command. Normally
	 * we don't want the monitoring daemon to be terminated
	 * leaving the running process and the stale pidfile, so we
	 * catch SIGTERM and forward it to the children expecting to
	 * get SIGCHLD eventually.
	 */
	pid = -1;
	if (pidfile != NULL || ppidfile != NULL || restart) {
		/*
		 * Restore default action for SIGTERM in case the
		 * parent process decided to ignore it.
		 */
		if (signal(SIGTERM, SIG_DFL) == SIG_ERR) {
			warn("signal");
			goto exit;
		}
		/*
		 * Because SIGCHLD is ignored by default, setup dummy handler
		 * for it, so we can mask it.
		 */
		if (signal(SIGCHLD, dummy_sighandler) == SIG_ERR) {
			warn("signal");
			goto exit;
		}
		/*
		 * Block interesting signals.
		 */
		sigemptyset(&mask);
		sigaddset(&mask, SIGTERM);
		sigaddset(&mask, SIGCHLD);
		if (sigprocmask(SIG_SETMASK, &mask, &oldmask) == -1) {
			warn("sigprocmask");
			goto exit;
		}
		/*
		 * Try to protect against pageout kill. Ignore the
		 * error, madvise(2) will fail only if a process does
		 * not have superuser privileges.
		 */
		(void)madvise(NULL, 0, MADV_PROTECT);
restart:
		/*
		 * Spawn a child to exec the command, so in the parent
		 * we could wait for it to exit and remove pidfile.
		 */
		pid = fork();
		if (pid == -1) {
			warn("fork");
			goto exit;
		}
	}
	if (pid <= 0) {
		if (pid == 0) {
			/* Restore old sigmask in the child. */
			if (sigprocmask(SIG_SETMASK, &oldmask, NULL) == -1)
				err(1, "sigprocmask");
		}
		/* Now that we are the child, write out the pid. */
		pidfile_write(pfh);

		if (user != NULL)
			restrict_process(user);

		(void)fflush(NULL);
		(void)fflush(stdout);

		fp=popen(buffer,"r");

		if (fp) {
			while (!feof(fp)) {
				fgets(line, sizeof(line), fp);
				if (feof(fp)) break;
				if (line[0]!='.') {
					fprintf(stdout,"%s",line);
					fflush(stdout);
					// add something with local.sqlite for sending notify to wakeup in cbsdd
					// todo: just send signal to cbsdd
					touch(dbpath);
				}
			}
		}
		else {
			err(1, "%s", argv[0]);
		}
	}

	setproctitle("%s[%d]", argv[0], pid);
//	if (wait_child(pid, &mask) == 0 && restart) {
//		sleep(1);
//		goto restart;
//	}
exit:
	pidfile_remove(pfh);
	pidfile_remove(ppfh);
	exit(1); /* If daemon(3) succeeded exit status does not matter. */
}

static void
dummy_sighandler(int sig __unused)
{
	/* Nothing to do. */
}

static void
restrict_process(const char *user)
{
	struct passwd *pw = NULL;

	pw = getpwnam(user);
	if (pw == NULL)
		errx(1, "unknown user: %s", user);

	if (setusercontext(NULL, pw, pw->pw_uid, LOGIN_SETALL) != 0)
		errx(1, "failed to set user environment");
}

static int
wait_child(pid_t pid, sigset_t *mask)
{
	int terminate, signo;

	terminate = 0;
	for (;;) {
		if (sigwait(mask, &signo) == -1) {
			warn("sigwaitinfo");
			return (-1);
		}
		switch (signo) {
		case SIGCHLD:
			if (waitpid(pid, NULL, WNOHANG) == -1) {
				warn("waitpid");
				return (-1);
			}
			return (terminate);
		case SIGTERM:
			terminate = 1;
			if (kill(pid, signo) == -1) {
				warn("kill");
				return (-1);
			}
			continue;
		default:
			warnx("sigwaitinfo: invalid signal: %d", signo);
			return (-1);
		}
	}
}

static void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: daemon [-cfr] [-p child_pidfile] [-P supervisor_pidfile] "
	    "[-u user]\n              command arguments ...\n");
	exit(1);
}

int
touch(char *mypath)
{
	struct stat sb;
	struct timeval tv[2];
	int (*stat_f)(const char *, struct stat *);
	int (*utimes_f)(const char *, const struct timeval *);
	int fd, rval = 0;
	char *p;

	stat_f = stat;
	utimes_f = utimes;
	if (gettimeofday(&tv[0], NULL) == -1)
		err(1, "gettimeofday");

	/* Both times default to the same. */
	tv[1] = tv[0];

	/* See if the file exists. */
	if (stat_f(mypath, &sb) != 0) {
			if (errno != ENOENT) {
				rval = 1;
				warn("%s", mypath);
				return 1;
			}
			/* Create the file. */
			fd = _open(mypath,
				O_WRONLY | O_CREAT, DEFFILEMODE);
			if (fd == -1 || fstat(fd, &sb) || _close(fd)) {
				rval = 1;
				warn("%s", mypath);
				return 1;
			}
	}

	/* Try utimes(2). */
	if (!utimes_f(mypath, tv))
		return 1;

	/*
	 * System V and POSIX 1003.1 require that a NULL argument
	 * set the access/modification times to the current time.
	 * The permission checks are different, too, in that the
	 * ability to write the file is sufficient.  Take a shot.
	 */
	 if (!utimes_f(mypath, NULL))
		return 1;

	rval = 1;
	warn("%s", mypath);
	return (rval);
}
