// This is implemenation of build-in "about" command
// For identification of correct CBSD shell only

#include <stdio.h>
#include <sys/param.h> //MAXPATHLEN2Q

#include <paths.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/wait.h> /* For WIFSIGNALED(status) */
#include <errno.h>

/*
 * Evaluate a command.
 */

#include "shell.h"
#include "nodes.h"
#include "syntax.h"
#include "expand.h"
#include "parser.h"
#include "jobs.h"
#include "eval.h"
#include "builtins.h"
#include "options.h"
#include "exec.h"
#include "redir.h"
#include "input.h"
#include "output.h"
#include "trap.h"
#include "var.h"
#include "memalloc.h"
#include "error.h"
#include "show.h"
#include "mystring.h"
#ifndef NO_HISTORY
#include "myhistedit.h"
#endif


int  
jlscmd(int argc, char **argv)
{
//char *buffer;
//    char buffer[MAXPATHLEN];
//    buffer[0] = 0;
//    int offset = 0;
//
//    while(argv++,--argc) {
//	int toWrite = MAXPATHLEN-offset;
//	int written = snprintf(buffer+offset, toWrite, "%s ", *argv);
//
//    	if(toWrite < written) {
//             break;
//        }
//	offset += written;
//    }
//buffer="date";
//out2fmt_flush("%s\n",pathval());
shellexec(argv + 1, environment(), pathval(), 0);
//shellexec(argv + 1, envp, path,0);

// system(buffer);
return 0;
}

