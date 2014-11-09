// CBSD Project
// See notes in daemon.c for detail
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
	printf("Wait for local.sqlite modification and print this fact to output. Also print dot by timeout\n");
	printf("require: --file, --timeout\n");
	printf("usage: cbsd_dot --file=path_to_file --timeout=0 (in seconds, 0 is infinity)\n");
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
		exit(1);
	}

	tv.tv_sec = timeout;
	tv.tv_nsec = 0;

	for (;;) {
		memset(cmd,0,sizeof(cmd));

		if (timeout == 0)
			nev = kevent(kq, NULL, 0, &ev, 1, NULL);
		else
			nev = kevent(kq, NULL, 0, &ev, 1, &tv);

		if (nev == -1) {
			printf("kevent\n");
			exit(1);
		}

		if (nev == 0) {
			//timeout 
			strcpy(cmd,".");
			//continue;
		} else {
			if (ev.fflags & NOTE_DELETE) {
				strcpy(cmd,"deleted ");
				ev.fflags &= ~NOTE_DELETE;
			}

			if (ev.fflags & NOTE_WRITE) {
				strcpy(cmd,"written ");
				ev.fflags &= ~NOTE_WRITE;
			}

			if (ev.fflags & NOTE_EXTEND) {
				strcpy(cmd,"extended ");
				ev.fflags &= ~NOTE_EXTEND;
			}

			if (ev.fflags & NOTE_ATTRIB) {
				strcpy(cmd,"chmod/chown/utimes ");
				ev.fflags &= ~NOTE_ATTRIB;
			}

			if (ev.fflags & NOTE_LINK) {
				strcpy(cmd,"hardlinked ");
				ev.fflags &= ~NOTE_LINK;
			}

			if (ev.fflags & NOTE_RENAME) {
				strcpy(cmd,"renamed ");
				ev.fflags &= ~NOTE_RENAME;
			}

			if (ev.fflags & NOTE_REVOKE) {
				strcpy(cmd,"revoked ");
				ev.fflags &= ~NOTE_REVOKE;
			}
		}

		fprintf(stdout,"%s\r\n",cmd);
		fflush(stdout);
		}
}

