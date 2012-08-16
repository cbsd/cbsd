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

#define SOCK_PATH "/tmp/nodepinger.sock"
#define RSSH_DIR "/.rssh"
#define KEY_EXT ".id_rsa"
#define SSH_CMD "/usr/bin/ssh -oPort=22222 -oConnectTimeout=5 -oBatchMode=yes -oStrictHostKeyChecking=no"
#define SSH_USER "cbsd"
#define ERROR_LOG "/var/log/nodepinger.log"
#define SIGNATURE "ok"
#define NCUPDATESCRIPT "/nodectl/nodestatus"
#define LOCK_DIR "/tmp"
#define DEFAULT_PIDFILE_PATH	"/var/run/nodepinger.pid"

// max lifetime of any lock files
#define MAX_DEADLOCK 300
#define MAXCMDLEN 5
#define MAXNODELEN 50
#define MAXVERSIONLEN 5
#define MAXSTATUSLEN 10

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

// size of records
int recsize=0;

// timeout for kqueue in sec. Default = 20 sec
static int interval=30;

// interval for offline host. Default 120 sec
int offlineinterval=120;

//node - ip or fqdn
//mtime - last modify
struct nodedata {
char node[MAXNODELEN];
int prevstatus;
int status;
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


void handler(int signo)
{
pid_t pid;
pid=wait(NULL);
signal(SIGCHLD,handler);
printf("Pid %d exit\n",pid);
return;
}



int base_add(char *node)
{
struct nodedata *newnode;
int i,records=0;
struct timeval now_time;
int current_time;
int ende=0;

gettimeofday( &now_time, NULL );
current_time        = (time_t) now_time.tv_sec;

for (newnode = nodelist; newnode; newnode = newnode -> next)
{
    if (!strcmp(newnode->node,node)) return 0; //already exist
    else records++;
}

CREATE(newnode, struct nodedata, 1);
memset((char *) newnode, 0, sizeof(struct nodedata));
strcpy(newnode->node,node);
newnode->prevstatus=100; //magic num. not 0 or 1
newnode->status=101; //must be diffrent with  prevstatus at first time
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
}

static void
vlog (char const *const fmt, va_list ap)
{
  vfprintf (stderr, fmt, ap);
  fputc ('\n', stderr);
}

static void fatal (char const *const fmt, ...)
    __attribute__ ((__noreturn__));

static void
fatal (char const *const fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  fprintf (stderr, "%s: ", pname);
  vlog (fmt, ap);
  va_end (ap);
  exit (1);
}

static void
error (char const *const fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  fprintf (stderr, "%s: ", pname);
  vlog (fmt, ap);
  va_end (ap);
}

static void
usage (void)
{
  fatal ("Usage `%s [-p port]'", pname);
}

static int
all_digits (register char const *const s)
{
  register char const *r;

  for (r = s; *r; r++)
    if (!isdigit (*r))
      return 0;
  return 1;
}

static void *
xmalloc (register unsigned long const size)
{
  register void *const result = malloc (size);

  if (!result)
    fatal ("Memory exhausted");
  return result;
}

static void *
xrealloc (register void *const ptr, register unsigned long const size)
{
  register void *const result = realloc (ptr, size);

  if (!result)
    fatal ("Memory exhausted");
  return result;
}

static void
ke_change (register int const ident,
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
  else if (ke_vec_used == ke_vec_alloc)
    {
      ke_vec_alloc <<= 1;
      ke_vec =
	(struct kevent *) xrealloc (ke_vec,
				    ke_vec_alloc * sizeof (struct kevent));
    }

  kep = &ke_vec[ke_vec_used++];

  kep->ident = ident;
  kep->filter = filter;
  kep->flags = flags;
  kep->fflags = 0;
  kep->data = 0;
  kep->udata = udata;
}

static void
do_write (register struct kevent const *const kep)
{
  register int n;
  register ecb *const ecbp = (ecb *) kep->udata;

  n = write (kep->ident, ecbp->buf, ecbp->bufsiz);
  free (ecbp->buf);  /* Free this buffer, no matter what.  */
  if (n == -1)
    {
      error ("Error writing socket: %s", strerror (errno));
      close (kep->ident);
      free (kep->udata);
    }

close(kep->ident);
free(kep->udata);

}

static void
do_read (register struct kevent const *const kep)
{
  enum { bufsize = MAXCMDLEN+MAXNODELEN+5 };
  auto char buf[bufsize];
  register int n;
  register ecb *const ecbp = (ecb *) kep->udata;
  char cmd[MAXCMDLEN];
  char node[MAXNODELEN+5];
  int len;
  char *ndlist;
  int i;
  char tmp[1024];
  int space=0;

  if ((n = read (kep->ident, buf, bufsize)) == -1)
    {
      error ("Error reading socket: %s", strerror (errno));
      close (kep->ident);
      free (kep->udata);
      goto getout;
    }
  else if (n == 0)
    {
      error ("EOF reading socket");
      close (kep->ident);
      free (kep->udata);
      goto getout;
    }

memset(cmd,0,sizeof(cmd));
memset(node,0,sizeof(node));
for (i=0;i<strlen(buf);i++)
if (buf[i]==' ') space=1;


if ((strlen(buf)>3)&&(space==1)) {
sscanf(buf,"%s %s",cmd,node);
} else if (strlen(buf)>3) sscanf(buf,"%s",cmd);


if (!strcmp(cmd,"quit")) { close(kep->ident); free(kep->udata); goto getout; }
if (!strcmp(cmd,"add")&&(strlen(node)>2)) base_add(node);
if (!strcmp(cmd,"del")&&(strlen(node)>2)) base_del(node);
if (!strcmp(cmd,"int")&&(strlen(node)!=0)) {
i=atoi(node);
// protect. must be >= of ssh timeout
if ((i>5)&&(i<3600)) interval=i;
memset(tmp,0,sizeof(tmp));
sprintf(tmp,"Change interval to %d\n",i);
tolog(tmp);
close(kep->ident); free(kep->udata); goto getout;
}

if (!strcmp(cmd,"list")) {
CREATE(ndlist,char,recsize);
if( !make_node_list(ndlist, recsize))
    {
    close(kep->ident); free(kep->udata); goto getout;
    }
ecbp->buf = (char *) xmalloc(recsize);
ecbp->bufsiz=recsize;
memcpy(ecbp->buf,ndlist,recsize);
free(ndlist);
}
  ke_change (kep->ident, EVFILT_READ, EV_DISABLE, kep->udata);
  ke_change (kep->ident, EVFILT_WRITE, EV_ENABLE, kep->udata);
getout:
//label at end of compound statement
len=0;
}


static void
do_accept (register struct kevent const *const kep)
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
    __attribute__ ((__noreturn__));

static void
event_loop (register int const kq)
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
//tolog(tmp);
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
fd = _open(_PATH_DEVNULL, O_RDWR, 0);
(void)_dup2(fd, STDIN_FILENO);
(void)_dup2(fd, STDOUT_FILENO);
(void)_dup2(fd, STDERR_FILENO);
//if (fd > 2) 
(void)_close(fd);

/* A SIGHUP may be thrown when the parent exits below. */
sigemptyset(&sa.sa_mask);
sa.sa_handler = SIG_IGN;
sa.sa_flags = 0;
osa_ok = _sigaction(SIGHUP, &sa, &osa);

signal(SIGCHLD,SIG_IGN);

pid_t p2=fork();
if (p2!=0) {
_exit(0);
}
pid_t p3=getpid();
newgrp = setsid();

fp=fopen(DEFAULT_PIDFILE_PATH,"w");
if (!fp) { tolog("Cant open pid file"); _exit(1); }
fprintf(fp,"%d\n",p3);
fclose(fp);

