#ifdef MAIN_PROGRAM
# define XTRN
# define INIT(x) = x
#else
# define XTRN extern
# define INIT(x)
#endif

XTRN const char *copyright[]
#ifdef MAIN_PROGRAM
	= {
		"@(#) ISC Cron V4.1",
		"@(#) Copyright 1988,1989,1990,1993,1994 by Paul Vixie",
		"@(#) Copyright 1997,2000 by Internet Software Consortium, Inc.",
		"@(#) Copyright 2004 by Internet Systems Consortium, Inc.",
		"@(#) All rights reserved",
		NULL
	}
#endif
	;

XTRN const char *MonthNames[]
#ifdef MAIN_PROGRAM
	= {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
		NULL
	}
#endif
	;

XTRN const char *DowNames[]
#ifdef MAIN_PROGRAM
	= {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun",
		NULL
	}
#endif
	;

XTRN char	*ProgramName INIT("amnesia");
XTRN int	LineNumber INIT(0);
XTRN time_t	StartTime INIT(0);
XTRN int	NoFork INIT(0);

#if DEBUGGING
XTRN int	DebugFlags INIT(0);
XTRN const char *DebugFlagNames[]
#ifdef MAIN_PROGRAM
	= {
		"ext", "sch", "proc", "pars", "load", "misc", "test", "bit",
		NULL
	}
#endif
	;
#else
#define	DebugFlags	0
#endif /* DEBUGGING */
