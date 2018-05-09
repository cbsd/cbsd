// CBSD Project 2017
// Oleg Ginzburg <olevole@olevole.ru>
// 0.1
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

#include <pthread.h>
#include "sqlite3.h"

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

#define MAXJNAME 1024

#define	RCTL_DEFAULT_BUFSIZE	128 * 1024

static struct jailparam *params;
static int *param_parent;
static int nparams;

static int add_param(const char *name, void *value, size_t valuelen,
		struct jailparam *source, unsigned flags);
static int sort_param(const void *a, const void *b);
static int print_jail(int pflags, int jflags);

int nullfd = -1;
struct pidfh *pidfile;
char *path_my_pidfile;

int ncpu;

static int	checkfile; /* restrict to particular files or filesystems */
static int	mflg;	/* include memory-mapped files */

uint64_t	fsid;
uint64_t	ino;
char		*name;
char		*workdir = NULL;

static char *memf, *nlistf;

int getfname(char *filename);
pid_t dofiles(struct procstat *procstat, struct kinfo_proc *p);
pid_t print_file_info(struct procstat *procstat,
    struct filestat *fst, const char *uname, const char *cmd, int pid);

/* List of all provider */
enum {
	IS_JAIL,
	IS_BHYVE,
};

int cur_jid=0;
int cur_bid=0;
char cur_jname[MAXJNAME];

struct item_data {
	char name[MAXJNAME];
	int emulator;
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
	unsigned int modified;
	unsigned int cpus;
	struct item_data *next;
};

struct item_data *item_list = NULL;

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
		printf("%s: Can't open database file: %s\n", nm(), dbfile);
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

pid_t
dofiles(struct procstat *procstat, struct kinfo_proc *kp)
{
	const char *cmd;
	// const char *uname;
	//at the moment, only root user can run bhyve
	const char *uname="root";
	struct filestat *fst = NULL;
	struct filestat_list *head;
	pid_t pid = 0;
	pid_t vm_pid = 0;
	//at the moment, only root user can run bhyve
	// uname = user_from_uid(kp->ki_uid, 0);
	pid = kp->ki_pid;
	cmd = kp->ki_comm;

	head = procstat_getfiles(procstat, kp, mflg);

	if (head == NULL)
		return -1;
	STAILQ_FOREACH(fst, head, next) {
		//processing only for bhyve command
		if (!strcmp(cmd,"bhyve")) {
			vm_pid = print_file_info(procstat, fst, uname, cmd, pid);
			if (vm_pid > 0) break;
		}
		if (vm_pid > 0) break;
	}
	procstat_freefiles(procstat, head);
	return vm_pid;
}


pid_t print_file_info(struct procstat *procstat, struct filestat *fst, const char *uname, const char *cmd, int pid)
{
	struct vnstat vn;
	int error, fsmatch = 0;

	if (checkfile == 0)
		return 0;

	if (fst->fs_type != PS_FST_TYPE_VNODE && fst->fs_type != PS_FST_TYPE_FIFO)
		return -1;
	error = procstat_get_vnode_info(procstat, fst, &vn, NULL);
	if (error != 0)
		return -1;

	if (fsid == vn.vn_fsid) {
		if (ino == vn.vn_fileid) {
			//no memleak?, struct from /src/lib/libprocstat/libprocstat.h
			free(vn.vn_mntdir);
			return pid;
		}
	}

	//no? memleak, struct from /src/lib/libprocstat/libprocstat.h
	free(vn.vn_mntdir);
	return 0;
}

// store filename data (inode, fsid)
int getfname(char *filename)
{
	struct stat statbuf;

	if (stat(filename, &statbuf)) {
		warn("%s", filename);
		return (0);
	}

	ino = statbuf.st_ino;
	fsid = statbuf.st_dev;
	name = filename;

	return (1);
}


