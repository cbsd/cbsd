/*---------------------------------------------------------------------------*\
  This code has been adapted from the original AT&T public domain
  implementation of getopt(3).  The following comment comes from the original
  source, as posted to a newsgroup in 1987.

  The original SCCS ID attached to this code was:

	@(#)getopt.c	2.5 (smail) 9/15/87

  I have ANSI-fied this code and remove the #define of the `index' macro.
  In all other respects, it is identical to the original, posted source.

  This source file is not subject to the license in this directory; it is in
  the public domain.
\*---------------------------------------------------------------------------*/

/*
 * Here's something you've all been waiting for:  the AT&T public domain
 * source for getopt(3).  It is the code which was given out at the 1985
 * UNIFORUM conference in Dallas.  I obtained it by electronic mail
 * directly from AT&T.  The people there assure me that it is indeed
 * in the public domain.
 * 
 * There is no manual page.  That is because the one they gave out at
 * UNIFORUM was slightly different from the current System V Release 2
 * manual page.  The difference apparently involved a note about the
 * famous rules 5 and 6, recommending using white space between an option
 * and its first argument, and not grouping options that have arguments.
 * Getopt itself is currently lenient about both of these things White
 * space is allowed, but not mandatory, and the last option in a group can
 * have an argument.  That particular version of the man page evidently
 * has no official existence, and my source at AT&T did not send a copy.
 * The current SVR2 man page reflects the actual behavor of this getopt.
 * However, I am not about to post a copy of anything licensed by AT&T.
 *
 * ----
 * NOTE: To avoid conflicts with existing system getopt routines, I've
 * changed the names of the function and external variables from getopt(),
 * optind, etc., to x_getopt(), x_optind, etc.
 *
 */

#include <stdio.h>
/* We're ANSI now; we're guaranteed to have strchr(). */
#include <string.h>

int ERR(char *, int);

int ERR(char *s, int c)
{
	extern int write();
	printf("%s\n",s);
	return 0;
}

int	x_opterr = 1;
int	x_optind = 1;
int	x_optopt;
char	*x_optarg;

int
x_getopt(int argc, char **argv, const char *opts)
{
	static int sp = 1;
	register int c;
	register char *cp;

	if(sp == 1) {
		if(x_optind >= argc ||
		   argv[x_optind][0] != '-' || argv[x_optind][1] == '\0') {
			return(EOF);
                } else if(strcmp(argv[x_optind], "--") == 0) {
			x_optind++;
			return(EOF);
		}
        }
	x_optopt = c = argv[x_optind][sp];
	if(c == ':' || (cp=strchr(opts, c)) == NULL) {
		ERR(": illegal option -- ", c);
		if(argv[x_optind][++sp] == '\0') {
			x_optind++;
			sp = 1;
		}
		return('?');
	}
	if(*++cp == ':') {
		if(argv[x_optind][sp+1] != '\0')
			x_optarg = &argv[x_optind++][sp+1];
		else if(++x_optind >= argc) {
			ERR(": option requires an argument -- ", c);
			sp = 1;
			return('?');
		} else
			x_optarg = argv[x_optind++];
		sp = 1;
	} else {
		if(argv[x_optind][++sp] == '\0') {
			sp = 1;
			x_optind++;
		}
		x_optarg = NULL;
	}
	return(c);
}
