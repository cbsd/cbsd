// Message daemon (test)
// Part of CBSD Project

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <netinet/in.h>

#include <limits.h>

#include "nodemsgd.h"

#define MAXMSGLEN 1024
#define DEFPORT 8106
#define OK 0

void
acquire_daemonlock(int closeflag) {
    static int fd = -1;
    char buf[3*MAX_FNAME];
    char *ep;
    long otherpid;
    ssize_t num; 

    if (closeflag) {
	/* close stashed fd for child so we don't leak it. */
	    if (fd != -1) {
		close(fd);
		    fd = -1; 
	    }
	return;
    }

    if (fd == -1) {
	/* Initial mode is 0600 to prevent flock() race/DoS. */
	    if ((fd = open(pidfile, O_RDWR|O_CREAT, 0600)) == -1) {
		errmsg("can't open or create %s: %s",pidfile, strerror(errno));
		    exit(ERROR_EXIT);
		}

	if (flock(fd, LOCK_EX|LOCK_NB) < OK) {
	    int save_errno = errno;
	    bzero(buf, sizeof(buf));
	    if ((num = read(fd, buf, sizeof(buf) - 1)) > 0 &&
		(otherpid = strtol(buf, &ep, 10)) > 0 &&
		ep != buf && *ep == '\n' && otherpid != LONG_MAX) {
			errmsg("can't lock %s, otherpid may be %ld: %s",pidfile, otherpid, strerror(save_errno));
		    } else {
			errmsg("can't lock %s, otherpid unknown: %s",pidfile, strerror(save_errno));
		}
	    errmsg("can't lock %s, otherpid may be %ld: %s",pidfile, otherpid, strerror(save_errno));
	    exit(ERROR_EXIT);
	}
	(void) fchmod(fd, 0644);
	(void) fcntl(fd, F_SETFD, 1);
    }

	sprintf(buf, "%ld\n", (long)getpid());
	(void) lseek(fd, (off_t)0, SEEK_SET);
	num = write(fd, buf, strlen(buf));
	(void) ftruncate(fd, num);

	/* abandon fd even though the file is open. we need to keep
	* it open and locked, but we don't need the handles elsewhere.
	*/
}


int main(int argc, char**argv)
{
    int sockfd,n;
    struct sockaddr_in servaddr,cliaddr;
    socklen_t len;
    char mesg[MAXMSGLEN];
    char *node;
    char *tmp;
    char *msg;
    int i;
    FILE *fp;

    sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servaddr.sin_port=htons(DEFPORT);
    bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

    for (;;) {
	len = sizeof(cliaddr);
	n = recvfrom(sockfd,mesg,MAXMSGLEN,0,(struct sockaddr *)&cliaddr,&len);
	if (n<0) continue;
	mesg[n] = 0;
	tmp=strstr(mesg,":");

	if (tmp==NULL) {
	    printf("Unknown or invalid message struct\n");
	    sendto(sockfd,"1\r\n",1,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
	    continue;
        }

	i=tmp-mesg;
	node=malloc(i);
	msg=malloc(strlen(mesg)-i);
	memmove(node,mesg, i);
	memmove(msg,mesg+i+1,strlen(mesg)-i-1);
//	printf("Got msg from %s: %s\n",node,msg);
	fp=fopen("/tmp/yhm.log","a");
	fprintf(fp,"%s:%s\n",node,msg);
	fclose(fp);
	sendto(sockfd,"0\r\n",2,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
    }
}