pid_t get_vm_pid(char *vmpath)
{
	struct kinfo_proc *p;
	struct procstat *procstat;
	int arg, ch, what;
	int i;
	unsigned int cnt;
	pid_t vmpid;

	arg = 0;
	what = KERN_PROC_PROC;
	nlistf = memf = NULL;

	if (getfname(vmpath))
		checkfile = 1;

	if (!checkfile)	/* file(s) specified, but none accessible */
		return -1;

	procstat = procstat_open_sysctl();
	if (procstat == NULL)
		errx(1, "procstat_open()");

	p = procstat_getprocs(procstat, what, arg, &cnt);
	if (p == NULL)
		errx(1, "procstat_getprocs()");

	/*
	 * Go through the process list.
	 */
	for (i = 0; i < cnt; i++) {
		if (p[i].ki_stat == SZOMB)
			continue;
		vmpid = dofiles(procstat, &p[i]);
		if (vmpid > 0) break;
	}
	procstat_freeprocs(procstat, p);
	procstat_close(procstat);
	return vmpid;
}

static void
enosys(void)
{
	int error, racct_enable;
	size_t racct_enable_len;

	racct_enable_len = sizeof(racct_enable);
	error = sysctlbyname("kern.racct.enable",
	    &racct_enable, &racct_enable_len, NULL, 0);

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

int list_data()
{
	struct item_data *target = NULL, *ch, *next_ch;
	char sql[512];
	char stats_file[1024];
	FILE *fp;
	int ret=0;

	for (ch = item_list; ch; ch = ch->next) {
		if (ch->modified==0) continue;
		//printf("DD\n");
		printf("TYPE: %d, NAME: %s, PID %d, PCPU: %d, MEM: %lu, PROC: %d, OPENFILES: %d, RB: %d, WB: %d, RIO: %d, WIO: %d\n",ch->emulator,ch->name,ch->pid,ch->pcpu,ch->memoryuse,ch->maxproc,
			ch->openfiles, ch->readbps, ch->writebps, ch->readiops, ch->writeiops);

		memset(sql,0,sizeof(sql));
		memset(stats_file,0,sizeof(stats_file));
		sprintf(stats_file,"%s/jails-system/%s/racct.sqlite",workdir,ch->name);
		fp=fopen(stats_file,"r");
		if (!fp) {
			printf("RACCT not exist, create via updatesql\n");
			sprintf(sql,"/usr/local/bin/cbsd %s/misc/updatesql %s /usr/local/cbsd/share/racct.schema racct",workdir,stats_file);
			system(sql);
			//write into base in next loop (protection if jail was removed in directory not exist anymore
			continue;
		} else {
			fclose(fp);
		}

		sprintf(sql,"INSERT INTO racct ( idx,memoryuse,maxproc,openfiles,pcpu,readbps,writebps,readiops,writeiops ) VALUES ( '%d', '%lu', '%d', '%d', '%d', '%d', '%d', '%d', '%d' );\n",
			ch->modified, ch->memoryuse, ch->maxproc, ch->openfiles, ch->pcpu, ch->readbps, ch->writebps, ch->readiops, ch->writeiops );
		ret=sqlitecmd(stats_file,sql);
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
		printf("NAME: %s, PID %d, PCPU: %d\n",ch->name,ch->pid,ch->cputime);
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
		if ( ch->modified ==0 ) continue;
		if ((cur_time - ch->modified)>10) {
			printf("Remove inactive env: %s\n",ch->name);
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

int update_racct_jail(char *jname, int jid)
{
	struct item_data *target = NULL, *ch, *next_ch;
	struct timeval  now_time;
	int cur_time = 0;
	int error;
	char *copy, *outbuf = NULL, *tmp;
	char *var;
	size_t outbuflen = RCTL_DEFAULT_BUFSIZE / 4;
	int store=0;
	char param_name[512];
	char filter[MAXJNAME+7], unexpanded_rule[MAXJNAME+7];		//7 - extra "jail::\0"

	sprintf(filter,"jail:%s:",jname);
	sprintf(unexpanded_rule,"jail:%s",jname);

	gettimeofday(&now_time, NULL);
	cur_time = (time_t) now_time.tv_sec;

	for (ch = item_list; ch; ch = ch->next) {
		if (!strcmp(jname,ch->name)) {
			ch->modified = (time_t) now_time.tv_sec;
			ch->pid=cur_jid;
			ch->emulator = IS_JAIL;

			for (;;) {
				outbuflen *= 4;
				outbuf = realloc(outbuf, outbuflen);
				if (outbuf == NULL)
					err(1, "realloc");
				error = rctl_get_racct(filter, strlen(filter) + 1, outbuf, outbuflen);
				if (error == 0)
					break;
				if (errno == ERANGE)
					continue;
				if (errno == ENOSYS)
					enosys();
				warn("failed to show resource consumption for '%s'",unexpanded_rule);
				free(outbuf);
				return (error);
			}
		copy = outbuf;
		int i = 0;
		while ((tmp = strsep(&copy, ",")) != NULL) {
			if (tmp[0] == '\0')
				break; /* XXX */
			while ((var = strsep(&tmp, "=")) != NULL) {
				i++;
				if (var[0] == '\0')
					break; /* XXX */
				if (i==1) {
					memset(param_name,0,sizeof(param_name));
					strcpy(param_name,var);
				}
				if (i==2) {
					//printf("val* %s\n",var);
					if (!strcmp(param_name,"cputime")) {
						ch->cputime=atoi(var);
					} else if (!strcmp(param_name,"datasize")) {
						ch->datasize=atoi(var);
					} else if (!strcmp(param_name,"stacksize")) {
						ch->stacksize=atoi(var);
					} else if (!strcmp(param_name,"memoryuse")) {
						ch->memoryuse=atoi(var);
					} else if (!strcmp(param_name,"memorylocked")) {
						ch->memorylocked=atoi(var);
					} else if (!strcmp(param_name,"maxproc")) {
						ch->maxproc=atoi(var);
					} else if (!strcmp(param_name,"openfiles")) {
						ch->openfiles=atoi(var);
					} else if (!strcmp(param_name,"vmemoryuse")) {
						ch->vmemoryuse=atoi(var);
					} else if (!strcmp(param_name,"swapuse")) {
						ch->swapuse=atoi(var);
					} else if (!strcmp(param_name,"nthr")) {
						ch->nthr=atoi(var);
					} else if (!strcmp(param_name,"pcpu")) {
						if (ncpu>1) {
							ch->pcpu=( atoi(var) / ncpu );
						} else {
							ch->pcpu=atoi(var);
						}
						if (ch->pcpu<0)
							ch->pcpu=0;
						if (ch->pcpu>100)
							ch->pcpu=100;
					} else if (!strcmp(param_name,"readbps")) {
						ch->readbps=atoi(var);
					} else if (!strcmp(param_name,"writebps")) {
						ch->writebps=atoi(var);
					} else if (!strcmp(param_name,"readiops")) {
						ch->readiops=atoi(var);
					} else if (!strcmp(param_name,"writeiops")) {
						ch->writeiops=atoi(var);
					}
					i=0;
					}
			}
		}
		free(outbuf);
		}
	}

	return 0;
}

int
get_bhyve_cpus(char *vmname)
{
	sqlite3		*db;
	int		res;
	int		i;
	char		query[1024];
	char		*err = NULL;
	int		maxretry = 10;
	int		retry = 0;
	sqlite3_stmt	*stmt;
	int		ret;
	char		dbfile[1024];
	int		vm_cpus=0;

	memset(dbfile,0,sizeof(dbfile));
	sprintf(dbfile,"%s/var/db/local.sqlite",workdir);

	if (SQLITE_OK != (res = sqlite3_open(dbfile, &db))) {
		printf("%s: Can't open database file: %s\n", nm(), dbfile);
		return 1;
	}

	res=1024;

	sprintf(query,"SELECT vm_cpus FROM bhyve WHERE jname='%s' LIMIT 1",vmname);
	ret = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);

	if (ret == SQLITE_OK) {
		ret = sqlite3_step(stmt);

		while ( ret == SQLITE_ROW ) {
			vm_cpus=sql_get_int(stmt);
			ret = sqlite3_step(stmt);
		}
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return vm_cpus;
}

int update_racct_bhyve(char *vmname, char *vmpath)
{
	struct item_data *target = NULL, *ch, *next_ch;
	struct timeval  now_time;
	int cur_time = 0;
	int error;
	char *copy, *outbuf = NULL, *tmp;
	char *var;
	size_t outbuflen = RCTL_DEFAULT_BUFSIZE / 4;
	int store=0;
	char param_name[512];
	char filter[MAXJNAME+10], unexpanded_rule[MAXJNAME+10];		//10 - extra "process::\0"
	pid_t oldpid = 0;
	int vm_cpus = 0;

	sprintf(filter,"process:%d:",cur_bid);
	sprintf(unexpanded_rule,"process:%d",cur_bid);

	gettimeofday(&now_time, NULL);
	cur_time = (time_t) now_time.tv_sec;

	for (ch = item_list; ch; ch = ch->next) {
		if (!strcmp(vmname,ch->name)) {
			oldpid = ch->pid;
			ch->modified = cur_time;
			ch->pid=cur_bid;
			ch->emulator = IS_BHYVE;

			if (oldpid != cur_bid) {
				// if PID change, get CPUs from bhyve table for ncpu value
				vm_cpus=get_bhyve_cpus(vmname);
				if (vm_cpus == 0) return 0;
				printf("* VM PID WAS CHANGES, UPDATE CPUS: %d\n", vm_cpus);
				ch->cpus = vm_cpus;
			} else {
				vm_cpus = ch->cpus;
			}

		for (;;) {
			outbuflen *= 4;
			outbuf = realloc(outbuf, outbuflen);
			if (outbuf == NULL)
				err(1, "realloc");
			error = rctl_get_racct(filter, strlen(filter) + 1, outbuf, outbuflen);
			if (error == 0)
				break;
			if (errno == ERANGE)
				continue;
			if (errno == ENOSYS)
				enosys();
			warn("failed to show resource consumption for '%s'",unexpanded_rule);
			free(outbuf);
			return (error);
		}
		copy = outbuf;
		int i = 0;
		while ((tmp = strsep(&copy, ",")) != NULL) {
			if (tmp[0] == '\0')
				break; /* XXX */
			while ((var = strsep(&tmp, "=")) != NULL) {
				i++;
				if (var[0] == '\0') {
					free(tmp);
					break; /* XXX */
					}
				if (i==1) {
					memset(param_name,0,sizeof(param_name));
					strcpy(param_name,var);
				}
				if (i==2) {
					//printf("val* %s\n",var);
					if (!strcmp(param_name,"cputime")) {
						ch->cputime=atoi(var);
					} else if (!strcmp(param_name,"datasize")) {
						ch->datasize=atoi(var);
					} else if (!strcmp(param_name,"stacksize")) {
						ch->stacksize=atoi(var);
					} else if (!strcmp(param_name,"memoryuse")) {
						ch->memoryuse=atoi(var);
					} else if (!strcmp(param_name,"memorylocked")) {
						ch->memorylocked=atoi(var);
					} else if (!strcmp(param_name,"maxproc")) {
						ch->maxproc=atoi(var);
					} else if (!strcmp(param_name,"openfiles")) {
						ch->openfiles=atoi(var);
					} else if (!strcmp(param_name,"vmemoryuse")) {
						ch->vmemoryuse=atoi(var);
					} else if (!strcmp(param_name,"swapuse")) {
						ch->swapuse=atoi(var);
					} else if (!strcmp(param_name,"nthr")) {
						ch->nthr=atoi(var);
					} else if (!strcmp(param_name,"pcpu")) {
						if (vm_cpus>1) {
							ch->pcpu=( atoi(var) / vm_cpus );
						} else {
							ch->pcpu=atoi(var);
						}
					} else if (!strcmp(param_name,"readbps")) {
						ch->readbps=atoi(var);
					} else if (!strcmp(param_name,"writebps")) {
						ch->writebps=atoi(var);
					} else if (!strcmp(param_name,"readiops")) {
						ch->readiops=atoi(var);
					} else if (!strcmp(param_name,"writeiops")) {
						ch->writeiops=atoi(var);
					}
					i=0;
					}
			}
		}

		free(tmp);
		free(var);
		free(outbuf);
		free(copy);
		}
	}

	return 0;
}

int
main(int argc, char **argv)
{
	char *dot, *ep, *jname, *pname;
	int c, i, jflags, jid, lastjid, pflags, spc;
	struct item_data *newd;
	struct item_data *temp;
	struct timeval  now_time;
	size_t ncpu_len;
	DIR *dirp;
	int bhyve_exist=1;
	int jail_exist=1;

	struct dirent *dp;
	char vmname[MAXJNAME];
	char vmpath[MAXJNAME];
	pid_t vmpid;

	ncpu_len = sizeof(ncpu);

	jname = NULL;
	pflags = jflags = jid = 0;

	int pipe_fd[2];
	pid_t otherpid;
	char name[]="racct-statsd";

	workdir=getenv("workdir");

	if (workdir == NULL ) {
		printf("no workdir env\n");
		exit(1);
	}

	if (pipe(pipe_fd) == -1) {
		printf("pipe");
		exit(-1);
	}

	switch (fork())	{
		case -1:
			printf("cannot fork");
			exit(-1);
		case 0:
			break;
		default:
			return (0);
		}

	// fork
	setproctitle("%s", name);

	setsid();
	dup2(nullfd, STDIN_FILENO);
	dup2(nullfd, STDOUT_FILENO);
	dup2(nullfd, STDERR_FILENO);
	close(nullfd);
	//close(pipe_fd[0]);
	//close(pipe_fd[1]);

	if (path_my_pidfile == NULL) {
		asprintf(&path_my_pidfile,
			"%sracct-statsd.pid", _PATH_VARRUN);
		if (path_my_pidfile == NULL) {
			printf("asprintf");
			exit(1);
			}
	}
	pidfile = pidfile_open(path_my_pidfile, 0644, &otherpid);
	if (pidfile == NULL) {
		if (errno == EEXIST) {
			printf("dhclient already running, pid: %d.", otherpid);
			exit(1);
			}
		if (errno == EAGAIN) {
			printf("dhclient already running.");
			exit(1);
			}
		printf("Cannot open or create pidfile: %s",path_my_pidfile);
	}

	if (pidfile != NULL)
		pidfile_write(pidfile);

	i = sysctlbyname("hw.ncpu",&ncpu, &ncpu_len, NULL, 0);

	if (i != 0) {
		if (errno == ENOENT)
			errx(1, "Unable to determine CPU count via hw.ncpu");
		err(1, "sysctlbyname");
	}

	/* Add the parameters to print. */
	add_param("jid", NULL, (size_t)0, NULL, JP_USER);
	add_param("name", NULL, (size_t)0, NULL, JP_USER);
	add_param("lastjid", &lastjid, sizeof(lastjid), NULL, 0);

	c=0;

	while ( 1 ) {
	//jail area
	if (jail_exist==1) {
		for (lastjid=0; lastjid>=0; ) {
			memset(cur_jname,0,sizeof(cur_jname));
			lastjid = print_jail(pflags, jflags);
			if (cur_jid == 0) continue;
			if (strlen(cur_jname)<1) continue;
			i=jname_exist(cur_jname);

			if (i) {
				update_racct_jail(cur_jname,cur_jid);
				continue;
			}
			CREATE(newd, struct item_data, 1);
			newd->cputime=lastjid;
			newd->pid=cur_jid;
			newd->modified = 0; // sign of new jail
			newd->next = item_list;
			item_list = newd;
			strcpy(newd->name,cur_jname);
			printf("[jail] %d [%s] has beed added\n",cur_jid,cur_jname);
		}
	}

	//bhyve area
	dirp = opendir("/dev/vmm");
	if (dirp == NULL)
		bhyve_exist=0;
	else
		bhyve_exist=1;

	if (bhyve_exist==1) {
		while ((dp = readdir(dirp)) != NULL) {
			if (dp->d_name[0]=='.') continue;
			memset(vmname,0,sizeof(vmname));
			memset(vmpath,0,sizeof(vmpath));
			sprintf(vmpath,"/dev/vmm/%s",dp->d_name);
			strcpy(vmname,dp->d_name);
			cur_bid=0;
			cur_bid = get_vm_pid(vmpath);
			if (cur_bid == 0) continue;

			i=jname_exist(vmname);
			if (i) {
				update_racct_bhyve(vmname,vmpath);
				continue;
			}

			CREATE(newd, struct item_data, 1);

			newd->pid=cur_bid;
			strcpy(newd->name,vmname);
			newd->modified = 0; // sign of new jail

			newd->next = item_list;
			item_list = newd;

			printf("[bhyve] %d [%s] has beed added\n",cur_jid,vmname);
		}
		free(dp);
		(void)closedir(dirp);
	}

	c++;
	list_data();
	if (c>5) {
		prune_inactive_env();
		c=0;
	}
	sleep(1);
	}

	if (pidfile != NULL)
		pidfile_remove(pidfile);
	return 0;

}

static int
add_param(const char *name, void *value, size_t valuelen,
    struct jailparam *source, unsigned flags)
{
	struct jailparam *param, *tparams;
	int i, tnparams;

	static int paramlistsize;

	/* The pseudo-parameter "all" scans the list of available parameters. */
	if (!strcmp(name, "all")) {
		tnparams = jailparam_all(&tparams);
		if (tnparams < 0) {
			printf("error: %s", jail_errmsg);
			return 1;
			}
		qsort(tparams, (size_t)tnparams, sizeof(struct jailparam),
		    sort_param);
		for (i = 0; i < tnparams; i++)
			add_param(tparams[i].jp_name, NULL, (size_t)0,
			    tparams + i, flags);
		free(tparams);
		return -1;
	}

	/* Check for repeat parameters. */
	for (i = 0; i < nparams; i++)
		if (!strcmp(name, params[i].jp_name)) {
			if (value != NULL && jailparam_import_raw(params + i,
			    value, valuelen) < 0) {
				printf("error: %s", jail_errmsg);
				return 1;
				}
			params[i].jp_flags |= flags;
			if (source != NULL)
				jailparam_free(source, 1);
			return i;
		}

	/* Make sure there is room for the new param record. */
	if (!nparams) {
		paramlistsize = 32;
		params = malloc(paramlistsize * sizeof(*params));
		param_parent = malloc(paramlistsize * sizeof(*param_parent));
		if (params == NULL || param_parent == NULL) {
			printf("malloc");
			return 1;
			}
	} else if (nparams >= paramlistsize) {
		paramlistsize *= 2;
		params = realloc(params, paramlistsize * sizeof(*params));
		param_parent = realloc(param_parent, paramlistsize * sizeof(*param_parent));
		if (params == NULL || param_parent == NULL) {
			printf("realloc");
			return 1;
			}
	}

	/* Look up the parameter. */
	param_parent[nparams] = -1;
	param = params + nparams++;
	if (source != NULL) {
		*param = *source;
		param->jp_flags |= flags;
		return param - params;
	}
	if (jailparam_init(param, name) < 0 ||
		(value != NULL ? jailparam_import_raw(param, value, valuelen)
		: jailparam_import(param, value)) < 0) {
		if (flags & JP_OPT) {
			nparams--;
			return (-1);
		}
		printf("error: %s", jail_errmsg);
		return 1;
	}
	param->jp_flags = flags;
	return param - params;
}

static int
sort_param(const void *a, const void *b)
{
	const struct jailparam *parama, *paramb;
	char *ap, *bp;

	/* Put top-level parameters first. */
	parama = a;
	paramb = b;
	ap = strchr(parama->jp_name, '.');
	bp = strchr(paramb->jp_name, '.');
	if (ap && !bp)
		return (1);
	if (bp && !ap)
		return (-1);
	return (strcmp(parama->jp_name, paramb->jp_name));
}

static int
print_jail(int pflags, int jflags)
{
	char *nname;
	char **param_values;
	int i, ai, jid, count, n, spc;

	jid = jailparam_get(params, nparams, jflags);
	if (jid < 0)
		return jid;

	cur_jid=*(int *)params[0].jp_value;
	strcpy(cur_jname,(char *)params[1].jp_value);

	return jid;
}
