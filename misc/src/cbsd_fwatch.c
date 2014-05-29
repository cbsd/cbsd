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
#include <malloc_np.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include <getopt.h>

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

int
main(int argc, char **argv)
{
	int		kq;
	struct kevent  *e, kev_r;
	int		n         , i;
	int		optcode = 0;
	int		option_index = 0;
	struct timespec	tv;
	FILE           *fp;
	char           *watchfile = NULL;
	int		timeout = 0;

	struct option	long_options[] = {
		{"file", required_argument, 0, C_FILE},
		{"timeout", required_argument, 0, C_TIMEOUT},
		/* End of options marker */
		{0, 0, 0, 0}
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
	fp = fopen(watchfile, "r");

	if (!fp) {
		printf("Can't open file for mon: %s\n", watchfile);
		free(watchfile);
		return 1;
	}
	kq = kqueue();

	if (kq == -1) {
		fclose(fp);
		free(watchfile);
		printf("kqueue error");
		return 1;
	}
	e = malloc(argc * sizeof(struct kevent));

	if (e == NULL) {
		fclose(fp);
		free(watchfile);
		printf("malloc");
		return 1;
	}
	i = 0;

	//todo:multiple file alteration
	EV_SET(e + i, (*fp)._file, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR, NOTE_DELETE | NOTE_WRITE, 0, NULL);

	if (kevent(kq, e + i, 1, &kev_r, 0, NULL) == -1)
		printf("kevent");
	else
		i++;

	tv.tv_sec = timeout;
	tv.tv_nsec = 0;
	//tv.tv_usec = 0;

	while (i > 0) {
		if (kev_r.fflags & NOTE_WRITE) {
			printf("updated\n");
		} else if (kev_r.fflags & NOTE_DELETE) {
			printf("deleted\n");
		}
		if (timeout == 0)
			n = kevent(kq, NULL, 0, e, i, NULL);
		else
			n = kevent(kq, NULL, 0, e, i, &tv);
		if (n == -1) {
			fclose(fp);
			printf("kevent");
			free(e);
			free(watchfile);
			return 1;
		}
		i--;
	}

	free(e);
	free(watchfile);
	fclose(fp);
	return (EX_OK);
}
