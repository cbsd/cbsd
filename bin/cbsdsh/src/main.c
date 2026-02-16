/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1997-2005
 *	Herbert Xu <herbert@gondor.apana.org.au>.  All rights reserved.
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
 * 3. Neither the name of the University nor the names of its contributors
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

#include <locale.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


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
#include "init.h"
#include "mystring.h"
#include "exec.h"
#include "cd.h"

#define PROFILE 0

//CIX
char *progname = NULL;
const char *cix_distdir = NULL;
int cix_function_time = 0;
//#if defined(WITH_REDIS) || defined(WITH_INFLUX) || defined(WITH_DBI)
//_REDIS(cbsdredis_t *redis;)
//_INFLUX(cbsdinflux_t *influx;)
//_DBI(cbsddbi_t *databases;)
//#include "contrib/ini.h"
//#include "cbsdconfig.c" /* Not the best way to do this but will do for now */
//#endif
//CIX

int rootpid;
int mypid;
int shlvl;
#ifdef __GLIBC__
int *dash_errno;
#endif
#if PROFILE
short profile_buf[16384];
extern int etext();
#endif
MKINIT struct jmploc main_handler;

STATIC void read_profile(const char *);
STATIC char *find_dot_file(char *);
static int cmdloop(int);
int main(int, char **);

/*
 * Main routine.  We initialize things, parse the arguments, execute
 * profiles if we're a login shell, and then call cmdloop to execute
 * commands.  The setjmp call sets up the location to jump to when an
 * exception occurs.  When an exception occurs the variable "state"
 * is used to figure out how far we had gotten.
 */

