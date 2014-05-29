// Part of CBSD Project
// mailto:olevole at olevole dot ru
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>
#include <stdarg.h> //for debugmsg/errmsg

#include "gentools.h"

int 
debugmsg(int level, const char *format,...)
{
	va_list		arg;
	int		done;

	if (debug < level)
		return 0;
	va_start(arg, format);
	done = vfprintf(stdout, format, arg);
	va_end(arg);

	return 0;
}

int 
errmsg(const char *format,...)
{
	va_list		arg;
	int		done;

	va_start(arg, format);
	done = vfprintf(stderr, format, arg);
	va_end(arg);

	return 0;
}
