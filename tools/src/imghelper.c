// Part of the CBSD Project
// Extract info from image file
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include <getopt.h>

#define FALSE 0
#define TRUE 1

/* List of opts */
enum {
	C_START_SIGN,
	C_END_SIGN,
	C_INFILE,
	C_OUTFILE,
	C_HELP,
};

static void
usage(void)
{
	printf("[sys] Extract info from image file\n");
	printf("require: start end infile\n");
	printf("opt: outfile\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	int ch;
	FILE *fp, *fo;
	int c;
	int len_st=0;
	int len_end=0;
	char buf_start[len_st];
	char buf_end[len_end];
	char *end = NULL;
	char *st = NULL;
	char *infile = NULL;
	char *outfile = NULL;
	int win = FALSE;
	int optcode = 0;
	int option_index = 0, ret = 0;

	static struct option long_options[] = {
		{"start", required_argument, 0, C_START_SIGN},
		{"end", required_argument, 0, C_END_SIGN},
		{"infile", required_argument, 0, C_INFILE},
		{"outfile", required_argument, 0, C_OUTFILE},
		{"help", no_argument, 0, C_HELP},
		/* End of options marker */
		{0, 0, 0, 0}
	};

	while (TRUE) {
		optcode = getopt_long_only(argc, argv, "", long_options, &option_index);
		if (optcode == -1)
			break;
		switch(optcode) {
			case C_END_SIGN:
				end = malloc(strlen(optarg) + 1);
				memset(end, 0, strlen(optarg) + 1);
				strcpy(end, optarg);
				break;
			case C_INFILE:
				infile = malloc(strlen(optarg) + 1);
				memset(infile, 0, strlen(optarg) + 1);
				strcpy(infile, optarg);
				break;
			case C_OUTFILE:
				outfile = malloc(strlen(optarg) + 1);
				memset(outfile, 0, strlen(optarg) + 1);
				strcpy(outfile, optarg);
				break;
			case C_START_SIGN:
				st = malloc(strlen(optarg) + 1);
				memset(st, 0, strlen(optarg) + 1);
				strcpy(st, optarg);
				break;
			case C_HELP:
				usage();
		}
	}

	if ((!infile)||(!st)||(!end)) {
		usage();
	}

	if (!outfile) outfile="/dev/stdout";

	len_st=strlen(st);
	len_end=strlen(end);

	off_t start_pos=0;
	off_t stop_pos=0;

	if ((fp = fopen(infile, "r")) == NULL) {
			err(1,"error open: %s\r\n", infile);
	}

	int hammer=0;
	int i=0;

	// first pass - label scan
	while ( hammer!= 2 ) {

		c=getc(fp);
		if (feof(fp)) break;

		switch (hammer) {
			case 0:
				if (c==st[i]) {
					buf_start[i]=c;
					i++;
				} else {
					i=0;
				}

				if (i == len_st) {
					start_pos=ftello(fp) + 1;
					hammer++;
				}
				break;
			case 1:
				if (c==end[i]) {
					buf_end[i]=c;
					i++;
				} else {
					i=0;
				}

				if (i == len_end) {
					stop_pos=ftello(fp) - len_end;
					hammer++;
				}
				break;
			case 2:
				break;
		}
	}

	fclose(fp);

	switch (hammer) {
		case 0:
			err(1,"Start label not found");
			break;
		case 1:
			err(1,"Start label found but end label absent");
			break;
		case 2:
			break;
		default:
			err(1,"Unknown label found");
			break;
	}

	if ( start_pos == stop_pos ) exit(0);

	// second pass
	if ((fp = fopen(infile, "r")) == NULL) {
			err(1,"error open: %s\r\n", infile);
	}

	fseek( fp, start_pos, SEEK_SET );

	if ((fo = fopen(outfile, "w")) == NULL) {
			err(1,"error open: %s\r\n", outfile);
	}

	while (!feof(fp)) {
		start_pos++;
		if ( start_pos == stop_pos ) break;

		c=getc(fp);
		fputc(c, fo);
	}

	free(st);
	free(end);

	fclose(fo);
	fclose(fp);

	return 0;
}

