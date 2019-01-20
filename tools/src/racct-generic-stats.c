// CBSD Project 2017-2018
// CBSD Team <cbsd+subscribe@lists.tilda.center>
// 0.2
#include <sys/param.h>
#include <sys/jail.h>

#include <jail.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/rctl.h>
#include <sys/sysctl.h>
#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <grp.h>
#include <libutil.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <stddef.h>

#include <sys/user.h>
#include <sys/stat.h>

#include <libprocstat.h>
#include <limits.h>
#include <dirent.h>

#include <paths.h>

#include <assert.h>
#include <inttypes.h>

#include "beanstalk.h"

#include <pthread.h>
#include "sqlite3.h"

#define FALSE 0
#define TRUE 1

#define DEFSQLDELIMER "|"

#define SQLITE_BUSY_TIMEOUT 5000

#define	JP_USER		0x01000000
#define	JP_OPT		0x02000000

#define	PRINT_DEFAULT	0x01
#define	PRINT_HEADER	0x02
#define	PRINT_NAMEVAL	0x04
#define	PRINT_QUOTED	0x08
#define	PRINT_SKIP	0x10
#define	PRINT_VERBOSE	0x20
#define	PRINT_JAIL_NAME	0x40

#define MAXJNAME 100
#define LOG_MAX_LEN    1024 /* Default maximum length of syslog messages */

#define	RCTL_DEFAULT_BUFSIZE	128 * 1024

#define strlens(s) (s==NULL?0:strlen(s))

const char *current_jobs_ready_str="current-jobs-ready: ";
const char *current_waiting_str="current-waiting: ";

/* List of all args */
enum {
	C_HELP,
	C_LOG_FILE,
	C_LOG_LEVEL,
	C_LOOP_INTERVAL,
	C_PROMETHEUS_EXPORTER,
	C_SAVE_LOOP_COUNT,
	C_SAVE_BEANSTALKD,
	C_SAVE_SQLITE3,
};

//
int cur_round=0;	// current round
int log_level=0;    // default log_level

char *log_file = NULL;     // logfile
int save_loop_count=5;        // save_loop_count by default
int prometheus_exporter=0; // no prometheus_exporter by default
int loop_interval=1; // loop 1 seconds by default

static struct jailparam *params;
static int *param_parent;
static int nparams;

static int add_param(const char *name, void *value, size_t valuelen,
		struct jailparam *source, unsigned flags);
static int sort_param(const void *a, const void *b);
static int print_jail(int pflags, int jflags);

int update_racct_jail(char *, char *, int);

int64_t nanoseconds(void);

int init_bs(char *);
int tolog(int level, const char *fmt, ...);

int sum_jname_exist(char *);
int jname_exist(char *);
int remove_data_by_jname(char *);

int tosqlite3=0;			// by default: sqlite3 disabled, 0
int tobeanstalkd=0;			// by default: beanstalkd disabled, 0
int skip_beanstalk=0;			// skip for bs_put (no current-waiting)

int	bs_socket=-1;
int	bs_connected=0;
int	bs_tick;

int nullfd = -1;
struct pidfh *pidfile;
char *path_my_pidfile;

int ncpu;

static int	checkfile;		/* restrict to particular files or filesystems */
static int	mflg;			/* include memory-mapped files */

uint64_t	fsid;
uint64_t	ino;
char		*name;
char		*workdir = NULL;

static char *memf, *nlistf;
unsigned long maxmem = 0;		/* Hoster memory in bytes, for jail pmem calculation */

int getfname(char *filename);
pid_t dofiles(struct procstat *procstat, struct kinfo_proc *p);
pid_t print_file_info(struct procstat *procstat,
    struct filestat *fst, const char *uname, const char *cmd, int pid);

/* List of all provider */
enum {
	IS_JAIL,
};

int cur_jid=0;
int cur_bid=0;
char cur_jname[MAXJNAME];

struct item_data {
	char name[MAXJNAME];
	char orig_name[MAXJNAME];
	int pid;
	unsigned int cputime;
	unsigned int datasize;
	unsigned int stacksize;
	unsigned int coredumpsize;
	unsigned long memoryuse;
	unsigned int memorylocked;
	unsigned int maxproc;
	unsigned int openfiles;
	unsigned long vmemoryuse;
	unsigned int pseudoterminals;
	unsigned long swapuse;
	unsigned int nthr;
	unsigned int msgqqueued;
	unsigned int msgqsize;
	unsigned int nmsgq;
	unsigned int nsem;
	unsigned int nsemop;
	unsigned int nshm;
	unsigned int shmsize;
	unsigned int wallclock;
	unsigned int pcpu;
	unsigned int readbps;
	unsigned int writebps;
	unsigned int readiops;
	unsigned int writeiops;
	int64_t modified;
	unsigned int cpus;
	unsigned long maxmem;
	unsigned int pmem;
	struct item_data *next;
};

