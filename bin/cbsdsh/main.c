/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char const copyright[] =
"@(#) Copyright (c) 1991, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)main.c	8.6 (Berkeley) 5/28/95";
#endif
#endif /* not lint */
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/bin/sh/main.c 255215 2013-09-04 22:10:16Z jilles $");

#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/file.h>
#include <locale.h>
#include <errno.h>

#ifdef CBSD
#include <sys/param.h> //MAXPATHLEN
#include <stdlib.h> //setenv
//#include <malloc_np.h> //calloc
//#include <libxo/xo.h> // XML/JSON/HTML stuff via xo_emit
#endif

#include "shell.h"
#include "main.h"
#include "mail.h"
#include "options.h"
#include "output.h"
#include "parser.h"
#include "nodes.h"
#include "expand.h"
#include "eval.h"
#include "jobs.h"
#include "input.h"
#include "trap.h"
#include "var.h"
#include "show.h"
#include "memalloc.h"
#include "error.h"
#include "mystring.h"
#include "exec.h"
#include "cd.h"
#include "redir.h"
#include "builtins.h"

int rootpid;
int rootshell;
struct jmploc main_handler;
int localeisutf8, initial_localeisutf8;

#ifdef CBSD
char *cbsd_history_file = NULL;
int cbsd_enable_history=0;
#endif

static void reset(void);
static void cmdloop(int);
static void read_profile(const char *);
static char *find_dot_file(char *);

/*
 * Main routine.  We initialize things, parse the arguments, execute
 * profiles if we're a login shell, and then call cmdloop to execute
 * commands.  The setjmp call sets up the location to jump to when an
 * exception occurs.  When an exception occurs the variable "state"
 * is used to figure out how far we had gotten.
 */

int
main(int argc, char *argv[])
{
	struct stackmark smark, smark2;
	volatile int state;
	char *shinit;
#ifdef CBSD
	char *cbsdpath = NULL;
	char *workdir = NULL;
	char *cbsd_disable_history = NULL; //getenv
	chdir("/var/empty");
	/* Only use history when stdin is a tty. */
	if ( isatty(0) && isatty(1) ) {
		cbsd_enable_history = 1;
	}
#endif
	(void) setlocale(LC_ALL, "");
	initcharset();
	state = 0;
	if (setjmp(main_handler.loc)) {
		switch (exception) {
		case EXEXEC:
			exitstatus = exerrno;
			break;

		case EXERROR:
			exitstatus = 2;
			break;

		default:
			break;
		}

		if (state == 0 || iflag == 0 || ! rootshell ||
		    exception == EXEXIT)
			exitshell(exitstatus);
		reset();
		if (exception == EXINT)
			out2fmt_flush("\n");
		popstackmark(&smark);
		FORCEINTON;				/* enable interrupts */
		if (state == 1)
			goto state1;
		else if (state == 2)
			goto state2;
		else if (state == 3)
			goto state3;
		else
			goto state4;
	}
	handler = &main_handler;
#ifdef DEBUG
	opentrace();
	trputs("Shell args:  ");  trargs(argv);
#endif
	rootpid = getpid();
	rootshell = 1;
	initvar();
	setstackmark(&smark);
	setstackmark(&smark2);

#ifdef CBSD
	if (argc>1)
	    if (!strcmp(argv[1],"--help")) {
		system("/usr/local/bin/cbsd help");
		exit(0);
	    }

	cbsd_disable_history=lookupvar("NO_CBSD_HISTORY");

	if ( cbsd_disable_history != NULL ) cbsd_enable_history=0;

	workdir=lookupvar("workdir");

	if ( workdir == NULL )  {
		read_profile("/etc/rc.conf");
		setvarsafe("workdir", lookupvar("cbsd_workdir"), 0);
	}

	workdir=lookupvar("workdir");
	if ( workdir == NULL ) {
		out2fmt_flush("cbsd: No workdir defined\n");
		exitshell(1);
	}

	setvarsafe("PS1","cbsd@\\h> ",1);
	setvarsafe("workdir",workdir,1);
	workdir=lookupvar("workdir"); //  ^^ after "setsave*" original is free
	cbsdpath = calloc(MAXPATHLEN, sizeof(char *));

	if (cbsdpath == NULL) {
		out2fmt_flush("cbsd: out of memory for cbsdpath\n");
		exitshell(1);
	}

	// %s/modules must be first for opportunity to have a module commands greater priority than the original CBSD command.
	// This makes it possible to write a 3rd party modules with altered functionality of the original code.
	sprintf(cbsdpath,"%s/modules:%s/bin:%s/sbin:%s/tools:%s/jailctl:%s/nodectl:%s/system:/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin",workdir,workdir,workdir,workdir,workdir,workdir,workdir);
	setvarsafe("PATH",cbsdpath,1);
	read_profile("${workdir}/cbsd.conf");

	if (cbsd_enable_history==1) {
		cbsd_history_file=calloc(MAXPATHLEN, sizeof(char *));
		sprintf(cbsd_history_file,"%s/%s",workdir,CBSD_HISTORYFILE);
	}

	ckfree(cbsdpath);
#endif
	procargs(argc, argv);
	pwd_init(iflag);
#ifndef CBSD
	if (iflag)
		chkmail(1);
#endif
	if (argv[0] && argv[0][0] == '-') {
		state = 1;
		read_profile("/etc/profile");
state1:
		state = 2;
		if (privileged == 0)
			read_profile("${HOME-}/.profile");
		else
			read_profile("/etc/suid_profile");
	}
state2:
	state = 3;
	if (!privileged && iflag) {
		if ((shinit = lookupvar("ENV")) != NULL && *shinit != '\0') {
			state = 3;
			read_profile(shinit);
		}
	}
state3:
	state = 4;
	popstackmark(&smark2);
	if (minusc) {
		evalstring(minusc, sflag ? 0 : EV_EXIT);
	}
state4:
	if (sflag || minusc == NULL) {
		cmdloop(1);
	}
	exitshell(exitstatus);
	/*NOTREACHED*/
	return 0;
}

