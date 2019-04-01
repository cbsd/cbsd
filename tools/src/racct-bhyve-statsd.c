// CBSD Project 2017-2018
// CBSD Team <cbsd+subscribe@lists.tilda.center>
// 0.2
#include <sys/param.h>

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

#include "racct-generic-stats.c"

int update_racct_bhyve(char *, char *, char *);
int sum_data_bhyve();
int list_data();

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
	ino = 0;
	fsid = 0;

	if (stat(filename, &statbuf)) {
		warn("%s", filename);
		return 0;
	}

	ino = statbuf.st_ino;
	fsid = statbuf.st_dev;
	name = filename;

	if ((ino == 0)||(fsid==0)) return 0;
	return 1;
}


pid_t get_vm_pid(char *vmpath)
{
	struct kinfo_proc *p;
	struct procstat *procstat = NULL;
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

int sum_data_bhyve()
{
	struct item_data *target = NULL, *ch, *next_ch;
	char sql[512];
	char stats_file[1024];
	int ret=0;
	FILE *fp;
	char json_str[20000];		// todo: dynamic from number of bhyve/jails
	char json_buf[1024];		// todo: dynamic from number of bhyve/jails
	int i;
	struct timeval  now_time;
	int cur_time = 0;
	int round_total=save_loop_count+1;

	struct sum_item_data *newd;
	struct sum_item_data *temp;
	struct sum_item_data *sumch, *next_sumch;

	tolog(log_level,"\n ***---calc bhyve avgdata---*** \n");

	gettimeofday(&now_time, NULL);
	cur_time = (time_t) now_time.tv_sec;

	for (ch = item_list; ch; ch = ch->next) {
		if (ch->modified==0) continue;
		if(strlen(ch->orig_name)<1) continue;
		i=sum_jname_exist(ch->orig_name);

		if (i) {
			for (sumch = sum_item_list; sumch; sumch = sumch->next) {
				if (!strcmp(ch->orig_name,sumch->name)) {
					sumch->modified+=ch->modified;
					sumch->pcpu+=ch->pcpu;
					sumch->memoryuse+=ch->memoryuse;
					sumch->maxproc+=ch->maxproc;
					sumch->openfiles+=ch->openfiles;
					sumch->readbps+=ch->readbps;
					sumch->writebps+=ch->writebps;
					sumch->readiops+=ch->readiops;
					sumch->writeiops+=ch->writeiops;
					sumch->pmem+=ch->pmem;
					break;
				}
			}
		} else {
			CREATE(newd, struct sum_item_data, 1);
			newd->modified=ch->modified;
			newd->pcpu=ch->pcpu;
			newd->memoryuse=ch->memoryuse;
			newd->maxproc=ch->maxproc;
			newd->openfiles=ch->openfiles;
			newd->readbps=ch->readbps;
			newd->writebps=ch->writebps;
			newd->readiops=ch->readiops;
			newd->writeiops=ch->writeiops;
			newd->pmem=ch->pmem;
			newd->next = sum_item_list;
			sum_item_list = newd;
			strcpy(newd->name,ch->orig_name);
			tolog(log_level,"[AVGSUM] !! %s struct has beed added\n",newd->name);
		}
	}

	memset(json_str,0,sizeof(json_str));
	for (sumch = sum_item_list; sumch; sumch = sumch->next) {
		if(strlen(sumch->name)<1) continue;
		tolog(log_level," ***[%s]SUM|PCPU:%d,MEM:%ld,PROC:%d,OPENFILES:%d,RBPS:%d,WBPS:%d,RIOPS:%d,WIOPS:%d,PMEM:%d,TIME:%ld\n",sumch->name,
		sumch->pcpu/round_total,
		sumch->memoryuse/round_total,
		sumch->maxproc/round_total,
		sumch->openfiles/round_total,
		sumch->readbps/round_total,
		sumch->writebps/round_total,
		sumch->readiops/round_total,
		sumch->writeiops/round_total,
		sumch->pmem/round_total,
		sumch->modified/round_total);
		memset(json_buf,0,sizeof(json_buf));
		sprintf(json_buf,"{\"name\": \"%s\",\"time\": %d,\"pcpu\": %d,\"pmem\": %d,\"readbps\": %d,\"writebps\": %d,\"readiops\": %d,\"writeiops\": %d }",sumch->name,
		cur_time,sumch->pcpu/round_total,sumch->pmem/round_total,sumch->readbps/round_total,sumch->writebps/round_total,sumch->readiops/round_total,sumch->writeiops/round_total);

		if (strlen(json_str)>2) {
			strcat(json_str,",");
			strcat(json_str,json_buf);
		} else {
			strcpy(json_str,"{ \"tube\":\"racct-bhyve\", \"data\":[");
			strcat(json_str,json_buf);
		}
		
		if (tosqlite3==1) {
			memset(sql,0,sizeof(sql));
			memset(stats_file,0,sizeof(stats_file));
			sprintf(stats_file,"%s/jails-system/%s/racct.sqlite",workdir,sumch->name);
			fp=fopen(stats_file,"r");
			if (!fp) {
				tolog(log_level,"RACCT not exist, create via updatesql\n");
				sprintf(sql,"/usr/local/bin/cbsd /usr/local/cbsd/misc/updatesql %s /usr/local/cbsd/share/racct.schema racct",stats_file);
				system(sql);
				//write into base in next loop (protection if jail was removed in directory not exist anymore
				continue;
			} else {
				fclose(fp);
			}

			sprintf(sql,"INSERT INTO racct ( idx,memoryuse,maxproc,openfiles,pcpu,readbps,writebps,readiops,writeiops,pmem ) VALUES ( '%d', '%lu', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d' );\n",
				cur_time, sumch->memoryuse/round_total, sumch->maxproc/round_total, sumch->openfiles/round_total, sumch->pcpu/round_total, sumch->readbps/round_total, sumch->writebps/round_total,
				sumch->readiops/round_total, sumch->writeiops/round_total, sumch->pmem/round_total);
			tolog(log_level,"Save to SQL: %s [%s]\n",stats_file,sql);
			ret=sqlitecmd(stats_file,sql);
		}
		sumch->modified=0;
		sumch->pcpu=0;
		sumch->memoryuse=0;
		sumch->maxproc=0;
		sumch->openfiles=0;
		sumch->readbps=0;
		sumch->writebps=0;
		sumch->readiops=0;
		sumch->writeiops=0;
		sumch->pmem=0;

		remove_data_by_jname(sumch->name);
	}

	strcat(json_str,"]}");
	bs_tick=0;

	if(cur_round!=save_loop_count)
		return 0;

	if(tobeanstalkd==0)
		skip_beanstalk=1;

	if (skip_beanstalk==1)
		return 0;
    
	if (strlen(json_str)>3) {
		tolog(log_level,"bs_put: (%s)\n",json_str);
		ret=bs_put(bs_socket, 0, 0, 0, json_str, strlen(json_str));
		if(ret > 0) {
			bs_tick=1;
		} else {
			tolog(log_level,"bs_put failed, trying to reconnect...\n");
			bs_disconnect(bs_socket);
			bs_connected=0;
			return 1;
		}
	} else {
		tolog(log_level,"skip_beanstalk = 1,skipp\n");
	}
	return 0;
}

int
get_bhyve_cpus(char *vmname)
{
	sqlite3		*db;
	char		query[40];
	char		*err = NULL;
	int		maxretry = 10;
	int		retry = 0;
	sqlite3_stmt	*stmt;
	int		ret;
	char		dbfile[512];
	int		vm_cpus=0;

	memset(dbfile,0,sizeof(dbfile));
	sprintf(dbfile,"%s/jails-system/%s/local.sqlite",workdir,vmname);

	if (SQLITE_OK != (ret = sqlite3_open(dbfile, &db))) {
		tolog(log_level,"%s: Can't open database file: %s\n", nm(), dbfile);
		return 1;
	}

	sprintf(query,"SELECT vm_cpus FROM settings LIMIT 1");
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

unsigned long
get_vm_pid_from_sql(char *vmname)
{
	sqlite3		*db;
	char		query[100];
	char		*err = NULL;
	int		maxretry = 10;
	int		retry = 0;
	sqlite3_stmt	*stmt;
	int		ret;
	char		dbfile[512];
	unsigned long	jid=0;

	memset(dbfile,0,sizeof(dbfile));
	sprintf(dbfile,"%s/var/db/local.sqlite",workdir);

	if (SQLITE_OK != (ret = sqlite3_open(dbfile, &db))) {
		tolog(log_level,"%s: Can't open database file: %s\n", nm(), dbfile);
		return 1;
	}

	memset(query,0,sizeof(query));
	sprintf(query,"SELECT jid FROM jails WHERE jname=\"%s\"",vmname);
	//tolog(log_level,"SQL[%s](%s)",query,dbfile);
	ret = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);

	if (ret == SQLITE_OK) {
		ret = sqlite3_step(stmt);

		while ( ret == SQLITE_ROW ) {
			jid=sql_get_int64(stmt);
			ret = sqlite3_step(stmt);
		}
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return jid;
}

unsigned long
get_bhyve_maxmem(char *vmname)
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
	unsigned long	maxmem=0;

	memset(dbfile,0,sizeof(dbfile));
	sprintf(dbfile,"%s/jails-system/%s/local.sqlite",workdir,vmname);

	if (SQLITE_OK != (res = sqlite3_open(dbfile, &db))) {
		tolog(log_level,"%s: Can't open database file: %s\n", nm(), dbfile);
		return 1;
	}

	res=1024;

	sprintf(query,"SELECT vm_ram FROM settings LIMIT 1");
	ret = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);

	if (ret == SQLITE_OK) {
		ret = sqlite3_step(stmt);

		while ( ret == SQLITE_ROW ) {
			maxmem=sql_get_int64(stmt);
			ret = sqlite3_step(stmt);
		}
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return maxmem;
}


int update_racct_bhyve(char *vmname, char *orig_jname, char *vmpath)
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
	unsigned long maxmem = 0;
	FILE *fp;

	sprintf(filter,"process:%d:",cur_bid);
	sprintf(unexpanded_rule,"process:%d",cur_bid);

	gettimeofday(&now_time, NULL);
	cur_time = (time_t) now_time.tv_sec;

	for (ch = item_list; ch; ch = ch->next) {
		if (!strcmp(vmname,ch->name)) {
			tolog(log_level,"update metrics for bhyve: [%s, pid: %d]\n",vmname,cur_bid);
			oldpid = ch->pid;
			ch->modified = nanoseconds();
			ch->pid=cur_bid;

			if (oldpid != cur_bid) {
				tolog(log_level,"oldpid(%d) != curpidr(%d) for %s\n",oldpid,cur_bid,vmname);

				// if PID change, get CPUs from bhyve table for ncpu value
				vm_cpus=get_bhyve_cpus(orig_jname);
				if (vm_cpus == 0) return 0;
				maxmem=get_bhyve_maxmem(orig_jname);
				if (maxmem == 0) return 0;
				tolog(log_level,"* VM PID WAS CHANGES, UPDATE CPUS: %d, UPDATE MAXMEM: %lu\n", vm_cpus,maxmem);

				ch->cpus = vm_cpus;
				ch->maxmem = maxmem;
			} else {
				if (ch->cpus == 0) {
					ch->cpus=get_bhyve_cpus(orig_jname);
				}
				vm_cpus = ch->cpus;
				if (ch->maxmem == 0) {
					ch->maxmem=get_bhyve_maxmem(orig_jname);
				}
				maxmem = ch->maxmem;
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
				break;
			while ((var = strsep(&tmp, "=")) != NULL) {
				i++;
				if (var[0] == '\0') {
					free(tmp);
					break;
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
						ch->memoryuse=atol(var);
					} else if (!strcmp(param_name,"memorylocked")) {
						ch->memorylocked=atoi(var);
					} else if (!strcmp(param_name,"maxproc")) {
						ch->maxproc=atoi(var);
					} else if (!strcmp(param_name,"openfiles")) {
						ch->openfiles=atoi(var);
					} else if (!strcmp(param_name,"vmemoryuse")) {
						ch->vmemoryuse=atol(var);
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
					} else {
						//calculate pmem
						ch->pmem = 100.0 * ch->memoryuse / ch->maxmem;
						if (ch->pmem>100)
							ch->pmem=100;
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
	size_t ncpu_len = 0;
	size_t maxmem_len = 0;
	DIR *dirp = NULL;
	int bhyve_exist=1;
	char *yaml;
	int current_jobs_ready=0;
	int jobs_max=2;		// jobs_max per one item, one graph = 25 rec
	int jobs_max_all_items=0;
	int current_waiting=0;
	BSJ *job;
	int x;
	char rnum[5];
	int optcode = 0;
	int option_index = 0;

	struct dirent *dp;
	char vmname[MAXJNAME];
	char vmpath[MAXJNAME];
	char tmpjname[MAXJNAME];
	pid_t vmpid;

	static struct option long_options[] = {
		{"help", no_argument, 0, C_HELP},
		{"log_file", required_argument, 0, C_LOG_FILE},
		{"log_level", required_argument, 0, C_LOG_LEVEL},
		{"loop_interval", required_argument, 0, C_LOOP_INTERVAL},
		{"prometheus_exporter", required_argument, 0, C_PROMETHEUS_EXPORTER},
		{"save_loop_count", required_argument, 0, C_SAVE_LOOP_COUNT},
		{"save_beanstalkd", required_argument, 0, C_SAVE_BEANSTALKD},
		{"save_sqlite3", required_argument, 0, C_SAVE_SQLITE3},
		/* End of options marker */
		{0, 0, 0, 0}
	};

	while (TRUE) {
		optcode = getopt_long_only(argc, argv, "", long_options, &option_index);
		if (optcode == -1)
			break;
		switch(optcode) {
			case C_HELP:
				usage();
				break;
			case C_LOG_FILE:
				log_file=optarg;
				break;
			case C_LOG_LEVEL:
				log_level=atoi(optarg);
				break;
			case C_LOOP_INTERVAL:
				loop_interval=atoi(optarg);
				break;
			case C_PROMETHEUS_EXPORTER:
				prometheus_exporter=atoi(optarg);
				break;
			case C_SAVE_LOOP_COUNT:
				save_loop_count=atoi(optarg);
				break;
			case C_SAVE_BEANSTALKD:
				tobeanstalkd=atoi(optarg);
				break;
			case C_SAVE_SQLITE3:
				tosqlite3=atoi(optarg);
				break;
		}
	}

	chdir("/var/empty");

	printf("CBSD bhyve racct statistics exporter\n");
	if(log_file)
		printf("log_file: %s\n",log_file);
	printf("log_level: %d\n",log_level);
	printf("loop_interval: %d seconds\n",loop_interval);
	printf("save_loop_count: %d\n",save_loop_count);
	printf("beanstalkd enabled: %d\n",tobeanstalkd);
	printf("prometheus enabled: %d\n",prometheus_exporter);
	printf("sqlite3 enabled: %d\n",tosqlite3);

	if((tosqlite3==0)&&(tobeanstalkd==0)&&(prometheus_exporter==0)) {
			printf("Error: select at least one backend ( --prometheus_exported | --save_beanstalkd | --save_sqlite3 )\n");
			exit(-1);
	}

	ncpu_len = sizeof(ncpu);
	maxmem_len = sizeof(maxmem);

	jname = NULL;
	pflags = jflags = jid = 0;

	int pipe_fd[2];
	pid_t otherpid;
	char name[]="racct-bhyve-statsd";

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
			"%sracct-bhyve-statsd.pid", _PATH_VARRUN);
		if (path_my_pidfile == NULL) {
			printf("asprintf");
			exit(1);
			}
	}
	pidfile = pidfile_open(path_my_pidfile, 0644, &otherpid);
	if (pidfile == NULL) {
		if (errno == EEXIST) {
			printf("racct-bhyve-statsd already running, pid: %d.", otherpid);
			exit(1);
			}
		if (errno == EAGAIN) {
			printf("racct-bhyve-statsd already running.");
			exit(1);
			}
		printf("Cannot open or create pidfile: %s",path_my_pidfile);
	}

	if (pidfile != NULL)
		pidfile_write(pidfile);

	i = sysctlbyname("hw.physmem",&maxmem, &maxmem_len, NULL, 0);

	if (i != 0) {
		if (errno == ENOENT)
			errx(1, "Unable to determine hoster physical memory via sysctl hw.physmem");
		err(1, "sysctlbyname");
	}

	i = sysctlbyname("hw.ncpu",&ncpu, &ncpu_len, NULL, 0);

	if (i != 0) {
		if (errno == ENOENT)
			errx(1, "Unable to determine CPU count via hw.ncpu");
		err(1, "sysctlbyname");
	}

	c=0;

	while(1) {
		tolog(log_level,"main loop\n");
		if (bs_socket!=-1)
			bs_disconnect(bs_socket);
		bs_socket=init_bs("racct-bhyve");

		while ( bs_connected==1 ) {
			tolog(log_level," round %d/%d\n ---------------- \n",cur_round,save_loop_count);
			//convert round integer to string
			memset(rnum,0,sizeof(rnum));
			sprintf(rnum,"%d",cur_round);

			dirp = opendir("/dev/vmm");
			if (dirp == NULL) {
				tolog(log_level,"no vmm exist in /dev/vmm, sleep for 60 sec\n");
				sleep(60);
				bhyve_exist=0;
				continue;
				}
			else {
				tolog(log_level,"vmm exist in /dev/vmm\n");
				bhyve_exist=1;
			}

			if (bhyve_exist==1) {
				tolog(log_level,"scan for /dev/vmm\n");
				//rewinddir(dirp);
				while ((dp = readdir(dirp)) != NULL) {
					if (dp->d_name[0]=='.') continue;
					tolog(log_level,"/dev/vmm found: %s\n",dp->d_name);
					memset(vmname,0,sizeof(vmname));
					memset(vmpath,0,sizeof(vmpath));
					sprintf(vmpath,"/dev/vmm/%s",dp->d_name);
					strcpy(vmname,dp->d_name);
					cur_bid=0;
					//cur_bid = get_vm_pid(vmpath);
					cur_bid = get_vm_pid_from_sql(dp->d_name);
					if (cur_bid == 0) continue;

					memset(tmpjname,0,sizeof(tmpjname));
					strcpy(tmpjname,vmname);
					vmname[strlen(vmname)]='\0';
					strcat(vmname,"_");
					strcat(vmname,rnum);
					vmname[strlen(vmname)]='\0';
					i=jname_exist(vmname);
					if (i) {
						update_racct_bhyve(vmname,tmpjname,vmpath);
						continue;
					}

					CREATE(newd, struct item_data, 1);
					newd->pid=cur_bid;
					newd->modified = 0; // sign of new jail
					newd->maxmem = 0;
					newd->pmem = 0;
					newd->next = item_list;
					item_list = newd;
					strcpy(newd->name,vmname);
					strcpy(newd->orig_name,tmpjname);
					tolog(log_level,"[BHYVE] !! %d [%s (%s)] has beed added\n",cur_bid,vmname,tmpjname);
				}
				free(dp);
			}

			if (dirp != NULL)
				(void)closedir(dirp);

			c++;
			list_data();

			if (c>5) {
				prune_inactive_env();
				c=0;
			}

			i=bs_stats_tube(bs_socket, "racct-bhyve", &yaml);
			if(yaml) {
				current_jobs_ready=get_bs_stats(yaml,current_jobs_ready_str);
				current_waiting=get_bs_stats(yaml,current_waiting_str);
				free(yaml);
				if (current_jobs_ready<0) {
					tolog(log_level,"get_bs_stats failed for current-jobs-ready\n");
					bs_connected=0;
					sleep(loop_interval);
					break;
				}
				if (current_waiting<0) {
					tolog(log_level,"get_bs_stats failed for current-waiting\n");
					bs_connected=0;
					sleep(loop_interval);
					break;
				}
				tolog(log_level,"current-jobs: %d, jobs_max_all: %d, current-waiting: %d\n",current_jobs_ready,jobs_max_all_items,current_waiting);
			} else {
				current_waiting=-1;
				current_jobs_ready=-1;
				bs_connected=0;
				tolog(log_level,"bs_stats_tube yaml error,reset bs connection\n");
				sleep(1);
				break;
			}

			if (current_waiting==0) {
					skip_beanstalk=1;
					//no consumer, (flush old data?)
					tolog(log_level,"[debug]no waiting consumer anymore, clear/flush old jobs: %d\n",current_jobs_ready);
//					for (i=0;i<current_jobs_ready;i++) {		//remove
//						bs_reserve_with_timeout(bs_socket, 1, &job);
//						bs_release(bs_socket, job->id, 0, 0);
//						bs_free_job(job);
//						bs_peek_ready(bs_socket, &job);
//						bs_delete(bs_socket, job->id);
//						bs_free_job(job);
//					}
			} else if (current_jobs_ready>20) {
					skip_beanstalk=1;
					tolog(log_level,"[debug]too many ready jobs in bs: %d. skip for beanstalk\n",current_jobs_ready);
			} else {
				skip_beanstalk=0;
			}

			// giant cycle sleep
			tolog(log_level,"\n");
//			usleep(100000);
			sleep(loop_interval);
			cur_round++;
			if(cur_round>save_loop_count) {
				cur_round=0;
			}
			if(cur_round==save_loop_count) {
				sum_data_bhyve();
			}
		}
	}

	if (dirp != NULL)
		(void)closedir(dirp);

	if (pidfile != NULL)
		pidfile_remove(pidfile);
	return 0;
}

int list_data()
{
	struct item_data *target = NULL, *ch, *next_ch;
	int ret=0;

	tolog(log_level,"---listdata---\n");

	for (ch = item_list; ch; ch = ch->next) {
		if (ch->modified==0) continue;
		tolog(log_level,"TIME:%ld,NAME:%s,ORIGNAME:%s,PID:%d,PCPU:%d,MEM:%lu,PROC:%d,OPENFILES:%d,RB:%d,WB:%d,RIO:%d,WIO:%d,PMEM:%d\n",ch->modified,ch->name,ch->orig_name,ch->pid,
		ch->pcpu,ch->memoryuse,ch->maxproc,ch->openfiles, ch->readbps, ch->writebps, ch->readiops, ch->writeiops, ch->pmem);
	}

	return 0;
}
