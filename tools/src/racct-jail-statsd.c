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

#include "racct-generic-stats.c"

#define	JP_USER		0x01000000
#define	JP_OPT		0x02000000

#define	PRINT_DEFAULT	0x01
#define	PRINT_HEADER	0x02
#define	PRINT_NAMEVAL	0x04
#define	PRINT_QUOTED	0x08
#define	PRINT_SKIP	0x10
#define	PRINT_VERBOSE	0x20
#define	PRINT_JAIL_NAME	0x40

static struct jailparam *params;
static int *param_parent;
static int nparams;

static int add_param(const char *name, void *value, size_t valuelen,
		struct jailparam *source, unsigned flags);
static int sort_param(const void *a, const void *b);
static int print_jail(int pflags, int jflags);

int list_data();

int update_racct_jail(char *, char *, int);

int sum_data()
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

	tolog(log_level,"\n ***---calc jail avgdata---*** \n");

	gettimeofday(&now_time, NULL);
	cur_time = (time_t) now_time.tv_sec;

	for (ch = item_list; ch; ch = ch->next) {
		if(strlen(ch->orig_name)<1) continue;
		if (ch->modified==0) continue;
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
		sprintf(json_buf,"{\"name\": \"%s\", \"time\": %d, \"pcpu\": %d, \"pmem\": %d,\"maxproc\": %d,\"openfiles\": %d,\"readbps\": %d,\"writebps\": %d,\"readiops\": %d,\"writeiops\": %d }",sumch->name,
		cur_time,sumch->pcpu/round_total,sumch->pmem/round_total,sumch->maxproc/round_total,sumch->openfiles/round_total,sumch->readbps/round_total,sumch->writebps/round_total,sumch->readiops/round_total,sumch->writeiops/round_total);

		if (strlen(json_str)>2) {
			strcat(json_str,",");
			strcat(json_str,json_buf);
		} else {
			strcpy(json_str,"{ \"tube\":\"racct-jail\", \"data\":[");
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


int update_racct_jail(char *jname, char *orig_jname, int jid)
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

	sprintf(filter,"jail:%s:",orig_jname);
	sprintf(unexpanded_rule,"jail:%s",orig_jname);

	gettimeofday(&now_time, NULL);
	cur_time = (time_t) now_time.tv_sec;

	for (ch = item_list; ch; ch = ch->next) {
		if (!strcmp(jname,ch->name)) {
			tolog(log_level,"update metrics for jail: [%s]\n",jname);
			//ch->modified = (time_t) now_time.tv_sec;
			ch->modified = nanoseconds();
			ch->pid=cur_jid;

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
						if (ncpu>1) {
							ch->pcpu=( atoi(var) / ncpu );
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
						ch->pmem = 100.0 * ch->memoryuse / maxmem;
						if (ch->pmem>100)
							ch->pmem=100;
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
	int jail_exist=1;
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

	printf("CBSD jail racct statistics exporter\n");
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
	char name[]="racct-jail-statsd";

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
			"%sracct-jail-statsd.pid", _PATH_VARRUN);
		if (path_my_pidfile == NULL) {
			printf("asprintf");
			exit(1);
			}
	}
	pidfile = pidfile_open(path_my_pidfile, 0644, &otherpid);
	if (pidfile == NULL) {
		if (errno == EEXIST) {
			printf("racct-jail-statsd already running, pid: %d.", otherpid);
			exit(1);
			}
		if (errno == EAGAIN) {
			printf("racct-jail-statsd already running.");
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

	/* Add the parameters to print. */
	add_param("jid", NULL, (size_t)0, NULL, JP_USER);
	add_param("name", NULL, (size_t)0, NULL, JP_USER);
	add_param("lastjid", &lastjid, sizeof(lastjid), NULL, 0);

	c=0;

	while(1) {
		tolog(log_level,"main loop\n");
		if (bs_socket!=-1)
			bs_disconnect(bs_socket);
		bs_socket=init_bs("racct-jail");

		while ( bs_connected==1 ) {
			tolog(log_level," round %d/%d\n ---------------- \n",cur_round,save_loop_count);
			//convert round integer to string
			memset(rnum,0,sizeof(rnum));
			sprintf(rnum,"%d",cur_round);
			//jail area
			if (jail_exist==1) {
				for (lastjid=0; lastjid>=0; ) {
					memset(cur_jname,0,sizeof(cur_jname));
					lastjid = print_jail(pflags, jflags);
					if (cur_jid == 0) continue;
					if (strlen(cur_jname)<1) continue;

					memset(tmpjname,0,sizeof(tmpjname));
					strcpy(tmpjname,cur_jname);
					cur_jname[strlen(cur_jname)]='\0';
					strcat(cur_jname,"_");
					strcat(cur_jname,rnum);
					cur_jname[strlen(cur_jname)]='\0';

					i=jname_exist(cur_jname);

					if (i) {
						update_racct_jail(cur_jname,tmpjname,cur_jid);
						continue;
					}
					CREATE(newd, struct item_data, 1);
					newd->cputime=lastjid;
					newd->pid=cur_jid;
					newd->modified = 0; // sign of new jail
					newd->next = item_list;
					item_list = newd;
					strcpy(newd->name,cur_jname);
					strcpy(newd->orig_name,tmpjname);
					tolog(log_level,"[JAIL] !! %d [%s (%s)] has beed added\n",cur_jid,cur_jname,tmpjname);
				}
			}

			c++;
			list_data();

			if (c>5) {
				prune_inactive_env();
				c=0;
			}

			i=bs_stats_tube(bs_socket, "racct-jail", &yaml);
			if(yaml) {
				current_jobs_ready=get_bs_stats(yaml,current_jobs_ready_str);
				current_waiting=get_bs_stats(yaml,current_waiting_str);
//				current_jobs_ready=get_bs_stats(yaml,"current-jobs-ready: ");
//				current_waiting=get_bs_stats(yaml,"current-waiting: ");
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
				sum_data();
			}
		}
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

int list_data()
{
	struct item_data *target = NULL, *ch, *next_ch;
	int ret=0;

	if(log_level==0)
		return 0;

	tolog(log_level,"---listdata---\n");

	for (ch = item_list; ch; ch = ch->next) {
		if (ch->modified==0) continue;
		tolog(log_level,"TIME:%ld,NAME:%s,ORIGNAME:%s,PID:%d,PCPU:%d,MEM:%lu,PROC:%d,OPENFILES:%d,RB:%d,WB:%d,RIO:%d,WIO:%d,PMEM:%d\n",ch->modified,ch->name,ch->orig_name,ch->pid,
		ch->pcpu,ch->memoryuse,ch->maxproc,ch->openfiles, ch->readbps, ch->writebps, ch->readiops, ch->writeiops, ch->pmem);
	}

	return 0;
}