int
main(int argc, char **argv)
{
	char *shinit;
	volatile int state;
	struct stackmark smark;
	int login;

	//CIX
	char *cix_subshell = getenv("CIX_SHELL");

	cix_distdir = getenv("CIX_DISTDIR");
	progname = argv[0] ? argv[0] : "cbsd";
	setvar("CIX_APP", progname, VEXPORT);
	char *cix_distdir_conf = NULL;

	if (cix_distdir == NULL) {
		// transparent support for APPIMAGE ( e.g.: /tmp/.mount_cbsd.A5tyr8g )
		char *appdir = getenv("APPDIR");
		if (appdir == NULL) {
			cix_distdir = "/usr/local/cbsd";
		} else {
			char *tmp = calloc(strlen(appdir) + strlen("/usr/local/cbsd") + 1, sizeof(char));
			if (tmp) {
				sprintf(tmp, "%s/usr/local/cbsd", appdir);
				cix_distdir = tmp;
			} else {
				cix_distdir = "/usr/local/cbsd";
			}
		}
		setvar("CIX_DISTDIR", cix_distdir, VEXPORT);
	}

	cix_distdir_conf = calloc(strlen(cix_distdir) + 11, sizeof(char));  /* + strlen("/cbsd.conf") + '\0' */
	if (cix_distdir_conf)
		sprintf(cix_distdir_conf, "%s/cbsd.conf", cix_distdir);

//        _REDIS(cbsd_redis_init();)
//        _INFLUX(cbsd_influx_init();)
//        _DBI(cbsd_dbi_init();)
//#if defined(WITH_REDIS) || defined(WITH_INFLUX) || defined(WITH_DBI)
//        load_config();
//#endif

	char *cix_function_time_env = NULL;
	char *workdir = NULL;

	// store original CWD in CIX_PWD vars
	cix_pwd_init();
//	chdir("/var/empty");
//CIX

#ifdef __GLIBC__
	dash_errno = __errno_location();
#endif

#if PROFILE
	monitor(4, etext, profile_buf, sizeof profile_buf, 50);
#endif

	setlocale(LC_ALL, "");

	state = 0;
	if (unlikely(setjmp(main_handler.loc))) {
		int e;
		int s;

		exitreset();

		e = exception;

		s = state;
		if (e == EXEND || e == EXEXIT || s == 0 || iflag == 0 || shlvl)
			goto exit;

		reset();

		if (e == EXINT
#if ATTY
		 && (! attyset() || equal(termval(), "emacs"))
#endif
		 ) {
			out2c('\n');
#ifdef FLUSHERR
			flushout(out2);
#endif
		}
		popstackmark(&smark);
		FORCEINTON;				/* enable interrupts */
		if (s == 1)
			goto state1;
		else if (s == 2)
			goto state2;
		else if (s == 3)
			goto state3;
		else
			goto state4;
	}
	handler = &main_handler;
#ifdef DEBUG
	opentrace();
	trputs("Shell args:  ");  trargs(argv);
#endif
	mypid = rootpid = getpid();
	init();
	setstackmark(&smark);

//CIX
	workdir = getenv("workdir");
	if (workdir == NULL) {
		if (cix_subshell == NULL) read_profile("/etc/rc.conf");
		workdir=lookupvar("cbsd_workdir");
	}

	if (workdir == NULL) {
		out2str("cbsd: no workdir defined\n");
		exit(1);
	}

	setvar("workdir", workdir, VEXPORT);
	setvar("CIX_SHELL", "1", VEXPORT);

	setvar("PS1", "cbsd# ", VEXPORT);
	//if (cix_subshell == NULL)
	if (cix_distdir_conf)
		read_profile(cix_distdir_conf);

//	cix_distdir = getenv("CIX_DISTDIR");
	setvar("CIX_PATH", lookupvar("CIX_PATH"), VEXPORT);

	// non-interactive global env
	if (getenv("NOINTER") != NULL) {
		setvar("inter", "1", VEXPORT);
	}

	cix_function_time_env=getenv("CIX_FUNCTION_TIME");
	if (cix_function_time_env != NULL)
		cix_function_time=atoi(cix_function_time_env);
	else
		cix_function_time=0;
//CIX
	login = procargs(argv);
	if (login) {
		state = 1;
		//CIX
		//read_profile("/etc/profile");
state1:
		state = 2;
		//CIX
		//read_profile("$HOME/.profile");
	}

state2:
	state = 3;
	if (
#ifndef linux
		getuid() == geteuid() && getgid() == getegid() &&
#endif
		iflag
	) {
		if ((shinit = lookupvar("ENV")) != NULL && *shinit != '\0') {
			read_profile(shinit);
		}
	}
	popstackmark(&smark);
state3:
	state = 4;
	if (minusc)
		evalstring(minusc, sflag ? 0 : EV_EXIT);

	if (sflag || minusc == NULL) {
state4:	/* XXX ??? - why isn't this before the "if" statement */
		cmdloop(1);
	}
exit:
#if PROFILE
	monitor(0);
#endif
#if GPROF
	{
		extern void _mcleanup(void);
		_mcleanup();
	}
#endif
	free(cix_distdir_conf);
	exitshell();
	/* NOTREACHED */
}


/*
 * Read and execute commands.  "Top" is nonzero for the top level command
 * loop; it turns on prompting if the shell is interactive.
 */

static int
cmdloop(int top)
{
	union node *n;
	struct stackmark smark;
	int inter;
	int status = 0;
	int numeof = 0;

	TRACE(("cmdloop(%d) called\n", top));
	for (;;) {
		int skip;

		setstackmark(&smark);
		if (jobctl)
			showjobs(out2, SHOW_CHANGED);
		inter = 0;
		if (iflag && top) {
			inter++;
			chkmail();
		}
		n = parsecmd(inter);
		/* showtree(n); DEBUG */
		if (n == NEOF) {
			if (!top || numeof >= 50)
				break;
			if (!stoppedjobs()) {
				if (!Iflag) {
					if (iflag) {
						out2c('\n');
#ifdef FLUSHERR
						flushout(out2);
#endif
					}
					break;
				}
				out2str("\nUse \"exit\" to leave shell.\n");
			}
			numeof++;
		} else {
			int i;

			job_warning = (job_warning == 2) ? 1 : 0;
			numeof = 0;
			i = evaltree(n, 0);
			if (n)
				status = i;
		}
		popstackmark(&smark);

		skip = evalskip;
		if (skip) {
			evalskip &= ~(SKIPFUNC | SKIPFUNCDEF);
			break;
		}
	}

	return status;
}



