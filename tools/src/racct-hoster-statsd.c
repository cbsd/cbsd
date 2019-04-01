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
#include <pthread.h>

#include "racct-generic-stats.c"

#define CP_USER   0
#define CP_NICE   1
#define CP_SYS    2
#define CP_INTR   3
#define CP_IDLE   4
#define CPUSTATES 5

int update_racct_hoster(char *, char *);
int sum_data_hoster();
int list_data();
int	get_pmem();

int pcpu;		// gloabal vars for pthread doWork()
int pmem;		// gloabal vars for pthread doWork()
int ncpu;		// number of cores

void *doWork(void *param) {

	long cur[CPUSTATES], last[CPUSTATES];
	size_t cur_sz = sizeof cur;
	int state, i;
	long sum;
	double util;

	memset(last, 0, sizeof last);

	pcpu=0;		//global
	pmem=0;		//global

	// we should never finish monitoring of cpu time
	for (;;)
	{
		if (sysctlbyname("kern.cp_time", &cur, &cur_sz, NULL, 0) < 0)
		{
			tolog(log_level,"Error reading kern.cp_times sysctl\n");
			pthread_exit(NULL);
			exit(-1);
		}

		sum = 0;
		for (state = 0; state<CPUSTATES; state++)
		{
			long tmp = cur[state];
			cur[state] -= last[state];
			last[state] = tmp;
			sum += cur[state];
		}

		util = 100.0L - (100.0L * cur[CP_IDLE] / (sum ? (double) sum : 1.0L));
		pcpu=(int)util;
		pmem=get_pmem();
		sleep(1);
	}

	pthread_exit(NULL);
}

