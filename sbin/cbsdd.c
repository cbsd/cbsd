// this dump empty daemon that will soon provide some services
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

#define SOCK_PATH "/tmp/cbsd.sock"
#define ERROR_LOG "/var/log/cbsd.log"
#define PIDFILE "/var/run/cbsdd.pid"

#define DBFILE "/var/db/cbsd.db"

// max lifetime of any lock files
#define MAX_DEADLOCK 300

#define MAXCMDLEN 7
#define MAXNODELEN 50
#define MAXVERSIONLEN 5
#define MAXSTATUSLEN 10
#define MAXVALUELEN 1024

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
int		recsize = 0;

//timeout for kqueue
	in sec.Default = 20 sec
		static int	interval = 30;

//interval for offline
	host.Default 120 sec
		int		offlineinterval = 120;

//node - ip or fqdn
// mtime - last modify
struct nodedata {
	char		node      [MAXNODELEN];
	int		prevstatus;
	int		status;
	char		value     [MAXVALUELEN];
	struct nodedata *next;
};

struct nodedata *nodelist = NULL;
int		add_base_data(char *);

typedef struct in_addr in_addr;
typedef struct sockaddr_in sockaddr_in;
typedef struct servent servent;

typedef void    (action) (register struct kevent const *const kep);

/* Event Control Block (ecb) */
typedef struct {
	action         *do_read;
	action         *do_write;
	char           *buf;
	unsigned	bufsiz;
}		ecb;

static char const *pname;
static struct kevent *ke_vec = NULL;
static unsigned	ke_vec_alloc = 0;
static unsigned	ke_vec_used = 0;
static char const protoname[] = "tcp";
static char const servname[] = "echo";

int		save_basedb();
int		tolog      (char *);
int		base_get   (char *, char *, int);
int		make_node_list(char *, int);
int		load_basedb();


void 
handler(int signo)
{
	pid_t		pid;
	pid = wait(NULL);
	signal(SIGCHLD, handler);
	printf("Pid %d exit\n", pid);
	return;
}



int 
base_add(char *node, char *value, int update)
{
	struct nodedata *newnode;
	int		i         , records = 0;
	struct timeval	now_time;
	int		current_time;
	int		ende = 0;
	char		tmp       [MAXVALUELEN + MAXNODELEN + 5];
	int		lsize = 0;

	gettimeofday(&now_time, NULL);
	current_time = (time_t) now_time.tv_sec;

	for (newnode = nodelist; newnode; newnode = newnode->next) {
		if (!strcmp(newnode->node, node)) {
			if (update == 0)
				return 0;
			//already exist
				else {
				lsize = strlen(newnode->value);
				memset(newnode->value, 0, sizeof(newnode->value));
				strcpy(newnode->value, value);
				recsize = recsize - lsize;
				recsize = recsize + strlen(value);
				save_basedb();
				return 0;
			}
			records++;
		}
	}

	CREATE(newnode, struct nodedata, 1);
	memset((char *)newnode, 0, sizeof(struct nodedata));
	strcpy(newnode->node, node);
	strcpy(newnode->value, value);
	newnode->prevstatus = 100;
	//magic num.not 0 or 1
		newnode->status = 101;
	//must be diffrent with prevstatus at first time
		newnode->next = nodelist;
	nodelist = newnode;

	memset(tmp, 0, sizeof(tmp));
	sprintf(tmp, "Added %s %s\n", node, value);
	tolog(tmp);

	recsize = recsize + strlen(node) + strlen(value) + 1;
	save_basedb();
	return 0;
}

int 
base_del(char *node)
{
	struct nodedata *newnode;
	struct nodedata *temp;

	int		i         , records = 0;

	for (newnode = nodelist; newnode; newnode = newnode->next) {
		if (!strcmp(newnode->node, node)) {
			//printf("%s removed\n", node);
			recsize = recsize - strlen(newnode->node) - strlen(newnode->value) - 1;
			REMOVE_FROM_LIST(newnode, nodelist, next);
			return 0;
		}
	}
	return 0;
}

static void
vlog(char const *const fmt, va_list ap)
{
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
}

static void 
fatal(char const *const fmt,...)
__attribute__((__noreturn__));

	static void
			fatal         (char const *const fmt,...)
{
	va_list		ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s: ", pname);
	vlog(fmt, ap);
	va_end(ap);
	exit(1);
}

static void
error(char const *const fmt,...)
{
	va_list		ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s: ", pname);
	vlog(fmt, ap);
	va_end(ap);
}

static void 
usage(void)
{
	fatal("Usage `%s [-p port]'", pname);
}

static int 
all_digits(register char const *const s)
{
	register char const *r;

	for (r = s; *r; r++)
		if (!isdigit(*r))
			return 0;
	return 1;
}