/*
 * Read /etc/profile or .profile.  Return on error.
 */

STATIC void
read_profile(const char *name)
{
	name = expandstr(name);
	if (setinputfile(name, INPUT_PUSH_FILE | INPUT_NOFILE_OK) < 0)
		return;

	cmdloop(0);
	popfile();
}



/*
 * Read a file containing shell functions.
 */

void
readcmdfile(char *name)
{
	setinputfile(name, INPUT_PUSH_FILE);
	cmdloop(0);
	popfile();
}



/*
 * Take commands from a file.  To be compatible we should do a path
 * search for the file, which is necessary to find sub-commands.
 */


STATIC char *
find_dot_file(char *basename)
{
	char *fullname;
	const char *path = pathval();
	struct stat64 statb;
	int len;

	/* don't try this for absolute or relative paths */
	if (strchr(basename, '/'))
		return basename;

	while ((len = padvance(&path, basename)) >= 0) {
		fullname = stackblock();
		if ((!pathopt || *pathopt == 'f') &&
		    !stat64(fullname, &statb) && S_ISREG(statb.st_mode)) {
			/* This will be freed by the caller. */
			return stalloc(len);
		}
	}

	/* not found in the PATH */
	sh_error("%s: not found", basename);
	/* NOTREACHED */
}

int
dotcmd(int argc, char **argv)
{
	int status = 0;

	nextopt(nullstr);
	argv = argptr;

	if (*argv) {
		char *fullname;

		fullname = find_dot_file(*argv);
		setinputfile(fullname, INPUT_PUSH_FILE);
		commandname = fullname;
		status = cmdloop(0);
		popfile();
	}

	return status;
}


/*
 * CIX cixinit builtin: parse and process $* (positional parameters)
 * according to CIXARG, CIXOPTARG, ADDHELP, MYDESC, CBSDMODULE, etc.
 */
static int in_cix_list(const char *name, const char *list)
{
	const char *p;
	size_t n;
	if (!list || !*list)
		return 0;
	n = strlen(name);
	for (p = list; *p; ) {
		while (*p == ' ') p++;
		if (!*p) break;
		if (strncmp(p, name, n) == 0 && (p[n] == ' ' || p[n] == '\0'))
			return 1;
		while (*p && *p != ' ') p++;
	}
	return 0;
}

/* Strip ":type" from "name:type", return pointer to name (may modify buf) */
static void cix_strip_type(char *buf, char **out_name)
{
	char *colon = strchr(buf, ':');
	if (colon) {
		*colon = '\0';
	}
	*out_name = buf;
}

/*
 * Parse key=value when value may span multiple argv[] (e.g. sysrc="a b c" split as sysrc="a, "b", "c").
 * If value starts with ", collect argv[start_idx] and following args until we see closing ".
 * Sets *out_value (caller must ckfree when != value_start) and *out_last_idx (last argv index consumed).
 */