struct sum_item_data {
	char name[MAXJNAME];
	char orig_name[MAXJNAME];
	int pid;
	unsigned int cputime;
	unsigned int datasize;
	unsigned int stacksize;
	unsigned int coredumpsize;
	unsigned long memoryuse;
	unsigned int memorylocked;
	unsigned int maxproc;
	unsigned int openfiles;
	unsigned long vmemoryuse;
	unsigned int pseudoterminals;
	unsigned long swapuse;
	unsigned int nthr;
	unsigned int msgqqueued;
	unsigned int msgqsize;
	unsigned int nmsgq;
	unsigned int nsem;
	unsigned int nsemop;
	unsigned int nshm;
	unsigned int shmsize;
	unsigned int wallclock;
	unsigned int pcpu;
	unsigned int readbps;
	unsigned int writebps;
	unsigned int readiops;
	unsigned int writeiops;
	int64_t modified;
	unsigned int cpus;
	unsigned long maxmem;
	unsigned int pmem;
	struct sum_item_data *next;
};

struct item_data *item_list = NULL;
struct sum_item_data *sum_item_list = NULL;

/* memory utils **********************************************************/

#define CREATE(result, type, number)  do {\
	if (!((result) = (type *) calloc ((number), sizeof(type))))\
	{ perror("malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
	if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
	{ perror("realloc failure"); abort(); } } while(0)

#define REMOVE_FROM_LIST(item, head, next)      \
	if ((item) == (head))                \
		head = (item)->next;              \
	else {                               \
		temp = head;                      \
		while (temp && (temp->next != (item))) \
			temp = temp->next;             \
		if (temp)                         \
			temp->next = (item)->next;     \
	}

int tolog(int level, const char *fmt, ...) {
	va_list ap;
	char msg[LOG_MAX_LEN];
	FILE *fp;

	if (log_file==NULL)
		return 0;

	if (log_level==0)
		return 0;

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);

	fp=fopen(log_file,"a");
	if (!fp) return 1;
	fputs(msg,fp);
	fclose(fp);
	return 0;
}

char *
nm(void)
{
	return "sqlcli";
}

int
sqlitecmd(char *dbfile, char *query)
{
	sqlite3        *db;
	int             ret = 0;
	sqlite3_stmt   *stmt;
	int res=0;

	if (SQLITE_OK != (res = sqlite3_open(dbfile, &db))) {
		tolog(log_level,"%s: Can't open database file: %s\n", nm(), dbfile);
		return 1;
	}

	char *zErrMsg = 0;
	ret = sqlite3_exec(db, query, 0, 0, &zErrMsg);
	sqlite3_free(zErrMsg);

	sqlite3_close(db);
	return ret;
}

int
sql_get_int(sqlite3_stmt * stmt)
{
	int		icol, irow;
	const char	*colname;
	int		allcol;
	char		*delim;
	char		*cp;
	int		printheader = 0;
	char		*sqlcolnames = NULL;
	int		ret = 0;

	if (stmt == NULL)
		return 1;

	if ((cp = getenv("sqldelimer")) == NULL)
		delim = DEFSQLDELIMER;
	else
		delim = cp;

	sqlcolnames = getenv("sqlcolnames");
	allcol = sqlite3_column_count(stmt);

	for (icol = 0; icol < allcol; icol++) {
			if (icol == (allcol - 1))
				return atoi((char *)sqlite3_column_text(stmt, icol));
	}

	return 0;
}

unsigned long
sql_get_int64(sqlite3_stmt * stmt)
{
	int		icol, irow;
	const char	*colname;
	int		allcol;
	char		*delim;
	char		*cp;
	int		printheader = 0;
	char		*sqlcolnames = NULL;
	int		ret = 0;

	if (stmt == NULL)
		return 1;

	if ((cp = getenv("sqldelimer")) == NULL)
		delim = DEFSQLDELIMER;
	else
		delim = cp;

	sqlcolnames = getenv("sqlcolnames");
	allcol = sqlite3_column_count(stmt);

	for (icol = 0; icol < allcol; icol++) {
			if (icol == (allcol - 1))
				return atol((char *)sqlite3_column_text(stmt, icol));
	}

	return 0;
}

int get_bs_stats(char *yaml,const char *str)
{
	char *pch;
	int str_len=0;
	int str_with_val_len=0;
	int yaml_len=0;
	char *tmp;
	int values=-1;
	int i=0;
	int x;
	char *token = NULL;
	char *tofree;

	str_len=strlens(str);
	str_with_val_len=str_len+10;		// assume value not greated than: XXXXXXXXXX

	if(str_len==0)
		return -1;

	yaml_len=strlens(yaml);

	if(yaml_len==0)
		return -1;

	if (strlen(yaml)<str_len)
		return -1;

	pch = strstr(yaml, str);

	if(pch) {
		tmp=malloc(str_with_val_len);
		i=0;
		while ( pch[i]!='\n' ) {
			tmp[i]=pch[i];
			i++;
			if (i>=str_with_val_len)
				break;
		}
		tmp[i]='\0';
		//tolog(log_level,"get_bs_stats: found: [%s]\n",tmp);
		x=0;
		tofree = tmp;

		while ((token = strsep(&tmp, ":")) != NULL) {
			switch (x) {
				case 0:
					//tolog(log_level,"TOKEN: [%s]\n",token);
					break;
				case 1:
					//tolog(log_level,"TOKEN2: [%s]\n",token);
					sscanf(token,"%d",&values);
					break;
				}
				x++;
		}
		free(tofree);
		free(tmp);
	} else {
		tolog(log_level,"get_bs_stats: no [%s] here\n",str);
	}

	return values;
}

