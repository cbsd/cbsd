/* signames.c -- Create and write `signames.c', which contains an array of
   signal names. */

/* Copyright (C) 1992 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2, or (at your option) any later
   version.

   Bash is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License along
   with Bash; see the file COPYING.  If not, write to the Free Software
   Foundation, 59 Temple Place, Suite 330, Boston, MA 02111 USA. */

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>

#if !defined (NSIG)
#  define NSIG 64
#endif

/*
 * Special traps:
 *	EXIT == 0
 */
#define LASTSIG NSIG-1

char *signal_names[2 * NSIG + 3];

#define signal_names_size (sizeof(signal_names)/sizeof(signal_names[0]))

char *progname;

/* AIX 4.3 defines SIGRTMIN and SIGRTMAX as 888 and 999 respectively.
   I don't want to allocate so much unused space for the intervening signal
   numbers, so we just punt if SIGRTMAX is past the bounds of the
   signal_names array (handled in configure). */
#if defined (SIGRTMAX) && defined (UNUSABLE_RT_SIGNALS)
#  undef SIGRTMAX
#  undef SIGRTMIN
#endif

#if defined (SIGRTMAX) || defined (SIGRTMIN)
#  define RTLEN 14
#  define RTLIM 256
#endif

void
initialize_signames ()
{
  register int i;
#if defined (SIGRTMAX) || defined (SIGRTMIN)
  int rtmin, rtmax, rtcnt;
#endif

  for (i = 1; i < signal_names_size; i++)
    signal_names[i] = (char *)NULL;

  /* `signal' 0 is what we do on exit. */
  signal_names[0] = "EXIT";

  /* Place signal names which can be aliases for more common signal
     names first.  This allows (for example) SIGABRT to overwrite SIGLOST. */

  /* POSIX 1003.1b-1993 real time signals, but take care of incomplete
     implementations. Acoording to the standard, both, SIGRTMIN and
     SIGRTMAX must be defined, SIGRTMIN must be stricly less than
     SIGRTMAX, and the difference must be at least 7, that is, there
     must be at least eight distinct real time signals. */

  /* The generated signal names are SIGRTMIN, SIGRTMIN+1, ...,
     SIGRTMIN+x, SIGRTMAX-x, ..., SIGRTMAX-1, SIGRTMAX. If the number
     of RT signals is odd, there is an extra SIGRTMIN+(x+1).
     These names are the ones used by ksh and /usr/xpg4/bin/sh on SunOS5. */

#if defined (SIGRTMIN)
  rtmin = SIGRTMIN;
  signal_names[rtmin] = "RTMIN";
#endif

#if defined (SIGRTMAX)
  rtmax = SIGRTMAX;
  signal_names[rtmax] = "RTMAX";
#endif

#if defined (SIGRTMAX) && defined (SIGRTMIN)
  if (rtmax > rtmin)
    {
      rtcnt = (rtmax - rtmin - 1) / 2;
      /* croak if there are too many RT signals */
      if (rtcnt >= RTLIM/2)
	{
	  rtcnt = RTLIM/2-1;
	  fprintf(stderr, "%s: error: more than %i real time signals, fix `%s'\n",
		  progname, RTLIM, progname);
	}

      for (i = 1; i <= rtcnt; i++)
	{
	  signal_names[rtmin+i] = (char *)malloc(RTLEN);
	  if (signal_names[rtmin+i])
	    sprintf (signal_names[rtmin+i], "RTMIN+%d", i);
	  signal_names[rtmax-i] = (char *)malloc(RTLEN);
	  if (signal_names[rtmax-i])
	    sprintf (signal_names[rtmax-i], "RTMAX-%d", i);
	}

      if (rtcnt < RTLIM/2-1 && rtcnt != (rtmax-rtmin)/2)
	{
	  /* Need an extra RTMIN signal */
	  signal_names[rtmin+rtcnt+1] = (char *)malloc(RTLEN);
	  if (signal_names[rtmin+rtcnt+1])
	    sprintf (signal_names[rtmin+rtcnt+1], "RTMIN+%d", rtcnt+1);
	}
    }
#endif /* SIGRTMIN && SIGRTMAX */

/* AIX */
#if defined (SIGLOST)	/* resource lost (eg, record-lock lost) */
  signal_names[SIGLOST] = "LOST";
#endif

#if defined (SIGMSG)	/* HFT input data pending */
  signal_names[SIGMSG] = "MSG";
#endif

#if defined (SIGDANGER)	/* system crash imminent */
  signal_names[SIGDANGER] = "DANGER";
#endif

#if defined (SIGMIGRATE) /* migrate process to another CPU */
  signal_names[SIGMIGRATE] = "MIGRATE";
#endif

#if defined (SIGPRE)	/* programming error */
  signal_names[SIGPRE] = "PRE";
#endif

#if defined (SIGVIRT)	/* AIX virtual time alarm */
  signal_names[SIGVIRT] = "VIRT";
#endif

#if defined (SIGALRM1)	/* m:n condition variables */
  signal_names[SIGALRM1] = "ALRM1";
#endif

#if defined (SIGWAITING)	/* m:n scheduling */
  signal_names[SIGWAITING] = "WAITING";
#endif

#if defined (SIGGRANT)	/* HFT monitor mode granted */
  signal_names[SIGGRANT] = "GRANT";
#endif

#if defined (SIGKAP)	/* keep alive poll from native keyboard */
  signal_names[SIGKAP] = "KAP";
#endif

#if defined (SIGRETRACT) /* HFT monitor mode retracted */
  signal_names[SIGRETRACT] = "RETRACT";
#endif

#if defined (SIGSOUND)	/* HFT sound sequence has completed */
  signal_names[SIGSOUND] = "SOUND";
#endif

#if defined (SIGSAK)	/* Secure Attention Key */
  signal_names[SIGSAK] = "SAK";
#endif

/* SunOS5 */
#if defined (SIGLWP)	/* special signal used by thread library */
  signal_names[SIGLWP] = "LWP";
#endif

#if defined (SIGFREEZE)	/* special signal used by CPR */
  signal_names[SIGFREEZE] = "FREEZE";
#endif

#if defined (SIGTHAW)	/* special signal used by CPR */
  signal_names[SIGTHAW] = "THAW";
#endif

#if defined (SIGCANCEL)	/* thread cancellation signal used by libthread */
  signal_names[SIGCANCEL] = "CANCEL";
#endif

/* HP-UX */
#if defined (SIGDIL)	/* DIL signal (?) */
  signal_names[SIGDIL] = "DIL";
#endif

/* System V */
#if defined (SIGCLD)	/* Like SIGCHLD.  */
  signal_names[SIGCLD] = "CLD";
#endif

#if defined (SIGPWR)	/* power state indication */
  signal_names[SIGPWR] = "PWR";
#endif

#if defined (SIGPOLL)	/* Pollable event (for streams)  */
  signal_names[SIGPOLL] = "POLL";
#endif

/* Unknown */
#if defined (SIGWINDOW)
  signal_names[SIGWINDOW] = "WINDOW";
#endif

/* Common */
#if defined (SIGHUP)	/* hangup */
  signal_names[SIGHUP] = "HUP";
#endif

#if defined (SIGINT)	/* interrupt */
  signal_names[SIGINT] = "INT";
#endif

#if defined (SIGQUIT)	/* quit */
  signal_names[SIGQUIT] = "QUIT";
#endif

#if defined (SIGILL)	/* illegal instruction (not reset when caught) */
  signal_names[SIGILL] = "ILL";
#endif

#if defined (SIGTRAP)	/* trace trap (not reset when caught) */
  signal_names[SIGTRAP] = "TRAP";
#endif

#if defined (SIGIOT)	/* IOT instruction */
  signal_names[SIGIOT] = "IOT";
#endif

#if defined (SIGABRT)	/* Cause current process to dump core. */
  signal_names[SIGABRT] = "ABRT";
#endif

#if defined (SIGEMT)	/* EMT instruction */
  signal_names[SIGEMT] = "EMT";
#endif

#if defined (SIGFPE)	/* floating point exception */
  signal_names[SIGFPE] = "FPE";
#endif

#if defined (SIGKILL)	/* kill (cannot be caught or ignored) */
  signal_names[SIGKILL] = "KILL";
#endif

#if defined (SIGBUS)	/* bus error */
  signal_names[SIGBUS] = "BUS";
#endif

#if defined (SIGSEGV)	/* segmentation violation */
  signal_names[SIGSEGV] = "SEGV";
#endif

#if defined (SIGSYS)	/* bad argument to system call */
  signal_names[SIGSYS] = "SYS";
#endif

#if defined (SIGPIPE)	/* write on a pipe with no one to read it */
  signal_names[SIGPIPE] = "PIPE";
#endif

#if defined (SIGALRM)	/* alarm clock */
  signal_names[SIGALRM] = "ALRM";
#endif

#if defined (SIGTERM)	/* software termination signal from kill */
  signal_names[SIGTERM] = "TERM";
#endif

#if defined (SIGURG)	/* urgent condition on IO channel */
  signal_names[SIGURG] = "URG";
#endif

#if defined (SIGSTOP)	/* sendable stop signal not from tty */
  signal_names[SIGSTOP] = "STOP";
#endif

#if defined (SIGTSTP)	/* stop signal from tty */
  signal_names[SIGTSTP] = "TSTP";
#endif

#if defined (SIGCONT)	/* continue a stopped process */
  signal_names[SIGCONT] = "CONT";
#endif

#if defined (SIGCHLD)	/* to parent on child stop or exit */
  signal_names[SIGCHLD] = "CHLD";
#endif

#if defined (SIGTTIN)	/* to readers pgrp upon background tty read */
  signal_names[SIGTTIN] = "TTIN";
#endif

#if defined (SIGTTOU)	/* like TTIN for output if (tp->t_local&LTOSTOP) */
  signal_names[SIGTTOU] = "TTOU";
#endif

#if defined (SIGIO)	/* input/output possible signal */
  signal_names[SIGIO] = "IO";
#endif

#if defined (SIGXCPU)	/* exceeded CPU time limit */
  signal_names[SIGXCPU] = "XCPU";
#endif

#if defined (SIGXFSZ)	/* exceeded file size limit */
  signal_names[SIGXFSZ] = "XFSZ";
#endif

#if defined (SIGVTALRM)	/* virtual time alarm */
  signal_names[SIGVTALRM] = "VTALRM";
#endif

#if defined (SIGPROF)	/* profiling time alarm */
  signal_names[SIGPROF] = "PROF";
#endif

#if defined (SIGWINCH)	/* window changed */
  signal_names[SIGWINCH] = "WINCH";
#endif

/* 4.4 BSD */
#if defined (SIGINFO) && !defined (_SEQUENT_)	/* information request */
  signal_names[SIGINFO] = "INFO";
#endif

#if defined (SIGUSR1)	/* user defined signal 1 */
  signal_names[SIGUSR1] = "USR1";
#endif

#if defined (SIGUSR2)	/* user defined signal 2 */
  signal_names[SIGUSR2] = "USR2";
#endif

#if defined (SIGKILLTHR)	/* BeOS: Kill Thread */
  signal_names[SIGKILLTHR] = "KILLTHR";
#endif

  for (i = 0; i < NSIG; i++)
    if (signal_names[i] == (char *)NULL)
      {
	signal_names[i] = (char *)malloc (18);
	if (signal_names[i])
	  sprintf (signal_names[i], "%d", i);
      }
}

