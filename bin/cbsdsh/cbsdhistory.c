// This is implemenation of build-in "cbsdhostory" command
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/file.h>
#include <errno.h>
#include <paths.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "main.h"

// max history in bytes before remove first line
// default - 300 Kbytes
#define MAX_HISTORY_SIZE 300000

int removeline(int fd, int line);

// return 0 if history size less than MAX_HISTORY_SIZE
// return 1 if history size greater than MAX_HISTORY_SIZE
int check_history_size(char *filename)
{
    struct stat st;

    if (stat(filename,&st)==0) {
	if (st.st_size<MAX_HISTORY_SIZE) return 0;
    }

    return 1;
}

// try to lock file, return 0 if success, return 1 if already locked
int setlock(int fd)
{
    struct flock lock, savelock;

    lock.l_type    = F_WRLCK;   /* Test for any lock on any part of file. */
    lock.l_start   = 0;
    lock.l_whence  = SEEK_SET;
    lock.l_len     = 0;

    savelock = lock;

    fcntl(fd, F_GETLK, &lock);  /* Overwrites lock structure with preventors. */

    if (lock.l_type == F_WRLCK)
    {
        return 1;
    }
    else if (lock.l_type == F_RDLCK)
    {
    //     printf("Process %d has a read lock already!\n", lock.l_pid);
        return 1;
    }
    else
        fcntl(fd, F_SETLK, &savelock);

    return 0;
}


int historycmd(int argc, char *argv[])
{
    return 0;

    int fd=0;
    int res=0;
    int i=0;
    char *line = NULL;
    FILE *fp;
//    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    if ( cbsd_enable_history == 0 ) return 0;

    // do not logging history command in history
    if (!strcmp(argv[1],"history")) return 0;
    if (!strcmp(argv[1],"sudo")) return 0;

    if ((fd = open(cbsd_history_file, O_RDWR | O_APPEND | O_CREAT,  (mode_t)0600 )) == -1) {
	return 0;
    }

    if (NULL == (fp = fdopen(fd, "a"))) {
	close(fd);
	return 1;
    }

    if (setlock(fd)!=0) return 1; //locked
    //rotate first line if necessary
    if (check_history_size(cbsd_history_file)!=0) removeline(fd,1);

    for (i = 1; i < argc; i++)
	res += strlen(argv[i])+1;
	res = res + 5; //+5: "cbsd " before and + "\n" at the end

    line=calloc(res,sizeof(char));

    strcpy(line,"cbsd ");

    for (i = 1; i < argc; i++) {
        strcat(line, argv[i]);
	strcat(line," " );
    }

    line[res-1] = '\n';
    fputs(line,fp);
    free(line);

    fflush(fp);
    fclose(fp);
    return 0;
}

//remove specified line number from file
int removeline(int fd, int line)
{
    if (fd < 0) {
	return EXIT_FAILURE;
    }

    struct stat stat_buf;

    if (fstat(fd, &stat_buf) < 0) {
	perror("fstat");
	return EXIT_FAILURE;
    }

    void *map = mmap(NULL, stat_buf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (map == MAP_FAILED) {
	perror("mmap");
	return EXIT_FAILURE;
    }

    char *linen = map;
    int i;

    for (i = 1; i < line; ++i) {
	linen = memchr(linen, '\n', stat_buf.st_size - (linen - (char *)map));
	if (!linen) {
	    fprintf(stderr, "Line %d is past EOF.\n", line);
	    return EXIT_FAILURE;
	}
	++linen;
    }

    char *linen1 = memchr(linen, '\n', stat_buf.st_size - (linen - (char *)map));

    if (linen1) {
	++linen1;
	memmove(linen, linen1, stat_buf.st_size - (linen1 - (char *)map));
	if (munmap(map, stat_buf.st_size) < 0) {
	    perror("munmap");
	    return EXIT_FAILURE;
	}

	if (ftruncate(fd, stat_buf.st_size - (linen1 - linen)) < 0) {
	    perror("ftruncate");
	    return EXIT_FAILURE;
	}
    } else {
	munmap(map, stat_buf.st_size);
	if (ftruncate(fd, linen - (char *)map) < 0) {
	    perror("ftruncate");
	    return EXIT_FAILURE;
	}
    }
    return EXIT_SUCCESS;
}
