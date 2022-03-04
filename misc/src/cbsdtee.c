// CBSD Project, 2013-2019
// modified tee(1) tools to store processed bytes into file via -f <filename>
// cbsdtee v0.2
#include <sys/stat.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

typedef struct list {
	struct list *next;
	int fd;
	const char *name;
} LIST;
static LIST *head;

static void add(int /*fd*/, const char * /*name*/);
static void usage(void);

int
generate_random_pct(int lower, int upper)
{
	int i;
	int num = 0;
	char random_seed = 0;
	FILE *fp;

	fp = fopen("/dev/random", "r");
	if (!fp) {
		srand(time(0));
	} else {
		random_seed = getc(fp);
		fclose(fp);
		srand(random_seed);
	}
	num = (rand() % (upper - lower + 1)) + lower;

	if (num < 0) {
		num = 0;
	}

	if (num > 100) {
		num = 99; // ;-)
	}

	return num;
}

int
main(int argc, char *argv[])
{
	LIST *p;
	int n;
	int fd;
	int rval;
	int wval;
	char *bp;
	int ch;
	int exitval;
	char *buf;
	off_t received = 0;
	off_t bytes_expected = 0;
	char *filename = NULL;
	FILE *fp;
	off_t part = 0;
	off_t stage_part[10];
	int progress = 0;
	int rounded_progress = 0;
	unsigned int cur_part = 1;

	srand(time(0)); // for rounded percent
#define BSIZE (8 * 1024)

	while ((ch = getopt(argc, argv, "f:e:")) != -1) {
		switch ((char)ch) {
		case 'e':
			bytes_expected = atoll(optarg);
			break;
		case 'f':
			filename = optarg;
			break;
		case '?':
		default:
			usage();
		}
	}
	argv += optind;
	argc -= optind;

	if ((!filename) && (bytes_expected == 0)) {
		usage();
	}

	if ((buf = malloc(BSIZE)) == NULL) {
		err(1, "malloc");
	}

	if (bytes_expected > 0) {
		fprintf(stderr, "WIP: [0%%");
		part = bytes_expected / 10;
		for (n = 0; n < 10; n++) {
			stage_part[n] = part * n;
		}
	}
	add(STDOUT_FILENO, "stdout");

	for (exitval = 0; *argv; ++argv) {
		if ((fd = open(*argv, O_WRONLY | O_CREAT | O_TRUNC,
			 DEFFILEMODE)) < 0) {
			warn("%s", *argv);
			exitval = 1;
		} else {
			add(fd, *argv);
		}
	}

	while ((rval = read(STDIN_FILENO, buf, BSIZE)) > 0) {
		for (p = head; p; p = p->next) {
			n = rval;
			bp = buf;
			do {
				if ((wval = write(p->fd, bp, n)) == -1) {
					warn("%s", p->name);
					exitval = 1;
					break;
				}
				bp += wval;
				received += n;
				if (bytes_expected > 0) {
					if (received > stage_part[cur_part]) {
						progress = cur_part * 10;
						rounded_progress =
						    generate_random_pct(
							progress - 5,
							progress +
							    5); // round
								// progress
						fprintf(stderr, "...%d%%",
						    rounded_progress);
						cur_part++;
					}
				}
			} while (n -= wval);
		}
	}
	if (rval < 0) {
		err(1, "read");
	}

	if (filename) {
		fp = fopen(filename, "w");
		if (!fp) {
			warn("cbsdtee: unable to open %s for writing\n",
			    filename);
		} else {
			fprintf(fp, "img_flat_size=\"%ld\"\n", received);
			fclose(fp);
		}
	}
	if (bytes_expected > 0) {
		fprintf(stderr, "...100%%]\n");
	}
	exit(exitval);
}

static void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: cbsdtee -f file [ -e <bytes expected> ]\n");
	exit(1);
}

static void
add(int fd, const char *name)
{
	LIST *p;
	if ((p = malloc(sizeof(LIST))) == NULL) {
		err(1, "malloc");
	}
	p->fd = fd;
	p->name = name;
	p->next = head;
	head = p;
}
