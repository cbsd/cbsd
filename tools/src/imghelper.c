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
	C_PARAM,
	C_NEWVAL,
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
	int prev_c;
	int len_st=0;
	int len_end=0;
	int len_param=0;
	char *end = NULL;
	char *st = NULL;
	char *infile = NULL;
	char *outfile = NULL;
	char *param = NULL;
	char *newval = NULL;
	int win = FALSE;
	int optcode = 0;
	int option_index = 0, ret = 0;
	int is_header=0;		//search for first MByte only
	int find_param=0;               //also find and replace param= by newval
	char *newval_buf;
	off_t total_bytes=0;		//bytes processed

	static struct option long_options[] = {
		{"start", required_argument, 0, C_START_SIGN},
		{"end", required_argument, 0, C_END_SIGN},
		{"infile", required_argument, 0, C_INFILE},
		{"outfile", required_argument, 0, C_OUTFILE},
		{"param", required_argument, 0, C_PARAM},
		{"newval", required_argument, 0, C_NEWVAL},
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
				if (!strcmp(st,"___NCSTART_HEADER=1"))
					is_header=1;		// we are looking header only at the beginning
				break;
			case C_PARAM:
				param = malloc(strlen(optarg) + 1);
				memset(param, 0, strlen(optarg) + 1);
				strcpy(param, optarg);
				find_param=1;
				break;
			case C_NEWVAL:
				newval = malloc(strlen(optarg) + 1);
				memset(newval, 0, strlen(optarg) + 1);
				strcpy(newval, optarg);
				find_param=1;
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
	off_t param_pos=0;

	if (find_param==1) {
		len_param=strlen(param);
		newval_buf=malloc(len_param+strlen(newval)+3);		// +2: extra char for quotes + \0
		printf("Find and replace param mode\r\n");
		if ((fp = fopen(infile, "r+b")) == NULL) {
				err(1,"error open for write: %s\r\n", infile);
		}
	} else {
		if ((fp = fopen(infile, "r")) == NULL) {
				err(1,"error open: %s\r\n", infile);
		}
	}

	char buf_start[len_st];
	char buf_end[len_end];
	char buf_param[len_param];

	int hammer=0;
	int hammer_param=0;
	int i=0;
	int j=0;
	//reset to \r by default
	prev_c=10;

	// first pass - label scan
	while ( hammer!= 2 ) {
		c=getc(fp);
		total_bytes++;
		if (feof(fp)) break;

		if ((is_header==1)&&(total_bytes > 1048576)) {
			fclose(fp);
			printf("no header in first 1048576 bytes. Not CBSD image?\n");
			exit(1);
		}

		switch (hammer) {
			case 0:
				if (c==st[i]) {
					// we only respond if it is the beginning of a new line, stored in prev_c variable
					if ((i==0)&&(prev_c==10)) {
						buf_start[i]=c;
						i++;
					} else if (i!=0) {
						buf_start[i]=c;
						i++;
					}
				} else {
					// sequence is broken, reset
					i=0;
				}

				if (i == len_st) {
					start_pos=ftello(fp) + 1;
					// start sign found on start_pos (%ld)
					hammer++;
				}
				break;
			case 1:
				if (c==end[i]) {
					// we only respond if it is the beginning of a new line, stored in prev_c variable
					if ((i==0)&&(prev_c==10)) {
						//fprintf(stderr,"H2 PREV: [%c][%d]\n",prev_c,prev_c);
						buf_end[i]=c;
						i++;
					} else if (i!=0) {
						buf_end[i]=c;
						i++;
					}
				} else {
					i=0;
				}

				if (find_param==1) {
					if (c==param[j]) {
						buf_param[j]=c;
						j++;
					} else {
						j=0;
					}
					if (j == len_param ) {
						param_pos=ftello(fp) - len_param;
						fseek( fp, param_pos, SEEK_SET );
						sprintf(newval_buf,"%s=\"%s\"\n",param,newval);
						fwrite( newval_buf, strlen(newval_buf), 1, fp );
						find_param=0;
						fclose(fp);
						exit(0);
					}
				}
				if (i == len_end) {
					stop_pos=ftello(fp) - len_end;
					hammer++;
				}
				break;
			case 2:
				break;
		}
		prev_c=c;
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

