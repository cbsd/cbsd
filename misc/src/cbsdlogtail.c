#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>

#define B_SIZE         4096

char           *offsetext = ".syncoff";
char           *offsetdir = "/tmp/";
char           *facil;
char           *myname;
char           *findex = NULL;

int 
usage(char *myname)
{
	printf("incremental tail for ascii files\n");
	printf("Usage: cbsd %s [-f facil -d offsetdir - %s default) asciifile\n", myname, offsetdir);
	exit(0);
}


long int 
getoffset(char *offsetfile)
{
	unsigned long long offset = 0;
	FILE           *fp, *fo;
	char		tmp       [B_SIZE + sizeof(long)];
	char		tmp2      [B_SIZE + sizeof(long)];
	int		i = 0,	n = 0, tmplen = 0;

	fp = fopen(offsetfile, "r");
	if (fp) {
		memset(tmp, 0, sizeof(tmp));
		fgets(tmp, B_SIZE + sizeof(long), fp);
		fclose(fp);
		tmplen = strlen(tmp);
		if (tmplen == 0)
			return 0;
		for (i = 0; i < tmplen; i++)
			if (tmp[i] == ':') {
				tmp[i] = ' ';
				n = i;
				break;
			}
		tmplen = 0;
		if (n > 0) {
			memset(tmp2, 0, sizeof(tmp2));
			sscanf(tmp, "%llu", &offset);
			strncpy(tmp2, tmp + n + 1, strlen(tmp) - n - 1);
	//+-1 =:	symbol
				fo = fopen(findex, "r");
			if (fo == NULL)
				return 0;
			fseek(fo, offset, SEEK_SET);
			memset(tmp, 0, sizeof(tmp));
			fgets(tmp, B_SIZE, fo);
			fclose(fo);
			if ((strlen(tmp2) != 0) && (!strcmp(tmp, tmp2))) {
				offset = offset + strlen(tmp2);
			} else
				offset = 0;
		}
	} else
		offset = 0;
	return offset;
}

int 
putoffset(char *offsetfile, char *str)
{
	FILE           *fp;
	fp = fopen(offsetfile, "w");
	if (fp) {
		fputs(str, fp);
		fclose(fp);
	}
	return 0;
}


char           *
show_myportion(long int offset)
{
	FILE           *fp;
	char		line      [B_SIZE];
	char           *lst;
	unsigned long long ipos;

	fp = fopen(findex, "r");
	if (fp == NULL) {
		printf("No such file\n");
		return NULL;
	}
	fseek(fp, offset, SEEK_SET);

	while (!feof(fp)) {
		if (fgets(line, sizeof(line), fp) != NULL)
			printf("%s", line);
	}

	ipos = ftell(fp) - strlen(line);
	fclose(fp);

	lst = malloc(sizeof(line) + sizeof(ipos) + 5);
	memset(lst, 0, strlen(lst));
	sprintf(lst, "%llu:%s", ipos, line);

	return lst;
}


int 
get_myportion()
{
	char           *offsetfile;
	int		i = 0;
	long int	ipos;
	char           *lst;
	offsetfile = malloc(sizeof(offsetdir) + sizeof(offsetext) + sizeof(facil));
	strcpy(offsetfile, offsetdir);
	strcat(offsetfile, facil);
	strcat(offsetfile, offsetext);
	i = getoffset(offsetfile);
	lst = show_myportion(i);
	if (lst)
		putoffset(offsetfile, lst);
	free(offsetfile);
	return 0;
}


int
main(int argc, char **argv)
{
	int		i = 0,	c = 0;

	myname = argv[0];
	if (!strcmp(argv[1], "--help"))
		usage(myname);
	findex = argv[argc - 1];
	facil = basename(findex);



	while (1) {
		c = getopt(argc, argv, "d:f:");
		/* Detect the end of the options. */
		if (c == -1)
			break;
		switch (c) {
		case 'd':
			offsetdir = optarg;
			/* if user didn't put trailing / do it for them */
			break;
		case 'f':
			facil = optarg;
			break;
		}
	}

	if (findex == myname)
		usage(myname);

	get_myportion();
	return 0;
}
