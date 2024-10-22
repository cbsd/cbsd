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
#include <signal.h> // sigaction(), sigsuspend(), sig*()
#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>

#include <curl/curl.h>

static int fetch_files(char * /*urls*/, char * /*fout*/);
void handle_signal(int /*signal*/);
void handle_sigalrm(int /*signal*/);

CURL *curl_handle;

off_t current_bytes = 0;
int speedtest = 0;

int start_time = 0;
int end_time = 0;

int
usage()
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
	int c;
	char *url = NULL;
	char *fout = NULL;

	if (argc < 2) {
		usage();
	}

	if (!strcmp(argv[1], "--help")) {
		usage();
	}

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
		if (c == -1) {
			break;
		}
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

	if ((!url) || (!fout)) {
		usage();
	}

	c = fetch_files(url, fout);
	return c;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
	return written;
}


static int
fetch_files(char *urls, char *fout)
{
	CURLcode res;

	FILE *fetch_out;
//	fetchIO *fetch_out;
	FILE *file_out;
//	struct url_stat ustat;
	off_t total_bytes;
	off_t fsize = 0;
	uint8_t block[4096];
	size_t chunk;
	int progress;
	int last_progress;
	int nsuccess = 0; /* Number of files successfully
			   * downloaded */
	int lprg = 0;
	struct sigaction sa;
	sigset_t mask;
	struct timeval now_time;
	int diff_time = 0;

	sa.sa_handler = &handle_sigalrm; // Intercept and ignore SIGALRM
	sa.sa_flags = SA_RESETHAND; // Remove the handler after first signal
	sigfillset(&sa.sa_mask);
	sigaction(SIGALRM, &sa, NULL);

	// Get the current signal mask
	sigprocmask(0, NULL, &mask);

	// Unblock SIGALRM
	sigdelset(&mask, SIGALRM);

	//	// Wait with this mask
//		alarm(1);
//		sigsuspend(&mask);

	progress = 0;
	total_bytes = 0;

	gettimeofday(&now_time, NULL);
	start_time = (time_t)now_time.tv_sec;


//	if (fetchStatURL(urls, &ustat, "") == 0 && ustat.size > 0) {
//		total_bytes += ustat.size;
//	}


	curl_global_init(CURL_GLOBAL_ALL);

	/* init the curl session */
	curl_handle = curl_easy_init();

	/* set URL to get here */
	curl_easy_setopt(curl_handle, CURLOPT_URL, urls);

	/* Switch on full protocol/debug output while testing */
	curl_easy_setopt(curl_handle, CURLOPT_HEADER, 0);
	curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1);

	// not fork for FTP? e.g. when
	// ./a.out -u ftp://ftp.de.freebsd.org/pub/FreeBSD/releases/amd64/amd64/ISO-IMAGES/14.0/FreeBSD-14.0-RELEASE-amd64-disc1.iso.xz -o /tmp/x -s1
	// Content-Length: 790216108
	// Accept-ranges: bytes
	// CURL_FTP_HTTPSTYLE_HEAD ifdef. - check out lib/ftp.c
	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L);
	/* Perform the request */
	res = curl_easy_perform(curl_handle);

	if(!res) {
		/* check the size */
		double cl;
		res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl);
		if(!res) {
			//printf("Size: %.0f\n", cl);
			total_bytes=cl;
		}
	}

	/* disable progress meter, set to 0L to enable it */
	//curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);

	/* send all data to this function  */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 0);

	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);

	/* follow HTTP 3xx redirects  */
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

	if (speedtest == 1) {
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 5);		// timeout for the URL to download
	}
//..	 else {
//		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 20);		// timeout for the URL to download
//	}

//	fetch_out = fetchXGetURL(urls, &ustat, "");
//	if (fetch_out == NULL) {
//		return 1;
//	}

//	file_out = fopen(fout, "w+");

	fetch_out = fopen(fout, "wb");

//	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);

	if(fetch_out) {
		if (speedtest != 1) {
			printf("Size: %d Mb\n", ((int)total_bytes / 1024 / 1024));
		}

		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, fetch_out);
		/* get it! */
		curl_easy_perform(curl_handle);
		/* close the header file */
		fclose(fetch_out);
	}

	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);
	curl_global_cleanup();

	current_bytes = 0;

/*
	current_bytes = 0;
	while ((chunk = fread(block, 1, sizeof(block), fetch_out)) > 0) {
		if (fwrite(block, 1, chunk, file_out) < chunk) {
			break;
		}

		current_bytes += chunk;
		fsize += chunk;

		if (total_bytes > 0) {
			last_progress = progress;
			progress = (current_bytes * 100) / total_bytes;
		}
		if ((progress % 10 == 0) && (lprg != progress)) {
			lprg = progress;
			if (speedtest != 1) {
				printf("Progress: %d%% \n", progress);
			}
		}
	}

	if (ustat.size > 0 && fsize < ustat.size) {
		if (fetchLastErrCode == 0) {
			// small chunk
			fclose(fetch_out);
//			fetchIO_close(fetch_out);
			fclose(file_out);
			return 0;
		}
	} else {
		nsuccess++;
	}

	fclose(fetch_out);
//	fetchIO_close(fetch_out);
	fclose(file_out);
*/
	gettimeofday(&now_time, NULL);
	end_time = (time_t)now_time.tv_sec;
	diff_time = end_time - start_time;
	if (diff_time == 0) {
		diff_time = 1;
	}

	/*
	 * Please note that printf et al. are NOT safe to use in signal
	 * handlers. Look for async safe functions.
	 */
	if (speedtest == 1) {
		double dl;
		 CURLcode res;
		/* check the size */
		res = curl_easy_getinfo(curl_handle, CURLINFO_SIZE_DOWNLOAD_T, &dl);
		if(!res) {
			printf("%.0f\n", dl / diff_time);
		}
	}

	return (0);
}

void
handle_signal(int signal)
{
	const char *signal_name;
	sigset_t pending;
	struct timeval now_time;
	int diff_time = 0;

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

	gettimeofday(&now_time, NULL);
	end_time = (time_t)now_time.tv_sec;
	diff_time = end_time - start_time;
	if (diff_time == 0) {
		diff_time = 1;
	}

	if (speedtest == 1) {
		double dl;
		 CURLcode res;
		/* check the size */
		res = curl_easy_getinfo(curl_handle, CURLINFO_SIZE_DOWNLOAD_T, &dl);
		if(!res) {
			printf("%.0f\n", dl / diff_time);
		}
	}

	exit(0);
}

void
handle_sigalrm(int signal)
{
	if (signal != SIGALRM) {
		fprintf(stderr, "Caught wrong signal: %d\n", signal);
	}

	printf("Got sigalrm, do_sleep() will end\n");
}
