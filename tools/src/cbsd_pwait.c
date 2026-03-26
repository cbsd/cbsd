#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include <getopt.h>

#define FALSE 0
#define TRUE 1

static int timeout = 0;

/* List of all nodesql */
enum {
	C_PID,
	C_TIMEOUT,
};

static void
usage(void)
{
	printf("Wait for processes to terminate\n");
	printf("require: --pid, --timeout\n");
	printf(
	    "usage: pwait --pid=pid --timeout=0 (in seconds, 0 is infinity)\n");
	exit(EX_USAGE);
}

int
main(int argc, char *argv[])
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

	while (TRUE) {
		optcode = getopt_long(argc, argv, "", long_options,
		    &option_index);
		if (optcode == -1)
			break;
		switch (optcode) {
		case C_PID:
		{
			errno = 0;
			long lp = strtol(optarg, &end, 10);
			if (errno != 0 || end == optarg || *end != '\0') {
				errx(EX_USAGE, "invalid --pid: %s", optarg);
			}
			if (lp <= 0 || lp > INT_MAX) {
				errx(EX_USAGE, "pid out of range: %s", optarg);
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
				errx(EX_USAGE, "invalid --timeout: %s",
				    optarg);
			}
			timeout = (int)lt;
			break;
		}
		}
	} // while

	if (pid <= 0)
		return 0;

	kq = kqueue();
	if (kq == -1)
		err(1, "kqueue");

	EV_SET(&change, (uintptr_t)pid, EVFILT_PROC, EV_ADD, NOTE_EXIT, 0, NULL);
	if (kevent(kq, &change, 1, NULL, 0, NULL) == -1)
		err(1, "kevent register pid %d", (int)pid);

	tv.tv_sec = timeout;
	tv.tv_nsec = 0;
	// tv.tv_usec = 0;

	if (timeout == 0)
		n = kevent(kq, NULL, 0, &event, 1, NULL);
	else
		n = kevent(kq, NULL, 0, &event, 1, &tv);
	if (n == -1)
		err(1, "kevent wait");

	(void)close(kq);
	exit(EX_OK);
}
