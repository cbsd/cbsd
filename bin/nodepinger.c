// telnet /tmp/nodepinger.sock
// /usr/bin/lockf -s -t0 /tmp/176.9.147.18.lock /usr/bin/ssh -oBatchMode=yes -oStrictHostKeyChecking=no -oConnectTimeout=5 -q -oPort=22222 -i /usr/jails/.rssh/c1734206407e7e69d14092ebe796fd6d.id_rsa 176.9.147.18 iostat 1
//
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <paths.h>

#define SOCK_PATH	"/tmp/nodepinger.sock"
#define ERROR_LOG	"/var/log/nodepinger.log"
#define DEFAULT_PIDFILE_PATH	"/var/run/nodepinger.pid"

#define MAXNODELEN 1024

#define CREATE(result, type, number)  do {\
if (!((result) = (type *) calloc ((number), sizeof(type))))\
{ perror("malloc failure"); abort(); } } while(0)

#define REMOVE_FROM_LIST(item, head, next)      \
   if ((item) == (head))                \
      head = (item)->next;              \
   else {                               \
      temp = head;                      \
      while (temp && (temp->next != (item))) \
         temp = temp->next;             \
      if (temp)                         \
         temp->next = (item)->next;     \
   }                                    \


int debug=0;
int base_del(char *);

// size of records
int recsize=0;
int online=0;

// timeout for kqueue in sec. Default = 20 sec
static int interval=5;

//node - ip or fqdn
struct nodedata {
    char node[MAXNODELEN];
//    pid_t pid;
    struct nodedata *next;
};

struct nodedata *nodelist = NULL;
int add_base_data(char *);

typedef struct in_addr in_addr;
typedef struct sockaddr_in sockaddr_in;
typedef struct servent servent;

typedef void (action) (register struct kevent const *const kep);

/* Event Control Block (ecb) */
typedef struct {
    action	*do_read;
    action	*do_write;
    char	*buf;
    unsigned	bufsiz;
} ecb;

static char const *pname;
static struct kevent *ke_vec = NULL;
static unsigned ke_vec_alloc = 0;
static unsigned ke_vec_used = 0;
static char const protoname[] = "tcp";
static char const servname[] = "echo";

int make_node_list(char *, int);
int nodeping();
int updatenodecenter(char *,int);

void handler(int signo)
{
    pid_t pid;
    pid=wait(NULL);
    signal(SIGCHLD,handler);
    printf("Pid %d exit\n",pid);
}

int base_add(char *node)
{
    struct nodedata *newnode;
    int i,records=0;
    struct timeval now_time;
    int current_time;
    int ende=0;

    for (newnode = nodelist; newnode; newnode = newnode -> next)
	{
	if (!strcmp(newnode->node,node)) {
	    base_del(node);
	    return 0; //already exist
	}
	else 
	    records++;
    }

    CREATE(newnode, struct nodedata, 1);
    memset((char *) newnode, 0, sizeof(struct nodedata));
    strcpy(newnode->node,node);
    newnode->next= nodelist;
    nodelist = newnode;
    recsize=recsize+strlen(node)+1;
    return 0;
}

int base_del(char *node)
{
    struct nodedata *newnode;
    struct nodedata *temp;

    int i,records=0;

    for (newnode = nodelist; newnode; newnode = newnode -> next)
	{
	    if (!strcmp(newnode->node,node))  {
		REMOVE_FROM_LIST(newnode,nodelist,next);
		recsize=recsize-strlen(node)-1;
		return 0;
	    }
	}
    return 0;
}

static void vlog (char const *const fmt, va_list ap)
{
    vfprintf (stderr, fmt, ap);
    fputc ('\n', stderr);
}

static void fatal (char const *const fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    fprintf (stderr, "%s: ", pname);
    vlog (fmt, ap);
    va_end (ap);
    exit(1);
}

int debugmsg(int level,const char *format, ...)
{
    va_list arg;
    int done;
    FILE *fp;

        if(debug<level) return 0;

    fp=fopen(ERROR_LOG,"a");
        va_start (arg, format);
        done = vfprintf (fp, format, arg);
        va_end (arg);
    fclose(fp);
    return 0;
}

static void usage (void)
{
    fatal ("Usage `%s [-p port]'", pname);
}

static void *xmalloc (register unsigned long const size)
{
    register void *const result = malloc (size);

    if (!result)
	fatal ("Memory exhausted");
    return result;
}

static void *xrealloc (register void *const ptr, register unsigned long const size)
{
    register void *const result = realloc (ptr, size);

    if (!result)
	fatal ("Memory exhausted");
    return result;
}

