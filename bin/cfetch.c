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
#include <unistd.h>

static int	fetch_files(char *, char *);

int usage()
{
	printf("fetch remove file via http[s]\n");
	printf("require: -u url -o out\n\n");
	printf("Example: cbsd cfetch -u url -o /tmp/out\n");
	exit(0);
}

int
main(int argc, char *argv[])
{
	int		c;
	char           *url = NULL, *fout = NULL;

	if (!strcmp(argv[1], "--help"))
		usage();

	while (1) {
		c = getopt(argc, argv, "u:o:");
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
	off_t		total_bytes, current_bytes, fsize;
	uint8_t		block  [4096];
	size_t		chunk;
	int		progress  , last_progress;
	int		nsuccess = 0;	/* Number of files successfully
					 * downloaded */
	int		lprg = 0;

	progress = 0;
	total_bytes = 0;
	if (fetchStatURL(urls, &ustat, "") == 0 && ustat.size > 0)
		total_bytes += ustat.size;

	fetchTimeout = 30;

	fetch_out = fetchXGetURL(urls, &ustat, "");
	if (fetch_out == NULL)
		return 1;
	file_out = fopen(fout, "w+");

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