void write_signames(FILE *stream)
{
  register int i;

  fprintf (stream, "/* This file was automatically created by %s.\n",
	   progname);
  fprintf (stream, "   Do not edit.  Edit support/mksignames.c instead. */\n\n");
  fprintf (stream, "#include <signal.h>\n\n");
  fprintf (stream,
	   "/* A translation list so we can be polite to our users. */\n");
  fprintf (stream, "const char *const signal_names[NSIG + 1] = {\n");

  for (i = 0; i <= LASTSIG; i++)
    fprintf (stream, "    \"%s\",\n", signal_names[i]);

  fprintf (stream, "    (char *)0x0\n");
  fprintf (stream, "};\n");
}

int
main(int argc, char **argv)
{
  char *stream_name;
  FILE *stream;

  progname = argv[0];

  if (argc == 1)
    {
      stream_name = "signames.c";
    }
  else if (argc == 2)
    {
      stream_name = argv[1];
    }
  else
    {
      fprintf (stderr, "Usage: %s [output-file]\n", progname);
      exit (1);
    }

  stream = fopen (stream_name, "w");
  if (!stream)
    {
      fprintf (stderr, "%s: %s: cannot open for writing\n",
	       progname, stream_name);
      exit (2);
    }

  initialize_signames ();
  write_signames (stream);
  exit (0);
}
