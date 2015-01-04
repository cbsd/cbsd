#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <sys/file.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include <getopt.h>

#include "output.h"

#define FALSE 0
#define TRUE 1

static int timeout=0;

/* List of all nodesql */
enum {
	C_PID,
	C_TIMEOUT,
};

int
cbsd_pwait_usage(void)
{
	printf("Wait for processes to terminate\n");
	printf("require: --pid, --timeout\n");
	printf("usage: cbsd_pwait --pid=pid --timeout=0 (in seconds, 0 is infinity)\n");
	return(EX_USAGE);
}

int
cbsd_pwaitcmd(int argc, char **argv)
{
	int kq;
	struct kevent *e;
	int n, i;
	long pid =0;
	char *end;
	int optcode = 0;
	int option_index = 0;
	struct timespec tv;

	struct option long_options[] = {
	    { "pid", required_argument, 0 , C_PID },
	    { "timeout", required_argument, 0 , C_TIMEOUT },
	    /* End of options marker */
	    { 0, 0, 0, 0 }
	};

	if (argc != 3)
		cbsd_pwait_usage();

	while (TRUE) {
	    optcode = getopt_long_only(argc, argv, "", long_options, &option_index);
	    if (optcode == -1) break;
	    switch (optcode) {
		case C_PID:
		    pid = strtol(optarg, &end, 10);
		    break;
		case C_TIMEOUT:
		    timeout=atoi(optarg);
		    break;
	    }
	} //while

	//zero for getopt* variables for next execute
	optarg=NULL;
	optind=0;
	optopt=0;
	opterr=0;
	optreset=0;

	if (pid<=0) return 0;

	kq = kqueue();
	if (kq == -1) {
		out2fmt_flush( "kqueue");
		return 1;
	}

	e = malloc(argc * sizeof(struct kevent));
	if (e == NULL) {
		out2fmt_flush("malloc");
		return 1;
	}
	i=0;

	EV_SET(e + i, pid, EVFILT_PROC, EV_ADD, NOTE_EXIT,0, NULL);
	if (kevent(kq, e + i, 1, NULL, 0, NULL) == -1)
	    out2fmt_flush("%ld", pid);
	else
	    i++;

	tv.tv_sec = timeout;
	tv.tv_nsec = 0;
	//tv.tv_usec = 0;

	while (i > 0) {
		if (timeout==0) n = kevent(kq, NULL, 0, e, i, NULL);
		else n = kevent(kq,NULL, 0, e, i , &tv);
		if (n == -1) {
			out2fmt_flush( "kevent");
			return 1;
		}
		i--;
	}

	return(EX_OK);
}