static void
reset(void)
{
	reseteval();
	resetinput();
}

/*
 * Read and execute commands.  "Top" is nonzero for the top level command
 * loop; it turns on prompting if the shell is interactive.
 */

static void
cmdloop(int top)
{
	union node *n;
	struct stackmark smark;
	int inter;
	int numeof = 0;

	TRACE(("cmdloop(%d) called\n", top));
	setstackmark(&smark);
	for (;;) {
		if (pendingsig)
			dotrap();
		inter = 0;
		if (iflag && top) {
			inter++;
			showjobs(1, SHOWJOBS_DEFAULT);
#ifndef CBSD
			chkmail(0);
#endif
			flushout(&output);
		}
		n = parsecmd(inter);
		/* showtree(n); DEBUG */
		if (n == NEOF) {
			if (!top || numeof >= 50)
				break;
			if (!stoppedjobs()) {
				if (!Iflag)
					break;
				out2fmt_flush("\nUse \"exit\" to leave shell.\n");
			}
			numeof++;
		} else if (n != NULL && nflag == 0) {
			job_warning = (job_warning == 2) ? 1 : 0;
			numeof = 0;
			evaltree(n, 0);
		}
		popstackmark(&smark);
		setstackmark(&smark);
		if (evalskip != 0) {
			if (evalskip == SKIPRETURN)
				evalskip = 0;
			break;
		}
	}
	popstackmark(&smark);
}



/*
 * Read /etc/profile or .profile.  Return on error.
 */

static void
read_profile(const char *name)
{
	int fd;
	const char *expandedname;

	expandedname = expandstr(name);
	if (expandedname == NULL)
		return;
	INTOFF;
	if ((fd = open(expandedname, O_RDONLY | O_CLOEXEC)) >= 0)
		setinputfd(fd, 1);
	INTON;
	if (fd < 0)
		return;
	cmdloop(0);
	popfile();
}



/*
 * Read a file containing shell functions.
 */

void
readcmdfile(const char *name)
{
	setinputfile(name, 1);
	cmdloop(0);
	popfile();
}



/*
 * Take commands from a file.  To be compatible we should do a path
 * search for the file, which is necessary to find sub-commands.
 */


static char *
find_dot_file(char *basename)
{
	char *fullname;
	const char *path = pathval();
	struct stat statb;

	/* don't try this for absolute or relative paths */
	if( strchr(basename, '/'))
		return basename;

	while ((fullname = padvance(&path, basename)) != NULL) {
		if ((stat(fullname, &statb) == 0) && S_ISREG(statb.st_mode)) {
			/*
			 * Don't bother freeing here, since it will
			 * be freed by the caller.
			 */
			return fullname;
		}
		stunalloc(fullname);
	}
	return basename;
}

int
dotcmd(int argc, char **argv)
{
	char *filename, *fullname;

	if (argc < 2)
		error("missing filename");

	exitstatus = 0;

	/*
	 * Because we have historically not supported any options,
	 * only treat "--" specially.
	 */
	filename = argc > 2 && strcmp(argv[1], "--") == 0 ? argv[2] : argv[1];

	fullname = find_dot_file(filename);
	setinputfile(fullname, 1);
	commandname = fullname;
	cmdloop(0);
	popfile();
	return exitstatus;
}


int
exitcmd(int argc, char **argv)
{
	if (stoppedjobs())
		return 0;
	if (argc > 1)
		exitshell(number(argv[1]));
	else
		exitshell_savedstatus();
}