int sum_data_hoster()
{
	struct item_data *target = NULL, *ch, *next_ch;
	char sql[512];
	char stats_file[1024];
	int ret=0;
	FILE *fp;
	char json_str[20000];		// todo: dynamic from number of hoster/jails
	char json_buf[1024];		// todo: dynamic from number of hoster/jails
	int i;
	struct timeval  now_time;
	int cur_time = 0;
	int round_total=save_loop_count+1;

	struct sum_item_data *newd;
	struct sum_item_data *temp;
	struct sum_item_data *sumch, *next_sumch;

	tolog(log_level,"\n ***---calc hoster avgdata---*** \n");

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
		tolog(log_level," ***[%s]SUM|PCPU:%d,MEM:%ld,TIME:%ld\n",sumch->name,
		sumch->pcpu/round_total,
		sumch->memoryuse/round_total,
		sumch->modified/round_total);
		memset(json_buf,0,sizeof(json_buf));
		sprintf(json_buf,"{\"name\": \"%s\",\"time\": %d,\"pcpu\": %d,\"pmem\": %d }",sumch->name,
		cur_time,sumch->pcpu/round_total,sumch->pmem/round_total);

		if (strlen(json_str)>2) {
			strcat(json_str,",");
			strcat(json_str,json_buf);
		} else {
			strcpy(json_str,"{ \"tube\":\"racct-system\", \"node\":\"electrode.alicorn.lv\", \"data\":[");
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


int update_racct_hoster(char *vmname, char *orig_jname)
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

	gettimeofday(&now_time, NULL);
	cur_time = (time_t) now_time.tv_sec;

	for (ch = item_list; ch; ch = ch->next) {
		if (!strcmp(vmname,ch->name)) {
			tolog(log_level,"update metrics for hoster: [%s]\n",vmname);
			ch->modified = nanoseconds();
			// obtain from pthread (doWork) global vars
			ch->pcpu = pcpu;
			ch->pmem = pmem;
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
	int hoster_exist=1;
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

	printf("CBSD hoster racct statistics exporter\n");
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

	int total = 1;
	int numThreads=1;

	int numWork = total / numThreads;
	int thread;

	ncpu_len = sizeof(ncpu);
	maxmem_len = sizeof(maxmem);

	jname = NULL;
	pflags = jflags = jid = 0;

	int pipe_fd[2];
	pid_t otherpid;
	char name[]="racct-hoster-statsd";

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
			"%sracct-hoster-statsd.pid", _PATH_VARRUN);
		if (path_my_pidfile == NULL) {
			printf("asprintf");
			exit(1);
			}
	}
	pidfile = pidfile_open(path_my_pidfile, 0644, &otherpid);
	if (pidfile == NULL) {
		if (errno == EEXIST) {
			printf("racct-hoster-statsd already running, pid: %d.", otherpid);
			exit(1);
			}
		if (errno == EAGAIN) {
			printf("racct-hoster-statsd already running.");
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
	strcpy(tmpjname,"local");

	pthread_t threads[numThreads];
	pthread_create(&threads[0], NULL, doWork, &numWork);

	while(1) {
        tolog(log_level,"main loop\n");
		if (bs_socket!=-1)
			bs_disconnect(bs_socket);
		bs_socket=init_bs("racct-system");

		while ( bs_connected==1 ) {

//			if (pthread_join(threads[thread], NULL)) {
//				printf("Error waiting for thread %i of %i\n", thread, numThreads);
//			}
            tolog(log_level," round %d/%d\n ---------------- \n",cur_round,save_loop_count);
			//convert round integer to string
			memset(rnum,0,sizeof(rnum));
			sprintf(rnum,"%d",cur_round);

			memset(vmname,0,sizeof(vmname));
			strcpy(vmname,tmpjname);
			strcat(vmname,"_");
			strcat(vmname,rnum);
			vmname[strlen(vmname)]='\0';
			i=jname_exist(vmname);

			if (i) {
				update_racct_hoster(vmname,tmpjname);
			} else {
				CREATE(newd, struct item_data, 1);
				newd->modified = 0; // sign of new jail
				newd->maxmem = 0;
				newd->pmem = 0;
				newd->next = item_list;
				item_list = newd;
				strcpy(newd->name,vmname);
				strcpy(newd->orig_name,tmpjname);
				tolog(log_level,"[hoster] !! %s has beed added (%s)\n",newd->name,newd->orig_name);
			}

			c++;
			list_data();

			if (c>5) {
				prune_inactive_env();
				c=0;
			}

			i=bs_stats_tube(bs_socket, "racct-system", &yaml);
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
				sum_data_hoster();
			}
		}
	}

	if (pidfile != NULL)
		pidfile_remove(pidfile);

	pthread_exit(NULL);

	return 0;
}



int get_pmem()
{
	unsigned long realmem = 0 ;
	unsigned long page_size = 0 ;
	unsigned long active_count = 0 ;
	unsigned long wire_count = 0 ;
	unsigned long active_size = 0 ;
	unsigned long wire_size = 0 ;
	unsigned long freemem = 0 ;
	unsigned long mem_use = 0 ;
	size_t realmem_len = 0;
	size_t page_size_len = 0;
	size_t active_count_len = 0;
	size_t wire_count_len = 0;
	size_t active_count_size_len = 0;
	size_t wire_size_len = 0;
	int i;
	int errno;
	int calc_pmem;

	realmem_len = sizeof(realmem);
	i = sysctlbyname("hw.realmem",&realmem, &realmem_len, NULL, 0);

	if (i != 0) {
		if (errno == ENOENT)
			errx(1, "Unable to determine hoster physical memory via sysctl hw.realmem");
	}

	page_size_len = sizeof(page_size);
	i = sysctlbyname("vm.stats.vm.v_page_size",&page_size, &page_size_len, NULL, 0);

	if (i != 0) {
		if (errno == ENOENT)
			errx(1, "Unable to determine hoster page_size via sysctl vm.stats.vm.v_page_size");
	}

	active_count_len = sizeof(active_count);
	i = sysctlbyname("vm.stats.vm.v_active_count",&active_count, &active_count_len, NULL, 0);

	if (i != 0) {
		if (errno == ENOENT)
			errx(1, "Unable to determine hoster active_count via sysctl vm.stats.vm.v_active_count");
	}

	wire_count_len = sizeof(wire_count);
	i = sysctlbyname("vm.stats.vm.v_wire_count",&wire_count, &wire_count_len, NULL, 0);

	if (i != 0) {
		if (errno == ENOENT)
		errx(1, "Unable to determine hoster physical memory via sysctl vm.stats.vm.v_wire_count");
	}

	active_size=page_size*active_count;
	wire_size=page_size*wire_count;
	freemem=realmem-active_size-wire_size;
	mem_use=realmem - freemem;

	//calculate calc_pmem
	calc_pmem = 100.0 * mem_use / realmem;
	if (calc_pmem>100)
		calc_pmem=100;

	//printf("calc_pmem: %d\n",calc_pmem);
	return calc_pmem;
}

int list_data()
{
	struct item_data *target = NULL, *ch, *next_ch;
	int ret=0;

	if(log_level==0)
		return 0;

	tolog(log_level,"---listdata---\n");

	for (ch = item_list; ch; ch = ch->next) {
		if (ch->modified==0) continue;
		tolog(log_level,"TIME:%ld,NAME:%s,ORIGNAME:%s,PCPU:%d,PMEM:%d\n",ch->modified,ch->name,ch->orig_name,ch->pcpu,ch->pmem);
	}

	return 0;
}