static void cix_parse_quoted_value_argv(char **argv, int nparam, int start_idx,
    const char *value_start, char **out_value, int *out_last_idx)
{
	const char *p;
	char *buf, *q;
	size_t cap, len;
	int idx;
	int in_quotes;

	if (*value_start != '"') {
		*out_value = (char *)value_start;
		*out_last_idx = start_idx;
		return;
	}
	/* Value starts with "; may span multiple args until we find closing " */
	cap = strlen(value_start) + 1;
	buf = ckmalloc(cap);
	strcpy(buf, value_start);
	idx = start_idx;
	in_quotes = 1;
	/* Scan buf for unescaped closing " */
	for (p = buf + 1; *p; ) {
		if (p[0] == '\\' && p[1] == '"') {
			p += 2;
			continue;
		}
		if (*p == '"') {
			in_quotes = 0;
			break;
		}
		p++;
	}
	/* Only append next arg if it contains '"' (continuation of quoted value); else stop so
	 * args like runasap=1 stay separate and are not merged into this value. */
	while (in_quotes && idx + 1 < nparam && argv[idx + 1] && strchr(argv[idx + 1], '"')) {
		idx++;
		len = strlen(buf) + 1 + strlen(argv[idx]) + 1;
		if (len > cap) {
			cap = len + 256;
			buf = ckrealloc(buf, cap);
		}
		strcat(buf, " ");
		strcat(buf, argv[idx]);
		for (p = buf + 1; *p; ) {
			if (p[0] == '\\' && p[1] == '"') {
				p += 2;
				continue;
			}
			if (*p == '"') {
				in_quotes = 0;
				break;
			}
			p++;
		}
		if (!*p)
			in_quotes = 1;
	}
	*out_last_idx = idx;
	/* Extract value between first " and closing ", unescaping \" */
	p = buf + 1;
	len = 0;
	for (; *p; ) {
		if (p[0] == '\\' && p[1] == '"') {
			len++;
			p += 2;
			continue;
		}
		if (*p == '"')
			break;
		len++;
		p++;
	}
	q = ckmalloc(len + 1);
	*out_value = q;
	p = buf + 1;
	while (*p && *p != '"') {
		if (p[0] == '\\' && p[1] == '"') {
			*q++ = '"';
			p += 2;
		} else {
			*q++ = *p++;
		}
	}
	*q = '\0';
	ckfree(buf);
}

/* Check if token is exactly "WHERE" (reserved) */
static int is_where(const char *s)
{
	return strcmp(s, "WHERE") == 0;
}

/* Print string to out1 interpreting ANSI-style backslash escapes (\033, \e, \n, \t, \r, \\, \xHH, \0OOO) */
static void cix_print_interpret_escapes(const char *s)
{
	unsigned int u;
	const char *p;

	for (p = s; *p; p++) {
		if (*p != '\\') {
			out1c((unsigned char)*p);
			continue;
		}
		p++;
		if (!*p)
			break;
		switch (*p) {
		case 'e': case 'E':
			out1c('\033');
			break;
		case 'n':
			out1c('\n');
			break;
		case 't':
			out1c('\t');
			break;
		case 'r':
			out1c('\r');
			break;
		case '\\':
			out1c('\\');
			break;
		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': {
			u = (unsigned char)(*p - '0');
			if (p[1] >= '0' && p[1] <= '7') {
				p++;
				u = u * 8 + (unsigned char)(*p - '0');
				if (p[1] >= '0' && p[1] <= '7') {
					p++;
					u = u * 8 + (unsigned char)(*p - '0');
				}
			}
			out1c((char)(u & 0xff));
			break;
		}
		case 'x': case 'X':
			u = 0;
			if (p[1] >= '0' && p[1] <= '9') u = u * 16 + (p[1] - '0');
			else if (p[1] >= 'A' && p[1] <= 'F') u = u * 16 + (p[1] - 'A' + 10);
			else if (p[1] >= 'a' && p[1] <= 'f') u = u * 16 + (p[1] - 'a' + 10);
			else { out1c(*p); break; }
			p++;
			if (p[1] >= '0' && p[1] <= '9') u = u * 16 + (p[1] - '0');
			else if (p[1] >= 'A' && p[1] <= 'F') u = u * 16 + (p[1] - 'A' + 10);
			else if (p[1] >= 'a' && p[1] <= 'f') u = u * 16 + (p[1] - 'a' + 10);
			else { out1c((char)(u & 0xff)); break; }
			p++;
			out1c((char)(u & 0xff));
			break;
		default:
			out1c(*p);
			break;
		}
	}
}

