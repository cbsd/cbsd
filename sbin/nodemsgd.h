#include <stdarg.h>

#define ERROR_EXIT     1
#define        MAX_FNAME       100     /* max length of internally generated fn */

int cbsddebug=0;
int errmsg(const char *format, ...);
int debugmsg(int level,const char *format, ...);
char *pidfile;

int debugmsg(int level,const char *format, ...)
{
    va_list arg;
    int done;

    if(cbsddebug<level) return 0;
    va_start (arg, format);
    done = vfprintf (stdout, format, arg);
    va_end (arg);
    fflush(stdout);
    return 0;
}

int errmsg(const char *format, ...)
{
   va_list arg;
   int done;

   va_start (arg, format);
   done = vfprintf (stderr, format, arg);
   va_end (arg);
    fflush(stderr);
   return 0;
}