static void
enosys(void)
{
	int error, racct_enable;
	size_t racct_enable_len;

	racct_enable_len = sizeof(racct_enable);
	error = sysctlbyname("kern.racct.enable",&racct_enable, &racct_enable_len, NULL, 0);

	if (error != 0) {
		if (errno == ENOENT)
			errx(1, "RACCT/RCTL support not present in kernel; see rctl(8) for details");

		err(1, "sysctlbyname");
	}

	if (racct_enable == 0)
		errx(1, "RACCT/RCTL present, but disabled; enable using kern.racct.enable=1 tunable");
}



/* release memory allocated for a item struct */
void free_item(struct item_data * ch)
{
	free(ch);
}

// avg
int sum_jname_exist(char *jname)
{
	struct sum_item_data *ch, *next_ch;

	for (ch = sum_item_list; ch; ch = ch->next) {
		if (!strcmp(jname,ch->name)) {
			return 1;
		}
	}

	return 0;
}

int jname_exist(char *jname)
{
	struct item_data *target = NULL, *ch, *next_ch;

	for (ch = item_list; ch; ch = ch->next) {
		if (!strcmp(jname,ch->name)) {
			return 1;
		}
	}

	return 0;
}

int remove_data()
{
	struct item_data *target = NULL, *ch, *next_ch;
	struct item_data *temp;

	for (ch = item_list; ch; ch = ch->next) {
		tolog(log_level,"NAME: %s, PID %d, PCPU: %d\n",ch->name,ch->pid,ch->cputime);
		free(ch);
	}

	return 0;
}


int remove_data_by_jname(char *jname)
{
	struct item_data *target = NULL, *ch, *next_ch;
	struct item_data *temp;

	for (ch = item_list; ch; ch = ch->next) {
		if (!strcmp(jname,ch->name)) {
			REMOVE_FROM_LIST(ch, item_list, next);
			free(ch);
		}
	}

	return 0;
}


int prune_inactive_env()
{
	struct item_data *target = NULL, *ch, *next_ch;
	struct timeval  now_time;
	int cur_time = 0;

	gettimeofday(&now_time, NULL);
	cur_time = (time_t) now_time.tv_sec;

	for (ch = item_list; ch; ch = ch->next) {
		// new env?
		if ( ch->modified == 0 ) continue;
		// save_loop_count - number of node
		if ((cur_time - (int)(ch->modified / 1000000000)) > (20 * save_loop_count) ) {
			tolog(log_level,"!! Remove inactive env: %s\n",ch->name);
			remove_data_by_jname(ch->name);
		}
	}

	return 0;
}

int get_pid_by_name(char *jname)
{
	struct item_data *target = NULL, *ch, *next_ch;

	for (ch = item_list; ch; ch = ch->next) {
		if (!strcmp(jname,ch->name))
			return ch->pid;
	}

	return 0;
}


static void
usage(void)
{
	printf("CBSD racct statistics exporter\n");
	printf("require: --nic\n");
	printf("optional: --log_file=<file> --log_level=LEVEL --loop_interval=N --prometheus_exporter=[0|1] --save_loop_count=N --save_beanstalkd=[0|1] --save_sqlite3=[0|1]\n");
	exit(1);
}


int
init_bs(char *tube)
{
	int a,b,c;
	int socket = BS_STATUS_FAIL;
	bs_connected=0;

	bs_version(&a, &b, &c);
	tolog(log_level,"beanstalk-client version %d.%d.%d\n", a,b,c);

	while (socket == BS_STATUS_FAIL ) {
		socket = bs_connect("127.0.0.1", 11300);
		if (socket != BS_STATUS_FAIL)
			break;
		tolog(log_level,"Unable to connect to beanstalk 127.0.0.1:11300, sleep 10sec\n");
		sleep(10);
	}

	tolog(log_level,"Connected to BS: %s\n",tube);
	bs_connected=1;
	bs_use(socket, tube);
	bs_watch(socket, tube);
	bs_ignore(socket, "default");

	return socket;
}

// return time in nanoseconds
// to convert to integrer/seconds:
// printf("%d\n",(int)( nanoseconds() / 1000000000));
int64_t
nanoseconds(void)
{
	int r;
	struct timeval tv;

	r = gettimeofday(&tv, 0);
	if (r != 0) return warnx("gettimeofday"), -1; // can't happen

	return ((int64_t)tv.tv_sec)*1000000000 + ((int64_t)tv.tv_usec)*1000;
}