int
cixinitcmd(int argc, char **argv)
{
	char **p;
	int nparam;
	int i, where_idx, has_help, has_desc, has_argstype, has_args;
	const char *addhelp, *mydesc, *cbsdmodule, *cbsdmoduleonly;
	const char *cixarg, *cixoptarg;
	char *myarg_str, *myoptarg_str;
	char *eq, *val_parsed;
	size_t myarg_len, myoptarg_len;
	char *prog;
	char **exec_argv;
	const char *old_other;
	const char *skip_list;
	char *new_other;
	(void)argc;
	(void)argv;

	nparam = shellparam.nparam;
	p = shellparam.p;

	/* 1) Only one param "--help" and ADDHELP not set -> exec ${progname} help */
	if (nparam == 1 && p[0] && strcmp(p[0], "--help") == 0) {
		addhelp = lookupvar("ADDHELP");
		if (!addhelp || !*addhelp) {
			prog = progname && *progname ? progname : "cbsd";
			exec_argv = ckmalloc(3 * sizeof(char *));
			exec_argv[0] = savestr(prog);
			exec_argv[1] = savestr("help");
			exec_argv[2] = NULL;
			shellexec(exec_argv, pathval(), 0);
			/* NOTREACHED */
		}
		/* else ADDHELP is set -> fall through to step 2/3 to print it */
	}

	/* 2) and 3) --help in params: no ADDHELP -> "no help available"; else print ADDHELP */
	has_help = 0;
	for (i = 0; i < nparam && p[i]; i++) {
		if (strcmp(p[i], "--help") == 0) {
			has_help = 1;
			break;
		}
	}
	if (has_help) {
		addhelp = lookupvar("ADDHELP");
		if (!addhelp || !*addhelp) {
			out2str("no help available\n");
			flushall();
			exraise(EXEXIT);
		}
		cix_print_interpret_escapes(addhelp);
		out1c('\n');
		flushall();
		exraise(EXEXIT);
	}

	/* 4) --desc */
	has_desc = 0;
	for (i = 0; i < nparam && p[i]; i++) {
		if (strcmp(p[i], "--desc") == 0) {
			has_desc = 1;
			break;
		}
	}
	if (has_desc) {
		mydesc = lookupvar("MYDESC");
		if (!mydesc || !*mydesc) {
			out2str("no desc available\n");
			flushall();
			exraise(EXEXIT);
		}
		cbsdmodule = lookupvar("CBSDMODULE");
		cbsdmoduleonly = lookupvar("CBSDMODULEONLY");
		if (!cbsdmodule && !cbsdmoduleonly) {
			cix_print_interpret_escapes(mydesc);
			out1c('\n');
			flushall();
			exraise(EXEXIT);
		}
		if (cbsdmoduleonly && *cbsdmoduleonly) {
			/* Check CBSDMODULEONLY is in CBSDMODULE (e.g. comma-sep list) */
			if (!cbsdmodule || !*cbsdmodule) {
				flushall();
				exraise(EXEXIT);
			}
			{
				char *modcopy = savestr(cbsdmodule);
				char *tok;
				int found = 0;
				for (tok = strtok(modcopy, ","); tok; tok = strtok(NULL, ",")) {
					while (*tok == ' ') tok++;
					if (strcmp(tok, cbsdmoduleonly) == 0) {
						found = 1;
						break;
					}
				}
				ckfree(modcopy);
				if (!found) {
					flushall();
					exraise(EXEXIT);
				}
			}
		}
		cix_print_interpret_escapes(mydesc);
		out1c('\n');
		flushall();
		exraise(EXEXIT);
	}

	/* 5) --argstype / --cixargs: output CIXARG and CIXOPTARG (with types) line by line */
	has_argstype = 0;
	for (i = 0; i < nparam && p[i]; i++) {
		if (strcmp(p[i], "--argstype") == 0 || strcmp(p[i], "--cixargs") == 0) {
			has_argstype = 1;
			break;
		}
	}
	if (has_argstype) {
		cixarg = lookupvar("CIXARG");
		cixoptarg = lookupvar("CIXOPTARG");
		if (cixarg && *cixarg) {
			char *copy = savestr(cixarg);
			char *tok = strtok(copy, " \t");
			while (tok) {
				out1str(tok);
				out1c('\n');
				tok = strtok(NULL, " \t");
			}
			ckfree(copy);
		}
		if (cixoptarg && *cixoptarg) {
			char *copy = savestr(cixoptarg);
			char *tok = strtok(copy, " \t");
			while (tok) {
				out1str(tok);
				out1c('\n');
				tok = strtok(NULL, " \t");
			}
			ckfree(copy);
		}
		flushall();
		exraise(EXEXIT);
	}

	/* 6) Copy CIXARG/CIXOPTARG to MYARG/MYOPTARG (strip :type), then unset */
	cixarg = lookupvar("CIXARG");
	cixoptarg = lookupvar("CIXOPTARG");
	myarg_len = 0;
	if (cixarg && *cixarg) {
		char *copy = savestr(cixarg);
		char *tok = strtok(copy, " \t");
		while (tok) {
			cix_strip_type(tok, &tok);
			myarg_len += strlen(tok) + 1;
			tok = strtok(NULL, " \t");
		}
		ckfree(copy);
	}
	myoptarg_len = 0;
	if (cixoptarg && *cixoptarg) {
		char *copy = savestr(cixoptarg);
		char *tok = strtok(copy, " \t");
		while (tok) {
			cix_strip_type(tok, &tok);
			myoptarg_len += strlen(tok) + 1;
			tok = strtok(NULL, " \t");
		}
		ckfree(copy);
	}
	myarg_str = ckmalloc(myarg_len + 1);
	myarg_str[0] = '\0';
	if (cixarg && *cixarg) {
		char *copy = savestr(cixarg);
		char *tok = strtok(copy, " \t");
		int first = 1;
		for (; tok; tok = strtok(NULL, " \t")) {
			cix_strip_type(tok, &tok);
			if (!first) strcat(myarg_str, " ");
			strcat(myarg_str, tok);
			first = 0;
		}
		ckfree(copy);
	}
	myoptarg_str = ckmalloc(myoptarg_len + 1);
	myoptarg_str[0] = '\0';
	if (cixoptarg && *cixoptarg) {
		char *copy = savestr(cixoptarg);
		char *tok = strtok(copy, " \t");
		int first = 1;
		for (; tok; tok = strtok(NULL, " \t")) {
			cix_strip_type(tok, &tok);
			if (!first) strcat(myoptarg_str, " ");
			strcat(myoptarg_str, tok);
			first = 0;
		}
		ckfree(copy);
	}
	setvar("MYARG", myarg_str, 0);
	setvar("MYOPTARG", myoptarg_str, 0);
	ckfree(myarg_str);
	ckfree(myoptarg_str);
	unsetvar("CIXARG");
	unsetvar("CIXOPTARG");

	/* Reload after setvar */
	myarg_str = (char *)lookupvar("MYARG");
	myoptarg_str = (char *)lookupvar("MYOPTARG");

	/* 7) --args: output MYARG and MYOPTARG line by line */
	has_args = 0;
	for (i = 0; i < nparam && p[i]; i++) {
		if (strcmp(p[i], "--args") == 0) {
			has_args = 1;
			break;
		}
	}
	if (has_args) {
		if (myarg_str && *myarg_str) {
			char *copy = savestr(myarg_str);
			char *tok = strtok(copy, " \t");
			while (tok) {
				out1str(tok);
				out1c('\n');
				tok = strtok(NULL, " \t");
			}
			ckfree(copy);
		}
		if (myoptarg_str && *myoptarg_str) {
			char *copy = savestr(myoptarg_str);
			char *tok = strtok(copy, " \t");
			while (tok) {
				out1str(tok);
				out1c('\n');
				tok = strtok(NULL, " \t");
			}
			ckfree(copy);
		}
		flushall();
		exraise(EXEXIT);
	}

	/* 8) Find WHERE: everything after goes to CIXINIT_SQL_CONDITION */
	where_idx = -1;
	for (i = 0; i < nparam && p[i]; i++) {
		if (is_where(p[i])) {
			where_idx = i;
			break;
		}
	}
	if (where_idx >= 0) {
		char *cond;
		size_t cond_len = 0;
		int first = 1;
		for (i = where_idx + 1; i < nparam && p[i]; i++) {
			if (!first) cond_len += 1;
			cond_len += strlen(p[i]) + 1;
			first = 0;
		}
		cond = ckmalloc(cond_len + 1);
		cond[0] = '\0';
		first = 1;
		for (i = where_idx + 1; i < nparam && p[i]; i++) {
			if (!first) strcat(cond, " ");
			strcat(cond, p[i]);
			first = 0;
		}
		setvar("CIXINIT_SQL_CONDITION", cond, 0);
		ckfree(cond);
	}

	/* 9) Parse key=value; 10) mandatory check; 11) o-prefix; 12) CIX_OTHER_ARGS */
	/* CIX_INIT_SKIP: params listed here are not initialized, added to CIX_OTHER_ARGS as-is */
	skip_list = lookupvar("CIX_INIT_SKIP");
	unsetvar("CIX_OTHER_ARGS");
	old_other = NULL;

	for (i = 0; i < nparam && p[i]; ) {
		char *param_name;
		size_t name_len;
		int last_consumed;
		int j;

		if (where_idx >= 0 && i >= where_idx)
			break; /* already in CIXINIT_SQL_CONDITION */
		if (strcmp(p[i], "--desc") == 0 || strcmp(p[i], "--argstype") == 0 ||
		    strcmp(p[i], "--cixargs") == 0 || strcmp(p[i], "--args") == 0) {
			i++;
			continue; /* skip special flags */
		}

		eq = strchr(p[i], '=');
		if (eq) {
			name_len = (size_t)(eq - p[i]);
			param_name = ckmalloc(name_len + 1);
			memcpy(param_name, p[i], name_len);
			param_name[name_len] = '\0';

			if (in_cix_list(param_name, myarg_str) || in_cix_list(param_name, myoptarg_str)) {
				cix_parse_quoted_value_argv(p, nparam, i, eq + 1, &val_parsed, &last_consumed);
				if (skip_list && *skip_list && in_cix_list(param_name, skip_list)) {
					/* Skip init: add p[i]..p[last_consumed] as-is to CIX_OTHER_ARGS */
					size_t raw_len = 0;
					char *raw;

					for (j = i; j <= last_consumed && p[j]; j++)
						raw_len += strlen(p[j]) + (j > i ? 1 : 0);
					raw = ckmalloc(raw_len + 1);
					raw[0] = '\0';
					for (j = i; j <= last_consumed && p[j]; j++) {
						if (j > i)
							strcat(raw, " ");
						strcat(raw, p[j]);
					}
					new_other = old_other
						? ckmalloc(strlen(old_other) + 1 + strlen(raw) + 1)
						: ckmalloc(strlen(raw) + 1);
					if (old_other)
						sprintf(new_other, "%s %s", old_other, raw);
					else
						strcpy(new_other, raw);
					setvar("CIX_OTHER_ARGS", new_other, 0);
					ckfree(new_other);
					ckfree(raw);
					old_other = lookupvar("CIX_OTHER_ARGS");
					if (val_parsed != (char *)(eq + 1))
						ckfree(val_parsed);
				} else {
					setvar(param_name, val_parsed, 0);
					if (val_parsed != (char *)(eq + 1))
						ckfree(val_parsed);
				}
				i = last_consumed + 1;
			} else {
				new_other = old_other
					? ckmalloc(strlen(old_other) + 1 + strlen(p[i]) + 1)
					: ckmalloc(strlen(p[i]) + 1);
				if (old_other)
					sprintf(new_other, "%s %s", old_other, p[i]);
				else
					strcpy(new_other, p[i]);
				setvar("CIX_OTHER_ARGS", new_other, 0);
				ckfree(new_other);
				old_other = lookupvar("CIX_OTHER_ARGS");
				i++;
			}
			ckfree(param_name);
		} else {
			/* No '=': add as-is to CIX_OTHER_ARGS */
			new_other = old_other
				? ckmalloc(strlen(old_other) + 1 + strlen(p[i]) + 1)
				: ckmalloc(strlen(p[i]) + 1);
			if (old_other)
				sprintf(new_other, "%s %s", old_other, p[i]);
			else
				strcpy(new_other, p[i]);
			setvar("CIX_OTHER_ARGS", new_other, 0);
			ckfree(new_other);
			old_other = lookupvar("CIX_OTHER_ARGS");
			i++;
		}
	}

	/* 10) Mandatory: every MYARG param must be present (non-empty) */
	if (myarg_str && *myarg_str) {
		char *copy = savestr(myarg_str);
		char *tok = strtok(copy, " \t");
		while (tok) {
			const char *val = lookupvar(tok);
			if (!val || !*val) {
				outfmt(out2, "param %s is mandatory\n", tok);
				ckfree(copy);
				exraise(EXEXIT);
			}
			tok = strtok(NULL, " \t");
		}
		ckfree(copy);
	}

	/* 11) Duplicate to o-prefix vars */
	if (myarg_str && *myarg_str) {
		char *copy = savestr(myarg_str);
		char *tok = strtok(copy, " \t");
		while (tok) {
			const char *v = lookupvar(tok);
			if (v) {
				char *oname = ckmalloc(strlen(tok) + 2);
				sprintf(oname, "o%s", tok);
				setvar(oname, v, 0);
				ckfree(oname);
			}
			tok = strtok(NULL, " \t");
		}
		ckfree(copy);
	}
	if (myoptarg_str && *myoptarg_str) {
		char *copy = savestr(myoptarg_str);
		char *tok = strtok(copy, " \t");
		while (tok) {
			const char *v = lookupvar(tok);
			if (v) {
				char *oname = ckmalloc(strlen(tok) + 2);
				sprintf(oname, "o%s", tok);
				setvar(oname, v, 0);
				ckfree(oname);
			}
			tok = strtok(NULL, " \t");
		}
		ckfree(copy);
	}

	/* CIX_INIT_SAVE2FILE: save initialized params to file, set CIX_INIT_CONF, unset CIX_INIT_SAVE2FILE */
	{
		const char *save2file;
		char *path = NULL;
		int fd = -1;
		char ch;

		save2file = lookupvar("CIX_INIT_SAVE2FILE");
		if (save2file && *save2file) {
			if (save2file[0] == '/') {
				path = savestr(save2file);
				if (path)
					fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			} else {
				char template[256];
				snprintf(template, sizeof(template), "/tmp/%s.XXXXXX", save2file);
				fd = mkstemp(template);
				if (fd >= 0) {
					path = savestr(template);
					if (!path)
						close(fd);
				}
			}
			if (path && fd >= 0) {
				/* Write MYARG and MYOPTARG params: one line per param, param="value" (escape " in value) */
				if (myarg_str && *myarg_str) {
					char *copy = savestr(myarg_str);
					char *tok = strtok(copy, " \t");
					while (tok) {
						const char *v = lookupvar(tok);
						if (v && *v) {
							write(fd, tok, strlen(tok));
							write(fd, "=\"", 2);
							for (; *v; v++) {
								if (*v == '"')
									write(fd, "\\\"", 2);
								else {
									ch = *v;
									write(fd, &ch, 1);
								}
							}
							write(fd, "\"\n", 2);
						}
						tok = strtok(NULL, " \t");
					}
					ckfree(copy);
				}
				if (myoptarg_str && *myoptarg_str) {
					char *copy = savestr(myoptarg_str);
					char *tok = strtok(copy, " \t");
					while (tok) {
						const char *v = lookupvar(tok);
						if (v && *v) {
							write(fd, tok, strlen(tok));
							write(fd, "=\"", 2);
							for (; *v; v++) {
								if (*v == '"')
									write(fd, "\\\"", 2);
								else {
									ch = *v;
									write(fd, &ch, 1);
								}
							}
							write(fd, "\"\n", 2);
						}
						tok = strtok(NULL, " \t");
					}
					ckfree(copy);
				}
				close(fd);
				setvar("CIX_INIT_CONF", path, 0);
			}
			if (path)
				ckfree(path);
			unsetvar("CIX_INIT_SAVE2FILE");
		}
	}

	return 0;
}

int
exitcmd(int argc, char **argv)
{
	if (stoppedjobs())
		return 0;

	if (argc > 1)
		savestatus = number(argv[1]);

	exraise(EXEXIT);
	/* NOTREACHED */
}

#ifdef mkinit
INCLUDE "error.h"

FORKRESET {
	handler = &main_handler;
}
#endif