  pname = strrchr (argv[0], '/');
  pname = pname ? pname+1 : argv[0];

  listen_addr.s_addr = htonl (INADDR_ANY);  /* Default.  */

  while ((optch = getopt (argc, argv, "p:")) != EOF)
    {
      switch (optch)
        {
        case 'p':
          if (strlen (optarg) == 0 || !all_digits (optarg))
            {
              error ("Invalid argument for -p option: %s", optarg);
              option_errors++;
            }
          portno = atoi (optarg);
          if (portno == 0 || portno >= (1u << 16))
            {
              error ("Invalid argument for -p option: %s", optarg);
              option_errors++;
            }
          break;
	default:
          error ("Invalid option: -%c", optch);
          option_errors++;
        }
    }

  if (option_errors || optind != argc)
    usage ();

  if (portno == 0)
    {
      if ((servp = getservbyname (servname, protoname)) == NULL)
        fatal ("Error getting port number for service `%s': %s",
               servname, strerror (errno));
      portno = ntohs (servp->s_port);
    }

  if ((server_sock = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
    fatal ("Error creating socket: %s", strerror (errno));

memset(&sin,0,sizeof sin);
sin.sun_family = AF_UNIX;

if ((cp = getenv("workdir")) == NULL) strcpy(sin.sun_path,SOCK_PATH);
else {
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
}

int make_node_list(char* ndlist, int size)
{
int i,n=0;
struct nodedata *hs;
char tmp[MAXNODELEN];

memset(ndlist,0,sizeof(ndlist));

for (hs = nodelist; hs; hs = hs->next)
{
memset(tmp,0,sizeof(tmp));
sprintf(tmp,"%s\n",hs->node);
strcat(ndlist,tmp);
}
return 1;
}

int nodeping()
{
struct nodedata *node;
int i;
char *keypath;
char *lock_path;
char *lockpathstat;
struct stat st;
char buff[4096];
char res[strlen(SIGNATURE)+1];
FILE *fp;
int fd;
struct sigaction osa, sa;
pid_t newgrp;
int oerrno;
int osa_ok;
struct timeval now_time;
int current_time;
int offtime;
char *sock_path;
char *key_ext;
char *lock_dir;
char *lock_file;
char *cp;


/* A SIGHUP may be thrown when the parent exits below. */
sigemptyset(&sa.sa_mask);
sa.sa_handler = SIG_IGN;
sa.sa_flags = 0;
osa_ok = _sigaction(SIGHUP, &sa, &osa);

signal(SIGCHLD,SIG_IGN);
 
pid_t p2=fork();
if (p2!=0) return 0;

newgrp = setsid();

newgrp = setsid();
oerrno = errno;

if ((cp = getenv("workdir")) == NULL) lock_path=LOCK_DIR;
else {
    lock_path=malloc(strlen(cp)+strlen(LOCK_DIR)+2); // +2 - splash
    sprintf(lock_path, "%s/%s", cp,LOCK_DIR);
}

for (node = nodelist; node; node = node -> next)
{
//for of all - check for near offline status of machine
gettimeofday( &now_time, NULL );
current_time        = (time_t) now_time.tv_sec;

if (cp!=NULL) {
     lockpathstat=malloc(MAXNODELEN+strlen(lock_path)+strlen(KEY_EXT)+6); // +6 = slash + status .o{n,off}
} 

memset(lockpathstat,0,sizeof(lockpathstat));
sprintf(lockpathstat,"%s/%s.off",lock_path,node->node);
	if (stat(lockpathstat,&st)==0) 
		{  //lock exist.
			    //check for dead lock
			    fp=fopen(lockpathstat,"r");
			    offtime=0;
			    fscanf(fp,"%d",&offtime);
			    fclose(fp);
				if ((current_time-offtime)>MAX_DEADLOCK) { //old lock
				memset(buff,0,sizeof(buff));
				sprintf(buff,"Deadlock for %s.off detected. Removed\n",node->node);
				tolog(buff);
				unlink(lockpathstat);
				} else 
				if ((current_time-offtime)>offlineinterval) unlink(lockpathstat);
		free(lockpathstat);
		continue;
		} 

//second of all - lock
lock_file=malloc(strlen(lock_path)+strlen(node->node)+6); // +6 = slash + .lck
memset(lock_file,0,sizeof(lock_file));
sprintf(lock_file,"%s/%s.lck",lock_path,node->node);
	if (stat(lock_file,&st)!=0) { //makelock
		fp=fopen(lock_file,"w");
		fprintf(fp,"%d",current_time);
		fclose(fp);
	}
    else { //lock exist
    //check for dead lock
    fp=fopen(lock_file,"r");
    offtime=0;
    fscanf(fp,"%d",&offtime);
	if ((current_time-offtime)>MAX_DEADLOCK) {
            memset(buff,0,sizeof(buff));
            sprintf(buff,"Dead lock for %s.lck detected. Removed\n",node->node);
            tolog(buff);
            unlink(lock_file);
	    free(lock_file);
            continue; // lock in progress
            }
	} //end of else lock exist

if (cp) {
keypath=malloc(strlen(cp)+strlen(RSSH_DIR)+strlen(node->node)+strlen(KEY_EXT)+2); // +2 = slash
memset(keypath,0,sizeof(keypath));
sprintf(keypath,"%s%s/%s%s",cp,RSSH_DIR,node->node,KEY_EXT);
}
else {
keypath=malloc(strlen(RSSH_DIR)+strlen(node->node)+strlen(KEY_EXT)+2); // +2 = slash
memset(keypath,0,sizeof(keypath));
sprintf(keypath,"%s/%s%s",RSSH_DIR,node->node,KEY_EXT);
}

	if (stat(keypath,&st)!=0) {
		memset(buff,0,sizeof(buff));
		sprintf(buff,"Cant access to %s\n",keypath);
        	tolog(buff);
		unlink(lock_file);
		free(lock_file);
		fp=fopen(lockpathstat,"w");
		fprintf(fp,"%d",current_time);
		fclose(fp);
		free(lockpathstat);
		free(keypath);
		continue;
	}

	memset(buff,0,sizeof(buff));
	sprintf(buff,"%s -i %s %s@%s echo %s",SSH_CMD,keypath,SSH_USER,node->node,SIGNATURE);
	fp=popen(buff,"r");
	if (!fp) {tolog("cant execute popen");break;}
	memset(res,0,sizeof(res));
	fgets(res,strlen(SIGNATURE)+1,fp);
	pclose(fp);
	if (!strcmp(res,SIGNATURE)) node->status=1;
	else node->status=0;
//this change not for fork with isolate own copy of memory lol ;)
//	if (node->prevstatus!=node->status) { // change in state
//	    node->prevstatus=node->status;
	unlink(lock_file);
	free(lock_file);
	if (node->status==0) {
	fp=fopen(lockpathstat,"w");
	fprintf(fp,"%d",current_time);
	fclose(fp);
	free(lockpathstat);
	}
	    updatenodecenter(node->node,node->status);
} //for

_exit(0);
}



int tolog(char *buff)
{
FILE *fp;
char *error_log;
char *cp;

if ((cp = getenv("workdir")) == NULL) error_log=ERROR_LOG;
else {
    error_log=malloc(strlen(cp)+strlen(ERROR_LOG)+2); // +2 - slash
    sprintf(error_log, "%s/%s", cp,ERROR_LOG);
}

fp=fopen(error_log,"a");

if (!fp) {
if (cp) { free(error_log);
return 1;
}
}

fputs(buff,fp);
fclose(fp);

if (cp) free(error_log);
}


updatenodecenter(char *node,int result)
{
char *cp;
char *buff;


if ((cp = getenv("workdir")) == NULL) {
    buff=malloc(strlen(NCUPDATESCRIPT)+strlen(node)+3); // +3 - result
    sprintf(buff, "%s %s %d", NCUPDATESCRIPT,node,result);
} else {
   buff=malloc(strlen(cp)+strlen(NCUPDATESCRIPT)+strlen(node)+4); // +4 = slash + result 
   sprintf(buff, "%s/%s %s %d", cp,NCUPDATESCRIPT,node,result);
}

system(buff);
free(buff);
}


