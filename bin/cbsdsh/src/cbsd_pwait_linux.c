#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/signalfd.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

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
	int epoll_fd, sfd;
	sigset_t mask;
	struct epoll_event ev, events[1];
	int n;
	pid_t pid = -1;
	char *end = NULL;
	int optcode = 0;
	int option_index = 0;
	struct itimerspec timeout_ts;
	int timer_fd = -1;

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
				out1fmt("invalid --pid: %s", optarg);
				return 1;
			}
			if (lp <= 0 || lp > INT_MAX) {
				out1fmt("pid out of range: %s", optarg);
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
				out1fmt("invalid --timeout: %s", optarg);
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

	if (pid <= 0)
		return 0;

	// Check if process exists
	if (kill(pid, 0) == -1) {
		if (errno == ESRCH) {
			// Process doesn't exist, already terminated
			return(EX_OK);
		}
		out1fmt("kill(pid %d, 0) failed", (int)pid);
		return 1;
	}

	// Create epoll instance
	epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if (epoll_fd == -1) {
		out1fmt("epoll_create1");
		return 1;
	}

	// Block SIGCHLD and create signalfd
	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
		out1fmt("sigprocmask");
		close(epoll_fd);
		return 1;
	}

	sfd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
	if (sfd == -1) {
		out1fmt("signalfd");
		sigprocmask(SIG_UNBLOCK, &mask, NULL);
		close(epoll_fd);
		return 1;
	}

	// Add signalfd to epoll
	ev.events = EPOLLIN;
	ev.data.fd = sfd;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sfd, &ev) == -1) {
		out1fmt("epoll_ctl");
		close(sfd);
		sigprocmask(SIG_UNBLOCK, &mask, NULL);
		close(epoll_fd);
		return 1;
	}

	// Setup timer if timeout > 0
	if (timeout > 0) {
		timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
		if (timer_fd == -1) {
			out1fmt("timerfd_create");
			close(sfd);
			sigprocmask(SIG_UNBLOCK, &mask, NULL);
			close(epoll_fd);
			return 1;
		}

		timeout_ts.it_value.tv_sec = timeout;
		timeout_ts.it_value.tv_nsec = 0;
		timeout_ts.it_interval.tv_sec = 0;
		timeout_ts.it_interval.tv_nsec = 0;

		if (timerfd_settime(timer_fd, 0, &timeout_ts, NULL) == -1) {
			out1fmt("timerfd_settime");
			close(timer_fd);
			close(sfd);
			sigprocmask(SIG_UNBLOCK, &mask, NULL);
			close(epoll_fd);
			return 1;
		}

		ev.events = EPOLLIN;
		ev.data.fd = timer_fd;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &ev) == -1) {
			out1fmt("epoll_ctl timer");
			close(timer_fd);
			close(sfd);
			sigprocmask(SIG_UNBLOCK, &mask, NULL);
			close(epoll_fd);
			return 1;
		}
	}

	// Wait for events
	n = 0;
	while (TRUE) {
		// Wait for signal or timeout
		int epoll_timeout;
		if (timeout == 0) {
			epoll_timeout = -1; // Wait indefinitely
		} else {
			epoll_timeout = timeout * 1000; // Convert to milliseconds
		}

		int ready = epoll_wait(epoll_fd, events, 1, epoll_timeout);

		if (ready == -1) {
			if (errno == EINTR) {
				// Interrupted by signal, check if process still exists
				if (kill(pid, 0) == -1 && errno == ESRCH) {
					n = 1; // Process terminated
					break;
				}
				continue;
			}
			out1fmt("epoll_wait");
			n = -1;
			break;
		}

		if (ready == 0) {
			// Timeout occurred
			// Check if process still exists (might have exited just before timeout)
			if (kill(pid, 0) == -1 && errno == ESRCH) {
				n = 1; // Process terminated
				break;
			}
			n = 0; // Timeout reached
			break;
		}

		// Check which event occurred
		if (events[0].data.fd == sfd) {
			// SIGCHLD received, check if our process exited
			struct signalfd_siginfo fdsi;
			ssize_t s = read(sfd, &fdsi, sizeof(fdsi));
			if (s == sizeof(fdsi)) {
				// Reap all children to avoid zombies and find our process
				pid_t waited_pid;
				int status;
				while ((waited_pid = waitpid(-1, &status, WNOHANG)) > 0) {
					if (waited_pid == pid) {
						// Our process terminated
						n = 1;
						break;
					}
				}
				if (waited_pid == pid || (waited_pid == -1 && errno == ECHILD)) {
					// Check one more time if our process exists
					if (kill(pid, 0) == -1 && errno == ESRCH) {
						n = 1;
						break;
					}
				}
				// Not our process or still running, continue waiting
				continue;
			}
		} else if (timer_fd != -1 && events[0].data.fd == timer_fd) {
			// Timeout occurred
			uint64_t expirations;
			read(timer_fd, &expirations, sizeof(expirations));
			// Check one more time if process exists
			if (kill(pid, 0) == -1 && errno == ESRCH) {
				n = 1;
				break;
			}
			n = 0;
			break;
		}
	}

	// Cleanup
	if (timer_fd != -1) {
		close(timer_fd);
	}
	close(sfd);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
	close(epoll_fd);

	if (n == -1) {
		return 1;
	}

	if (n == 0 && timeout > 0) {
		// Timeout occurred
		return 1;
	}

	return(EX_OK);
}