static void ke_change (register int const ident,
	   register int const filter,
	   register int const flags,
	   register void *const udata)
{
    enum { initial_alloc = 64 };
    register struct kevent *kep;

    if (!ke_vec_alloc)
    {
	ke_vec_alloc = initial_alloc;
	ke_vec = (struct kevent *) xmalloc(ke_vec_alloc * sizeof (struct kevent));
    }
    else 
	if (ke_vec_used == ke_vec_alloc)
	{
	    ke_vec_alloc <<= 1;
	    ke_vec =
	    (struct kevent *) xrealloc (ke_vec,ke_vec_alloc * sizeof (struct kevent));
    }

    kep = &ke_vec[ke_vec_used++];

    kep->ident = ident;
    kep->filter = filter;
    kep->flags = flags;
    kep->fflags = 0;
    kep->data = 0;
    kep->udata = udata;
}

static void do_write (register struct kevent const *const kep)
{
    register int n;
    register ecb *const ecbp = (ecb *) kep->udata;

    n = write (kep->ident, ecbp->buf, ecbp->bufsiz);
    free (ecbp->buf);  /* Free this buffer, no matter what.  */

    if (n == -1)
    {
	debugmsg(0,"Error writing socket: %s", strerror (errno));
	close (kep->ident);
	free(kep->udata);
    }

    close(kep->ident);
    free(kep->udata);
}

static void do_read (register struct kevent const *const kep)
{
  enum { bufsize = MAXNODELEN+5 };
  auto char buf[bufsize];
  register int n;
  register ecb *const ecbp = (ecb *) kep->udata;
  char node[MAXNODELEN+5];
  int len;
  char *ndlist;
  int i;
  char tmp[1024];
  int space=0;

    if ((n = read (kep->ident, buf, bufsize)) == -1)
    {
	debugmsg(0,"Error reading socket: %s", strerror (errno));
	close(kep->ident);
	free(kep->udata);
	goto getout;
    }
    else if (n == 0)
    {
	debugmsg(0,"EOF reading socket");
	close(kep->ident);
	free(kep->udata);
      goto getout;
    }

    buf[strlen(buf)-2]='\0';

    if (buf[0]=='l') {
	memset(buf,0,sizeof(buf));
	CREATE(ndlist,char,recsize);
//	if( !make_node_list(ndlist, recsize))
//	    close(kep->ident); free(kep->udata); goto getout;

	make_node_list(ndlist,recsize);
	ecbp->buf = (char *) xmalloc(recsize);
	ecbp->bufsiz=recsize;
	memcpy(ecbp->buf,ndlist,recsize);
	free(ndlist);
//	close(kep->ident);
//	free(kep->udata);
//	goto getout;
    }
    if (buf[0]=='q') { close(kep->ident); free(kep->udata); online=1; goto getout; }
    if (strlen(buf)>5) base_add(buf);

    ke_change (kep->ident, EVFILT_READ, EV_DISABLE, kep->udata);
    ke_change (kep->ident, EVFILT_WRITE, EV_ENABLE, kep->udata);

getout:
    len=0;
}


static void do_accept (register struct kevent const *const kep)
{
    auto sockaddr_in sin;
    auto socklen_t sinsiz;
    register int s;
    register ecb *ecbp;

    if ((s = accept (kep->ident, (struct sockaddr *)&sin, &sinsiz)) == -1)
	fatal ("Error in accept(): %s", strerror (errno));

    ecbp = (ecb *) xmalloc (sizeof (ecb));
    ecbp->do_read = do_read;
    ecbp->do_write = do_write;
    ecbp->buf = NULL;
    ecbp->bufsiz = 0;

    ke_change (s, EVFILT_READ, EV_ADD | EV_ENABLE, ecbp);
    ke_change (s, EVFILT_WRITE, EV_ADD | EV_DISABLE, ecbp);
}

static void event_loop (register int const kq)
{
    struct timespec tv;
    char tmp[1024];

    for (;;)
    {
	register int n;
	register struct kevent const *kep;

	tv.tv_sec = interval;
	tv.tv_nsec = 0;
	//tv.tv_usec = 0;

	//      n = kevent (kq, ke_vec, ke_vec_used, ke_vec, ke_vec_alloc, NULL);
	 n = kevent (kq, ke_vec, ke_vec_used, ke_vec, ke_vec_alloc, &tv);

	//memset(tmp,0,sizeof(tmp));
	//sprintf(tmp,"alive, interval = %d, tv.tv_sec=%d\n",interval,tv.tv_sec);
	//delete tv;

	//struct timespec tv;
	tv.tv_sec = 5;
	tv.tv_nsec=0;

	ke_vec_used = 0;  /* Already processed all changes.  */

	if (n == -1)
	    fatal ("Error in kevent(): %s", strerror (errno));
	if (n == 0) nodeping();

	for (kep = ke_vec; kep < &ke_vec[n]; kep++)
	{
	    register ecb const *const ecbp = (ecb *) kep->udata;
	    if (kep->filter == EVFILT_READ)
		(*ecbp->do_read) (kep);
	    else
		(*ecbp->do_write) (kep);
	}
	if (online!=0) break;
    }
}