static void    *
xmalloc(register unsigned long const size)
{
	register void  *const result = malloc(size);

	if (!result)
		fatal("Memory exhausted");
	return result;
}

static void    *
xrealloc(register void *const ptr, register unsigned long const size)
{
	register void  *const result = realloc(ptr, size);

	if (!result)
		fatal("Memory exhausted");
	return result;
}

static void 
ke_change(register int const ident,
	  register int const filter,
	  register int const flags,
	  register void *const udata)
{
	enum {
	initial_alloc = 64};
	register struct kevent *kep;

	if (!ke_vec_alloc) {
		ke_vec_alloc = initial_alloc;
		ke_vec = (struct kevent *)xmalloc(ke_vec_alloc * sizeof(struct kevent));
	} else if (ke_vec_used == ke_vec_alloc) {
		ke_vec_alloc <<= 1;
		ke_vec = (struct kevent *)xrealloc(ke_vec, ke_vec_alloc * sizeof(struct kevent));
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
do_write(register struct kevent const *const kep)
{
	register int	n;
	register ecb   *const ecbp = (ecb *) kep->udata;

	n = write(kep->ident, ecbp->buf, ecbp->bufsiz);
	free(ecbp->buf);	/* Free this buffer, no matter what.  */
	if (n == -1) {
		error("Error writing socket: %s", strerror(errno));
		close(kep->ident);
		free(kep->udata);
	}
	close(kep->ident);
	free(kep->udata);

}

static void 
do_read(register struct kevent const *const kep)
{
	enum {
	bufsize = MAXCMDLEN + MAXNODELEN + 5};
	auto char	buf   [bufsize];
	register int	n;
	register ecb   *const ecbp = (ecb *) kep->udata;
	char		cmd       [MAXCMDLEN];
	char		node      [MAXNODELEN + 5];
	char		value     [MAXVALUELEN + 5];
	int		len;
	char           *ndlist;
	int		i;
	char		tmp       [1024];
	int		space = 0;

	if ((n = read(kep->ident, buf, bufsize)) == -1) {
		error("Error reading socket: %s", strerror(errno));
		close(kep->ident);
		free(kep->udata);
		goto getout;
	} else if (n == 0) {
		error("EOF reading socket");
		close(kep->ident);
		free(kep->udata);
		goto getout;
	}
	memset(cmd, 0, sizeof(cmd));
	memset(node, 0, sizeof(node));
	memset(value, 0, sizeof(value));
	for (i = 0; i < strlen(buf); i++)
		if (buf[i] == ' ')
			space++;

	if (strlen(buf) < 3)
		goto getout;
	switch (space) {
	case 0:
		sscanf(buf, "%s", cmd);
		break;
	case 1:
		sscanf(buf, "%s %s", cmd, node);
		break;
	case 2:
		sscanf(buf, "%s %s %s", cmd, node, value);
		break;
	}

	if (!strcmp(cmd, "quit")) {
		close(kep->ident);
		free(kep->udata);
		goto getout;
	}
	if (!strcmp(cmd, "add") && (strlen(node) > 2))
		base_add(node, value, 0);
	if (!strcmp(cmd, "update") && (strlen(node) > 2))
		base_add(node, value, 1);

	if (!strcmp(cmd, "get") && (strlen(node) > 2)) {
		CREATE(ndlist, char, MAXVALUELEN + 5);

		if (!base_get(node, ndlist, MAXVALUELEN + 5)) {
			close(kep->ident);
			free(kep->udata);
			goto getout;
		}
		ecbp->buf = (char *)xmalloc(recsize);
		ecbp->bufsiz = recsize;
		memcpy(ecbp->buf, ndlist, recsize);
		free(ndlist);
	}
	if (!strcmp(cmd, "del") && (strlen(node) > 2))
		base_del(node);

	if (!strcmp(cmd, "list")) {
		CREATE(ndlist, char, recsize);
		if (!make_node_list(ndlist, recsize)) {
			//printf("Something wrong with baselist");
			close(kep->ident);
			free(kep->udata);
			goto getout;
		}
		ecbp->buf = (char *)xmalloc(recsize);
		ecbp->bufsiz = recsize;
		memcpy(ecbp->buf, ndlist, recsize);
		free(ndlist);
	}
	ke_change(kep->ident, EVFILT_READ, EV_DISABLE, kep->udata);
	ke_change(kep->ident, EVFILT_WRITE, EV_ENABLE, kep->udata);
getout:
	//label at end of compound statement
		len = 0;
}

static void
do_accept(register struct kevent const *const kep)
{
	auto sockaddr_in sin;
	auto socklen_t	sinsiz;
	register int	s;
	register ecb   *ecbp;

	if ((s = accept(kep->ident, (struct sockaddr *)&sin, &sinsiz)) == -1)
		fatal("Error in accept(): %s", strerror(errno));

	ecbp = (ecb *) xmalloc(sizeof(ecb));
	ecbp->do_read = do_read;
	ecbp->do_write = do_write;
	ecbp->buf = NULL;
	ecbp->bufsiz = 0;

	ke_change(s, EVFILT_READ, EV_ADD | EV_ENABLE, ecbp);
	ke_change(s, EVFILT_WRITE, EV_ADD | EV_DISABLE, ecbp);
}

static void 
event_loop(register int const kq)
__attribute__((__noreturn__));

	static void	event_loop(register int const kq)
{
	struct timespec	tv;
	char		tmp       [1024];

	for (;;) {
		register int	n;
		register struct kevent const *kep;

		tv.tv_sec = interval;
		tv.tv_nsec = 0;
		//tv.tv_usec = 0;

		//n = kevent(kq, ke_vec, ke_vec_used, ke_vec, ke_vec_alloc, NULL);
		n = kevent(kq, ke_vec, ke_vec_used, ke_vec, ke_vec_alloc, &tv);

		tv.tv_sec = 5;
		tv.tv_nsec = 0;

		ke_vec_used = 0;/* Already processed all changes.  */

		if (n == -1)
			fatal("Error in kevent(): %s", strerror(errno));
		//if (n == 0)
			//fatal("No events received!");
		//if (n == 0)
			nodeping();

		for (kep = ke_vec; kep < &ke_vec[n]; kep++) {
			register ecb const *const ecbp = (ecb *) kep->udata;
			if (kep->filter == EVFILT_READ)
				(*ecbp->do_read) (kep);
			else
				(*ecbp->do_write) (kep);
		}
	}
}

int
main(register int const argc, register char *const argv[])
{
	auto in_addr	listen_addr;
	register int	optch;
	auto int	one = 1;
	register int	portno = 0;
	register int	option_errors = 0;
	register int	server_sock;
	struct sockaddr_un sin;
	register servent *servp;
	auto ecb	listen_ecb;
	register int	kq;
	int		len;
	int		fd;
	FILE           *fp;
	struct sigaction osa, sa;
	pid_t		newgrp;
	int		oerrno;
	int		osa_ok;
	char           *cp;
	char           *mpath;

	(void)chdir("/");
	fd = open(_PATH_DEVNULL, O_RDWR, 0);
	(void)dup2(fd, STDIN_FILENO);
	(void)dup2(fd, STDOUT_FILENO);
	(void)dup2(fd, STDERR_FILENO);
	//if (fd > 2)
		(void)close(fd);

	/* A SIGHUP may be thrown when the parent exits below. */
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	osa_ok = sigaction(SIGHUP, &sa, &osa);

	signal(SIGCHLD, SIG_IGN);

	pid_t		p2 = fork();
	if (p2 != 0) {
		_exit(0);
	}
	pid_t		p3 = getpid();

	newgrp = setsid();

	fp = fopen(PIDFILE, "w");
	if (!fp) {
		tolog("Cant open pid file");
		_exit(1);
	}
	fprintf(fp, "%d\n", p3);
	fclose(fp);

	pname = strrchr(argv[0], '/');
	pname = pname ? pname + 1 : argv[0];

	listen_addr.s_addr = htonl(INADDR_ANY);	/* Default.  */

	while ((optch = getopt(argc, argv, "p:")) != EOF) {
		switch (optch) {
		case 'p':
			if (strlen(optarg) == 0 || !all_digits(optarg)) {
				error("Invalid argument for -p option: %s", optarg);
				option_errors++;
			}
			portno = atoi(optarg);
			if (portno == 0 || portno >= (1u << 16)) {
				error("Invalid argument for -p option: %s", optarg);
				option_errors++;
			}
			break;
		default:
			error("Invalid option: -%c", optch);
			option_errors++;
		}
	}

	if (option_errors || optind != argc)
		usage();

	if (portno == 0) {
		if ((servp = getservbyname(servname, protoname)) == NULL)
			fatal("Error getting port number for service `%s': %s",
			      servname, strerror(errno));
		portno = ntohs(servp->s_port);
	}
	//if ((server_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		if ((server_sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
			fatal("Error creating socket: %s", strerror(errno));

	memset(&sin, 0, sizeof sin);
	sin.sun_family = AF_UNIX;

	if ((cp = getenv("workdir")) == NULL)
		strcpy(sin.sun_path, SOCK_PATH);
	else {
		mpath = malloc(strlen(cp) + strlen(SOCK_PATH) + 2);
		//+2 = slash
			sprintf(mpath, "%s/%s", cp, SOCK_PATH);
		strcpy(sin.sun_path, mpath);
		free(mpath);
	}
	unlink(sin.sun_path);

	len = strlen(sin.sun_path) + sizeof(sin.sun_family) + 1;

	if (bind(server_sock, (const struct sockaddr *)&sin, len) == -1)
		fatal("Error binding socket: %s", strerror(errno));

	if (listen(server_sock, 20) == -1)
		fatal("Error listening to socket: %s", strerror(errno));

	if ((kq = kqueue()) == -1)
		fatal("Error creating kqueue: %s", strerror(errno));

	listen_ecb.do_read = do_accept;
	listen_ecb.do_write = NULL;
	listen_ecb.buf = NULL;
	listen_ecb.buf = 0;

	ke_change(server_sock, EVFILT_READ, EV_ADD | EV_ENABLE, &listen_ecb);
	load_basedb();
	event_loop(kq);
}

int 
make_node_list(char *ndlist, int size)
{
	int		i         , n = 0;
	struct nodedata *hs;
	char		tmp       [MAXNODELEN + MAXVALUELEN + 2];

	memset(ndlist, 0, strlen(ndlist));

	for (hs = nodelist; hs; hs = hs->next) {
		memset(tmp, 0, sizeof(tmp));
		sprintf(tmp, "%s %s\n", hs->node, hs->value);
		strcat(ndlist, tmp);
	}
	return 1;
}


int 
base_get(char *node, char *ndlist, int size)
{
	int		i         , n = 0;
	struct nodedata *hs;
	char		tmp       [MAXNODELEN + MAXVALUELEN + 2];

	memset(ndlist, 0, strlen(ndlist));

	for (hs = nodelist; hs; hs = hs->next) {
		if (!strcmp(hs->node, node))
			strcat(ndlist, hs->value);
	}
	return 1;
}


int 
tolog(char *buff)
{
	FILE           *fp;
	char           *error_log;
	char           *cp;

	if ((cp = getenv("workdir")) == NULL)
		error_log = ERROR_LOG;
	else {
		error_log = malloc(strlen(cp) + strlen(ERROR_LOG) + 2);
		//+2 - slash
			sprintf(error_log, "%s/%s", cp, ERROR_LOG);
	}

	fp = fopen(error_log, "a");

	if (!fp) {
		if (cp) {
			free(error_log);
			return 1;
		}
	}
	fputs(buff, fp);
	fclose(fp);

	if (cp)
		free(error_log);
	return 0;
}

int 
save_basedb()
{
	int		i         , n = 0;
	struct nodedata *hs;
	struct timeval	now_time;
	int		current_time;
	int		ende = 0;
	FILE           *fp;
	char           *dbfile;
	char           *cp;

	if ((cp = getenv("workdir")) == NULL)
		dbfile = DBFILE;
	else {
		dbfile = malloc(strlen(cp) + strlen(DBFILE) + 2);
		//+2 = slash
			sprintf(dbfile, "%s/%s", cp, DBFILE);
	}

	fp = fopen(dbfile, "w");

	if (!fp) {
		if (cp)
			free(dbfile);
		return 0;
	}
	for (hs = nodelist; hs; hs = hs->next)
		fprintf(fp, "%s %s\n", hs->node, hs->value);
	fclose(fp);
	if (cp)
		free(dbfile);
	return 0;
}

int 
load_basedb()
{
	int		i         , n = 0;
	struct nodedata *hs;
	char		tmp       [MAXNODELEN + MAXVALUELEN + 5];
	struct timeval	now_time;
	int		current_time;
	int		ende = 0;
	FILE           *fp;
	int		mtime = 0;
	char		node      [MAXNODELEN];
	char		value     [MAXVALUELEN];
	char           *dbfile;
	char           *cp;

	if ((cp = getenv("workdir")) == NULL)
		dbfile = DBFILE;
	else {
		dbfile = malloc(strlen(cp) + strlen(DBFILE) + 2);
		//+2 = slash
			sprintf(dbfile, "%s/%s", cp, DBFILE);
	}

	fp = fopen(dbfile, "r");

	if (!fp) {
		//printf("Cant open %s for read!\n", hardcode1);
		if (cp)
			free(dbfile);
		return 0;
	}
	while (!feof(fp)) {
		memset(tmp, 0, sizeof(tmp));
		fgets(tmp, 1024, fp);
		if (feof(fp))
			break;
		if (strlen(tmp) > 5) {
			memset(node, 0, sizeof(node));
			memset(value, 0, sizeof(value));
			sscanf(tmp, "%s %s\n", node, value);
			//if (feof(fp))
				break;
			base_add(node, value, 0);
		}
	}
	fclose(fp);
	free(dbfile);
	return 0;
}
