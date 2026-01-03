#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <err.h>
#include <getopt.h>
#include <poll.h>
#include <sys/inotify.h>
#include <limits.h>

#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <sysexits.h>

#include "output.h"

#define FALSE 0
#define TRUE 1

#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))


/* List of all nodesql */
enum {
	C_FILE,
	C_TIMEOUT,
};

int
cbsd_fwatch_usage(void)
{
	out1fmt("Wait for file modification to terminate\n");
	out1fmt("require: --file, --timeout\n");
	out1fmt(
	    "usage: cbsd_fwatch --file=path_to_file --timeout=0 (in seconds, 0 is infinity)\n");
	return (EX_USAGE);
}

/* Display information from inotify_event structure */
static void displayInotifyEvent(struct inotify_event *i)
{
//	out1fmt("    wd =%2d; ", i->wd);
//	if (i->cookie > 0)
//		out1fmt("cookie =%4d; ", i->cookie);

//	out1fmt("mask =\n");
	if (i->mask & IN_ACCESS)        out1fmt("IN_ACCESS\n");
	if (i->mask & IN_ATTRIB)        out1fmt("IN_ATTRIB\n");
	if (i->mask & IN_CLOSE_NOWRITE) out1fmt("IN_CLOSE_NOWRITE\n");
	if (i->mask & IN_CLOSE_WRITE)   out1fmt("IN_CLOSE_WRITE\n");
	if (i->mask & IN_CREATE)        out1fmt("IN_CREATE\n");
	if (i->mask & IN_DELETE)        out1fmt("deleted\n");
	if (i->mask & IN_DELETE_SELF)   out1fmt("deleted\n");
	if (i->mask & IN_IGNORED)       out1fmt("IN_IGNORED\n");
	if (i->mask & IN_ISDIR)         out1fmt("IN_ISDIR\n");
	if (i->mask & IN_MODIFY)        out1fmt("written\n");
	if (i->mask & IN_MOVE_SELF)     out1fmt("renamed\n");
	if (i->mask & IN_MOVED_FROM)    out1fmt("renamed\n");
	if (i->mask & IN_MOVED_TO)      out1fmt("renamed\n");
	if (i->mask & IN_OPEN)          out1fmt("IN_OPEN\n");
	if (i->mask & IN_Q_OVERFLOW)    out1fmt("IN_Q_OVERFLOW\n");
	if (i->mask & IN_UNMOUNT)       out1fmt("IN_UNMOUNT\n");

//	if (i->len > 0)
//		out1fmt("        name = %s\n", i->name);
}

int
cbsd_fwatchcmd(int argc, char *argv[])
{
	int fd;
	int ret;
	fd_set rfds;
	int kq;
	int nev;
	int inotifyFd, j;
	//	static const struct timespec tout = { 1, 0 };
	struct inotify_event *event;
	ssize_t numRead;
	char buf[BUF_LEN] __attribute__ ((aligned(8)));
	char *p;
//	struct pollfd fds[2];
	struct pollfd fds[1];
	nfds_t nfds;
	int poll_num;
	int *wd;

	int optcode = 0;
	int option_index = 0;
	char *watchfile = NULL;
	int timeout = 10;
	char cmd[10];

	struct option long_options[] = { { "file", required_argument, 0,
					     C_FILE },
		{ "timeout", required_argument, 0, C_TIMEOUT },
		/* End of options marker */
		{ 0, 0, 0, 0 } };

	if (argc < 2) {
		cbsd_fwatch_usage();
		return 0;
	}

	while (TRUE) {
		optcode = getopt_long_only(argc, argv, "", long_options,
		    &option_index);
		if (optcode == -1) {
			break;
		}
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
	} // while

	// zero for getopt *variables for next execute
	optarg = NULL;
	optind = 0;
	optopt = 0;
	opterr = 0;

	if (!watchfile) {
		cbsd_fwatch_usage();
		return 1;
	}

	inotifyFd = inotify_init1(IN_NONBLOCK);

	if (inotifyFd == -1) {
		out1fmt("inotify_init1");
		return(1);
	}

	wd = calloc(argc, sizeof(int));
	if (wd == NULL) {
		perror("calloc");
		return(EXIT_FAILURE);
	}


//	wd[0] = inotify_add_watch(inotifyFd, watchfile, IN_ALL_EVENTS);
	wd[0] = inotify_add_watch(inotifyFd, watchfile, IN_OPEN | IN_CLOSE);
	if (wd[0] == -1) {
		out1fmt("inotify_add_watch");
		return(1);
	}

	/* Prepare for polling. */
//	nfds = 2;
//	fds[0].fd = STDIN_FILENO;       /* Console input */
//	fds[0].events = POLLIN;
//	fds[1].fd = inotifyFd;                 /* Inotify input */
//	fds[1].events = POLLIN;

	nfds = 1;
	fds[0].fd = inotifyFd;                 /* Inotify input */
	fds[0].events = POLLIN;

	while (TRUE) {
		poll_num = poll(fds, nfds, 1000 * timeout);
		if ( poll_num == 0 ) {
			// timeout
			close(inotifyFd);
			free(wd);
			return(0);
		}

		if ( poll_num == -1 ) {
			if (errno == EINTR)
				continue;
			out1fmt("poll error");
			close(inotifyFd);
			free(wd);
			return(EXIT_FAILURE);
		}
		break;
	}

	numRead = read(inotifyFd, buf, BUF_LEN);
	if (numRead == 0) {
		out1fmt("read() from inotify fd returned 0!");
		close(inotifyFd);
		free(wd);
		return(1);
	}
	if (numRead == -1) {
		out1fmt("read");
		close(inotifyFd);
		free(wd);
		return(1);
	}

	out1fmt("Read %ld bytes from inotify fd\n", (long) numRead);

	/* Process all of the events in buffer returned by read() */

	for (p = buf; p < buf + numRead; ) {
		event = (struct inotify_event *) p;
		displayInotifyEvent(event);
		p += sizeof(struct inotify_event) + event->len;
	}


	close(inotifyFd);
	free(wd);
	return(0);
}