int
main (register int const argc, register char *const argv[])
{
    auto in_addr listen_addr;
    register int optch;
    auto int one = 1;
    register int portno = 0;
    register int option_errors = 0;
    register int server_sock;
    struct sockaddr_un sin;
    register servent *servp;
    auto ecb listen_ecb;
    register int kq;
    int len;
    int fd;
    FILE *fp;
    struct sigaction osa, sa;
    pid_t newgrp;
    int oerrno;
    int osa_ok;
    char *cp;
    char *mpath;

    (void)chdir("/");
    fd = open(_PATH_DEVNULL, O_RDWR, 0);
    //(void)_dup2(fd, STDIN_FILENO);
    //(void)_dup2(fd, STDOUT_FILENO);
    //(void)_dup2(fd, STDERR_FILENO);
    //(void)_close(fd);

    (void)dup2(fd, STDIN_FILENO);
    (void)dup2(fd, STDOUT_FILENO);
    (void)dup2(fd, STDERR_FILENO);
    (void)close(fd);

    /* A SIGHUP may be thrown when the parent exits below. */
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    //osa_ok = _sigaction(SIGHUP, &sa, &osa);
    osa_ok = sigaction(SIGHUP, &sa, &osa);

    signal(SIGCHLD,SIG_IGN);

    pid_t p2=fork();
    if (p2!=0) _exit(0);
    pid_t p3=getpid();
    newgrp = setsid();

    fp=fopen(DEFAULT_PIDFILE_PATH,"w");
    if (!fp) 
    {
	printf("Cant open pid file"); _exit(1); 
    }

    fprintf(fp,"%d\n",p3);
    fclose(fp);

    pname = strrchr (argv[0], '/');
    pname = pname ? pname+1 : argv[0];

    listen_addr.s_addr = htonl (INADDR_ANY);  /* Default.  */

    if ((server_sock = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
	fatal ("Error creating socket: %s", strerror (errno));

    memset(&sin,0,sizeof sin);
    sin.sun_family = AF_UNIX;

    if ((cp = getenv("workdir")) == NULL) strcpy(sin.sun_path,SOCK_PATH);
    else
    {
	mpath=malloc(strlen(cp)+strlen(SOCK_PATH)+2); // +2 = slash
	sprintf(mpath, "%s/%s", cp,SOCK_PATH);
	strcpy(sin.sun_path,mpath);
	free(mpath);
    }

    unlink(sin.sun_path);

    len = strlen(sin.sun_path) + sizeof(sin.sun_family)+1;

    if (bind (server_sock, (const struct sockaddr *)&sin, len) == -1)
	fatal ("Error binding socket: %s", strerror (errno));

    if (listen (server_sock, 20) == -1)
	fatal ("Error listening to socket: %s", strerror (errno));

    if ((kq = kqueue ()) == -1)
	fatal ("Error creating kqueue: %s", strerror (errno));

    listen_ecb.do_read = do_accept;
    listen_ecb.do_write = NULL;
    listen_ecb.buf = NULL;
    listen_ecb.buf = 0;

    ke_change (server_sock, EVFILT_READ, EV_ADD | EV_ENABLE, &listen_ecb);

    event_loop (kq);
    unlink(sin.sun_path);
}

int make_node_list(char* ndlist, int size)
{
    int i,n=0;
    struct nodedata *hs;
    char tmp[MAXNODELEN];

    memset(ndlist,0,strlen(ndlist));

    for (hs = nodelist; hs; hs = hs->next)
    {
	memset(tmp,0,sizeof(tmp));
	sprintf(tmp,"%s\n",hs->node);
	strcat(ndlist,tmp);
    }
return 0;
}

int nodeping()
{
    struct nodedata *node;
    struct sigaction osa, sa;
    int osa_ok;
    pid_t newgrp;

    /* A SIGHUP may be thrown when the parent exits below. */
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    //osa_ok = _sigaction(SIGHUP, &sa, &osa);
    osa_ok = sigaction(SIGHUP, &sa, &osa);

    signal(SIGCHLD,SIG_IGN);

    pid_t p2=fork();
    if (p2!=0) return 0;

    newgrp = setsid();

    for (node = nodelist; node; node = node -> next)
    {
	debugmsg(0,"Exec: [%s]\n",node->node);
	system(node->node);
    }
    _exit(0);
}
