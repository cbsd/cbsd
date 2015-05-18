#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <err.h>
#include <getopt.h>

#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <sysexits.h>

#define FALSE 0
#define TRUE 1

/* List of all nodesql */
enum {
	C_FILE,
	C_TIMEOUT,
};

int
cbsd_fwatch_usage(void)
{
	printf("Wait for file modification to terminate\n");
	printf("require: --file, --timeout\n");
	printf("usage: cbsd_fwatch --file=path_to_file --timeout=0 (in seconds, 0 is infinity)\n");
	return (EX_USAGE);
}



int main(int argc, char *argv[])
{
	int fd, kq, nev;
	struct kevent ev;
//	static const struct timespec tout = { 1, 0 };

	int optcode = 0;
	int option_index = 0;
	struct timespec tv;
	char *watchfile = NULL;
	int timeout = 10;
	char cmd[10];

	struct option   long_options[] = {
		{ "file", required_argument, 0, C_FILE },
		{ "timeout", required_argument, 0, C_TIMEOUT },
		/* End of options marker */
		{ 0, 0, 0, 0 }
	};

	if (argc < 2) {
		cbsd_fwatch_usage();
		return 0;
	}

	while (TRUE) {
		optcode = getopt_long_only(argc, argv, "", long_options, &option_index);
		if (optcode == -1)
			break;
		switch (optcode) {
			case C_FILE:
				watchfile = malloc(strlen(optarg) + 1);
				memset(watchfile, 0, strlen(optarg) + 1);
				strcpy(watchfile, optarg);
			break;
			case C_TIMEOUT:
				timeout = atoi(optarg);
			break;
		}
	} //while

	//zero for getopt *variables for next execute
	optarg = NULL;
	optind = 0;
	optopt = 0;
	opterr = 0;
	optreset = 0;

	if (!watchfile) {
		cbsd_fwatch_usage();
		return 1;
	}


	if ((fd = open(watchfile, O_RDONLY)) == -1) {
		printf("Cannot open: %s\n", watchfile);
		exit(1);
		}

	if ((kq = kqueue()) == -1) {
		printf("Cannot create kqueue\n");
		exit(1);
	}

	EV_SET(&ev, fd, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR,
		NOTE_DELETE|NOTE_WRITE|NOTE_EXTEND|NOTE_ATTRIB|NOTE_LINK|
		NOTE_RENAME|NOTE_REVOKE, 0, 0);

	if (kevent(kq, &ev, 1, NULL, 0, NULL) == -1) {
		printf("kevent\n");
		close(kq);
		exit(1);
	}

	tv.tv_sec = timeout;
	tv.tv_nsec = 0;

	memset(cmd,0,sizeof(cmd));

	if (timeout == 0)
		nev = kevent(kq, NULL, 0, &ev, 1, NULL);
	else
		nev = kevent(kq, NULL, 0, &ev, 1, &tv);

	if (nev == -1) {
		printf("kevent\n");
		close(kq);
		exit(1);
	}

	close(kq);

	if (nev != 0) {
		if (ev.fflags & NOTE_DELETE) {
			printf("deleted\n");
			ev.fflags &= ~NOTE_DELETE;
		}

		if (ev.fflags & NOTE_WRITE) {
			printf("written\n");
			ev.fflags &= ~NOTE_WRITE;
		}

		if (ev.fflags & NOTE_EXTEND) {
			printf("extended\n");
			ev.fflags &= ~NOTE_EXTEND;
		}

		if (ev.fflags & NOTE_ATTRIB) {
			printf("chmod/chown/utimes\n");
			ev.fflags &= ~NOTE_ATTRIB;
		}

		if (ev.fflags & NOTE_LINK) {
			printf("hardlinked\n");
			ev.fflags &= ~NOTE_LINK;
		}

		if (ev.fflags & NOTE_RENAME) {
			printf("renamed\n");
			ev.fflags &= ~NOTE_RENAME;
		}

		if (ev.fflags & NOTE_REVOKE) {
			printf("revoked\n");
			ev.fflags &= ~NOTE_REVOKE;
		}
	}

	free(watchfile);
	return 0;
}
