// TODO
// visualisation method:
//....(just progress)
// nbytes / mbytes
// percent
// or quiet
// eg:fetch - u url - o localfile 3(3 dot)
// eg:fetch url localfile S(size / allsize)
// eg:fetch url localfile P(percent)
// eg:fetch url localfile Q(quiet)
// eg:fetch url localfile E(printf errorcode to stdout)
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fetch.h>
#include <signal.h> // sigaction(), sigsuspend(), sig*()
#include <unistd.h>

static int	fetch_files(char *, char *);
void		handle_signal(int);
void		handle_sigalrm(int);

off_t		current_bytes=0;
int		speedtest=0;

int usage()
{
	printf("fetch remove file via http[s]\n");
	printf("require: -u url -o out\n\n");
	printf("optional: -s <only print s url -o out\n\n");
	printf("Example: cbsd cfetch -u url -o /tmp/out\n");
	exit(0);
}

int
main(int argc, char *argv[])
{
	int		c;
	char		*url = NULL, *fout = NULL;

	if(argc<2) usage();

	if (!strcmp(argv[1], "--help"))
		usage();

	struct sigaction sa;
	// Setup the sighub handler
	sa.sa_handler = &handle_signal;

	// Restart the system call, if at all possible
	sa.sa_flags = SA_RESTART;

	// Block every signal during the handler
	sigfillset(&sa.sa_mask);

	// Intercept SIGHUP and SIGINT
	if (sigaction(SIGHUP, &sa, NULL) == -1) {
		perror("Error: cannot handle SIGHUP"); // Should not happen
	}

	while (1) {
		c = getopt(argc, argv, "u:o:s:");
		/* Detect the end of the options. */
		if (c == -1)
			break;
		switch (c) {
		case 'u':
			url = optarg;
			break;
		case 'o':
			fout = optarg;
			break;
		case 's':
			speedtest = 1;
			break;
		}
	}

	if ((!url) || (!fout))
		usage();

	c = fetch_files(url, fout);
	return c;
}

static int
fetch_files(char *urls, char *fout)
{
	FILE           *fetch_out, *file_out;
	struct url_stat	ustat;
	off_t		total_bytes, fsize;
	uint8_t		block  [4096];
	size_t		chunk;
	int		progress  , last_progress;
	int		nsuccess = 0;	/* Number of files successfully
					 * downloaded */
	int		lprg = 0;
	struct sigaction sa;
	sigset_t mask;

	sa.sa_handler = &handle_sigalrm; // Intercept and ignore SIGALRM
	sa.sa_flags = SA_RESETHAND; // Remove the handler after first signal
	sigfillset(&sa.sa_mask);
	sigaction(SIGALRM, &sa, NULL);

	// Get the current signal mask
	sigprocmask(0, NULL, &mask);

	// Unblock SIGALRM
	sigdelset(&mask, SIGALRM);

//	// Wait with this mask
//	alarm(1);
//	sigsuspend(&mask);

	progress = 0;
	total_bytes = 0;
	if (fetchStatURL(urls, &ustat, "") == 0 && ustat.size > 0)
		total_bytes += ustat.size;

	fetchTimeout = 30;

	fetch_out = fetchXGetURL(urls, &ustat, "");
	if (fetch_out == NULL)
		return 1;

	file_out = fopen(fout, "w+");

	if (speedtest!=1)
		printf("Size: %d Mb\n", ((int)total_bytes / 1024 / 1024));

	current_bytes = 0;
	while ((chunk = fread(block, 1, sizeof(block), fetch_out)) > 0) {
		if (fwrite(block, 1, chunk, file_out) < chunk)
			break;

		current_bytes += chunk;
		fsize += chunk;

		if (total_bytes > 0) {
			last_progress = progress;
			progress = (current_bytes * 100) / total_bytes;
		}
		if ((progress % 10 == 0) && (lprg != progress)) {
			lprg = progress;
			if (speedtest!=1)
				printf("Progress: %d%% \n", progress);
		}
	}

	if (ustat.size > 0 && fsize < ustat.size) {
		if (fetchLastErrCode == 0) {
			//small chunk
			fclose(fetch_out);
			fclose(file_out);
			return 0;
		}
	} else
		nsuccess++;

	fclose(fetch_out);
	fclose(file_out);
	return (0);
}


void handle_signal(int signal) {
	const char *signal_name;
	sigset_t pending;

	// Find out which signal we're handling
	switch (signal) {
		case SIGHUP:
			signal_name = "SIGHUP";
			break;
		case SIGUSR1:
			signal_name = "SIGUSR1";
			break;
		case SIGINT:
			printf("Caught SIGINT, exiting now\n");
			exit(0);
		default:
			fprintf(stderr, "Caught wrong signal: %d\n", signal);
			return;
	}

	/*
	 * Please note that printf et al. are NOT safe to use in signal handlers.
	 * Look for async safe functions.
	 */
	printf("%ld\n",current_bytes);

	exit(0);
}


void handle_sigalrm(int signal) {
	if (signal != SIGALRM) {
		fprintf(stderr, "Caught wrong signal: %d\n", signal);
	}

	printf("Got sigalrm, do_sleep() will end\n");
}
