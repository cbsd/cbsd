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
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include <getopt.h>

#define FALSE 0
#define TRUE 1

static int	timeout = 0;

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
	printf("usage: pwait --pid=pid --timeout=0 (in seconds, 0 is infinity)\n");
	exit(EX_USAGE);
}

int
main(int argc, char *argv[])
{
	int		kq;
	struct kevent  *e;
	int		verbose = 0;
	int		opt       , n, i, duplicate, status;
	long		pid = 0;
	char           *s, *end;
	int		win = FALSE;
	int		optcode = 0;
	int		option_index = 0, ret = 0;
	int		action = 0;
	struct timespec	tv;

	static struct option long_options[] = {
		{"pid", required_argument, 0, C_PID},
		{"timeout", required_argument, 0, C_TIMEOUT},
		/* End of options marker */
		{0, 0, 0, 0}
	};

	if (argc != 3)
		usage();

	while (TRUE) {
		optcode = getopt_long_only(argc, argv, "", long_options, &option_index);
		if (optcode == -1)
			break;
		switch (optcode) {
		case C_PID:
			pid = strtol(optarg, &end, 10);
			break;
		case C_TIMEOUT:
			timeout = atoi(optarg);
			break;
		}
	} //while

		if (pid <= 0)
		exit(0);

	kq = kqueue();
	if (kq == -1)
		err(1, "kqueue");

	e = malloc(argc * sizeof(struct kevent));
	if (e == NULL)
		err(1, "malloc");

	i = 0;

	EV_SET(e + i, pid, EVFILT_PROC, EV_ADD, NOTE_EXIT, 0, NULL);
	if (kevent(kq, e + i, 1, NULL, 0, NULL) == -1)
		warn("%ld", pid);
	else
		i++;

	tv.tv_sec = timeout;
	tv.tv_nsec = 0;
	//tv.tv_usec = 0;

	while (i > 0) {
		if (timeout == 0)
			n = kevent(kq, NULL, 0, e, i, NULL);
		else
			n = kevent(kq, NULL, 0, e, i, &tv);
		if (n == -1)
			err(1, "kevent");
		i--;
	}

	exit(EX_OK);
}
