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
#include <limits.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include <getopt.h>

#include "output.h"

#define FALSE 0
#define TRUE 1


static int timeout = 0;

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
	printf(
	    "usage: cbsd_pwait --pid=pid --timeout=0 (in seconds, 0 is infinity)\n");
	return(EX_USAGE);
}

int
cbsd_pwaitcmd(int argc, char **argv)
{
	int kq;
	struct kevent change, event;
	int n;
	pid_t pid = -1;
	char *end = NULL;
	int optcode = 0;
	int option_index = 0;
	struct timespec tv;

	static struct option long_options[] = { { "pid", required_argument, 0,
						    C_PID },
		{ "timeout", required_argument, 0, C_TIMEOUT },
		/* End of options marker */
		{ 0, 0, 0, 0 } };

	if (argc != 3) {
		cbsd_pwait_usage();
		return(EX_USAGE);
	}

	while (TRUE) {
		optcode = getopt_long_only(argc, argv, "", long_options,
		    &option_index);
		if (optcode == -1)
			break;
		switch (optcode) {
		case C_PID:
		{
			errno = 0;
			long lp = strtol(optarg, &end, 10);
			if (errno != 0 || end == optarg || *end != '\0') {
				out2fmt_flush("invalid --pid: %s", optarg);
				return 1;
			}
			if (lp <= 0 || lp > INT_MAX) {
				out2fmt_flush("pid out of range: %s", optarg);
				return 1;
			}
			pid = (pid_t)lp;
			break;
		}
		case C_TIMEOUT:
		{
			errno = 0;
			long lt = strtol(optarg, &end, 10);
			if (errno != 0 || end == optarg || *end != '\0' ||
			    lt < 0 || lt > INT_MAX) {
				out2fmt_flush("invalid --timeout: %s", optarg);
				return 1;
			}
			timeout = (int)lt;
			break;
		}
		}
	} // while

	// zero for getopt* variables for next execute
	optarg = NULL;
	optind = 0;
	optopt = 0;
	opterr = 0;
	optreset = 0;

	if (pid <= 0)
		return 0;

	kq = kqueue();
	if (kq == -1) {
		out2fmt_flush("kqueue");
		return 1;
	}

	EV_SET(&change, (uintptr_t)pid, EVFILT_PROC, EV_ADD, NOTE_EXIT, 0, NULL);
	if (kevent(kq, &change, 1, NULL, 0, NULL) == -1) {
		out2fmt_flush("kevent register pid %d", (int)pid);
		return 1;
	}
	tv.tv_sec = timeout;
	tv.tv_nsec = 0;
	// tv.tv_usec = 0;

	if (timeout == 0)
		n = kevent(kq, NULL, 0, &event, 1, NULL);
	else
		n = kevent(kq, NULL, 0, &event, 1, &tv);
	if (n == -1) {
		out2fmt_flush("kevent wait");
		return 1;
	}
	(void)close(kq);
	return(EX_OK);
}
