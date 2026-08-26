// CBSD Project 2012-2025
#include <sys/param.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>

#include <paths.h>

#include <assert.h>
#include <inttypes.h>

#include "beanstalk.h"

#include <pthread.h>
#include "sqlite3.h"

#include "racct-generic-stats.c"

unsigned int running_bhyves;
static int prometheus_listen_fd = -1;
static pthread_mutex_t prometheus_mutex = PTHREAD_MUTEX_INITIALIZER;
static char *prometheus_snapshot;

struct prometheus_client {
	int fd;
};

static int
write_all(int fd, const char *buffer, size_t length)
{
	ssize_t written;
	while (length > 0) {
		written = write(fd, buffer, length);
		if (written < 0 && errno == EINTR)
			continue;
		if (written <= 0)
			return -1;
		buffer += written;
		length -= (size_t)written;
	}
	return 0;
}

static void
escape_prometheus_label(const char *source, char *destination, size_t size)
{
	size_t used = 0;
	while (*source != '\0' && used + 1 < size) {
		if ((*source == '\\' || *source == '"' || *source == '\n') &&
		    used + 2 < size) {
			destination[used++] = '\\';
			destination[used++] = *source == '\n' ? 'n' : *source;
		} else {
			destination[used++] = *source;
		}
		source++;
	}
	destination[used] = '\0';
}

static void *
handle_prometheus_client(void *argument)
{
	struct prometheus_client *client = argument;
	char header[256], *body;
	size_t length;

	pthread_mutex_lock(&prometheus_mutex);
	body = strdup(prometheus_snapshot != NULL ? prometheus_snapshot :
	    "bhyves_up 0\n");
	pthread_mutex_unlock(&prometheus_mutex);
	if (body != NULL) {
		length = strlen(body);
		snprintf(header, sizeof(header),
		    "HTTP/1.1 200 OK\r\nConnection: close\r\n"
		    "Content-Type: text/plain; version=0.0.4\r\n"
		    "Content-Length: %zu\r\n\r\n", length);
		if (write_all(client->fd, header, strlen(header)) == 0)
			(void)write_all(client->fd, body, length);
		free(body);
	}
	close(client->fd);
	free(client);
	return NULL;
}

static void *
accept_prometheus_clients(void *unused)
{
	struct prometheus_client *client;
	pthread_t thread;
	pthread_attr_t attributes;
	int fd;

	(void)unused;
	pthread_attr_init(&attributes);
	pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
	for (;;) {
		fd = accept(prometheus_listen_fd, NULL, NULL);
		if (fd < 0) {
			if (errno != EINTR)
				perror("Prometheus accept failed");
			continue;
		}
		client = malloc(sizeof(*client));
		if (client == NULL) {
			close(fd);
			continue;
		}
		client->fd = fd;
		if (pthread_create(&thread, &attributes, handle_prometheus_client,
		    client) != 0) {
			close(fd);
			free(client);
			continue;
		}
	}
}

static int
start_prometheus_exporter(void)
{
	struct sockaddr_in address;
	pthread_t thread;
	pthread_attr_t attributes;
	int option = 1;
	const char *ip = prometheus_listen4 != NULL ? prometheus_listen4 :
	    "127.0.0.1";

	prometheus_listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (prometheus_listen_fd < 0)
		return -1;
	(void)setsockopt(prometheus_listen_fd, SOL_SOCKET, SO_REUSEADDR,
	    &option, sizeof(option));
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(prometheus_port);
	if (inet_pton(AF_INET, ip, &address.sin_addr) != 1 ||
	    bind(prometheus_listen_fd, (struct sockaddr *)&address,
	    sizeof(address)) < 0 || listen(prometheus_listen_fd, 16) < 0) {
		close(prometheus_listen_fd);
		prometheus_listen_fd = -1;
		return -1;
	}
	pthread_attr_init(&attributes);
	pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&thread, &attributes, accept_prometheus_clients,
	    NULL) != 0) {
		pthread_attr_destroy(&attributes);
		close(prometheus_listen_fd);
		prometheus_listen_fd = -1;
		return -1;
	}
	pthread_attr_destroy(&attributes);
	return 0;
}
int update_racct_bhyve(char * /*vmname*/, char * /*orig_jname*/,
    char * /*vmpath*/);
int sum_data_bhyve();
int list_data();

static void
remove_bhyve_samples(const char *name)
{
	struct item_data *ch = item_list, *previous = NULL, *next;

	while (ch != NULL) {
		next = ch->next;
		if (strcmp(ch->orig_name, name) == 0) {
			if (previous == NULL)
				item_list = next;
			else
				previous->next = next;
			free(ch);
		} else {
			previous = ch;
		}
		ch = next;
	}
}

pid_t
dofiles(struct procstat *procstat, struct kinfo_proc *kp)
{
	const char *cmd;
	// const char *uname;
	// at the moment, only root user can run bhyve
	const char *uname = "root";
	struct filestat *fst = NULL;
	struct filestat_list *head;
	pid_t pid = 0;
	pid_t vm_pid = 0;
	// at the moment, only root user can run bhyve
	//  uname = user_from_uid(kp->ki_uid, 0);
	pid = kp->ki_pid;
	cmd = kp->ki_comm;

	head = procstat_getfiles(procstat, kp, mflg);

	if (head == NULL) {
		return -1;
	}

	STAILQ_FOREACH (fst, head, next) {
		// processing only for bhyve command
		if (!strcmp(cmd, "bhyve")) {
			vm_pid = print_file_info(procstat, fst, uname, cmd,
			    pid);
			if (vm_pid > 0) {
				break;
			}
		}
		if (vm_pid > 0) {
			break;
		}
	}
	procstat_freefiles(procstat, head);
	return vm_pid;
}

pid_t
print_file_info(struct procstat *procstat, struct filestat *fst,
    const char *uname, const char *cmd, int pid)
{
	struct vnstat vn;
	int error;
	int fsmatch = 0;

	if (checkfile == 0) {
		return 0;
	}
	if (fst->fs_type != PS_FST_TYPE_VNODE &&
	    fst->fs_type != PS_FST_TYPE_FIFO) {
		return -1;
	}

	error = procstat_get_vnode_info(procstat, fst, &vn, NULL);
	if (error != 0) {
		return -1;
	}

	if (fsid == vn.vn_fsid) {
		if (ino == vn.vn_fileid) {
			// no memleak?, struct from
			// /src/lib/libprocstat/libprocstat.h
			free(vn.vn_mntdir);
			return pid;
		}
	}

	// no? memleak, struct from /src/lib/libprocstat/libprocstat.h
	free(vn.vn_mntdir);
	return 0;
}

// store filename data (inode, fsid)
int
getfname(char *filename)
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

	if ((ino == 0) || (fsid == 0)) {
		return 0;
	}
	return 1;
}

pid_t
get_vm_pid(char *vmpath)
{
	struct kinfo_proc *p;
	struct procstat *procstat = NULL;
	int arg;
	int ch;
	int what;
	int i;
	unsigned int cnt;
	pid_t vmpid = 0;

	arg = 0;
	what = KERN_PROC_PROC;
	nlistf = memf = NULL;

	if (getfname(vmpath)) {
		checkfile = 1;
	}

	if (!checkfile) { /* file(s) specified, but none accessible */
		return -1;
	}

	procstat = procstat_open_sysctl();
	if (procstat == NULL) {
		errx(1, "procstat_open()");
	}

	p = procstat_getprocs(procstat, what, arg, &cnt);
	if (p == NULL) {
		errx(1, "procstat_getprocs()");
	}

	/*
	 * Go through the process list.
	 */
	for (i = 0; i < cnt; i++) {
		if (p[i].ki_stat == SZOMB) {
			continue;
		}
		vmpid = dofiles(procstat, &p[i]);
		if (vmpid > 0) {
			break;
		}
	}
	procstat_freeprocs(procstat, p);
	procstat_close(procstat);
	return vmpid;
}

int
sum_data_bhyve()
{
	struct item_data *target = NULL;
	struct item_data *ch;
	struct item_data *next_ch;
	const char *hostname = getenv("HOST");
	const char *cix_distdir_env = getenv("CIX_DISTDIR");
	const char *cix_bin_env = getenv("CIX_BIN");
	const char *cix_distdir;
	const char *cix_bin;
	char sql[512];
	char stats_file[1024];
	int ret = 0;
	FILE *fp;
	char *json_str = NULL;
	size_t json_capacity = 64;
	char *prometheus = NULL;
	size_t prometheus_length = 0, prometheus_capacity = 64;
	int prometheus_valid = 1;
	char escaped_name[MAXJNAME * 2];
	char json_buf[1024];  // todo: dynamic from number of bhyve/jails
	int i;
	struct timeval now_time;
	int cur_time = 0;

	struct sum_item_data *newd;
	struct sum_item_data *temp;
	struct sum_item_data *sumch;
	struct sum_item_data *next_sumch;

	// Set default values if environment variables are not set
	if (cix_distdir_env == NULL) {
		cix_distdir = "/usr/local/cbsd";
	} else {
		cix_distdir = cix_distdir_env;
	}
	if (cix_bin_env == NULL) {
		cix_bin = "/usr/local/bin/cbsd";
	} else {
		cix_bin = cix_bin_env;
	}

	tolog(log_level, "\n ***---calc bhyve avgdata---*** \n");

	gettimeofday(&now_time, NULL);
	cur_time = (time_t)now_time.tv_sec;

	// First, free existing sum_item_list
	for (sumch = sum_item_list; sumch; sumch = next_sumch) {
		next_sumch = sumch->next;
		free(sumch);
	}
	sum_item_list = NULL;

	for (ch = item_list; ch; ch = ch->next) {
		if (ch->modified == 0) {
			continue;
		}
		if (strlen(ch->orig_name) < 1) {
			continue;
		}
		i = sum_jname_exist(ch->orig_name);

		if (i) {
			for (sumch = sum_item_list; sumch;
			     sumch = sumch->next) {
				if (!strcmp(ch->orig_name, sumch->name)) {
					sumch->pid++;
					sumch->modified += ch->modified;
					sumch->pcpu += ch->pcpu;
					sumch->memoryuse += ch->memoryuse;
					sumch->maxproc += ch->maxproc;
					sumch->openfiles += ch->openfiles;
					sumch->readbps += ch->readbps;
					sumch->writebps += ch->writebps;
					sumch->readiops += ch->readiops;
					sumch->writeiops += ch->writeiops;
					sumch->pmem += ch->pmem;
					break;
				}
			}
		} else {
			CREATE(newd, struct sum_item_data, 1);
			if (!newd) {
				tolog(log_level, "Failed to allocate memory for newd\n");
				continue;
			}
			newd->modified = ch->modified;
			newd->pid = 1; /* number of samples in this aggregate */
			newd->pcpu = ch->pcpu;
			newd->memoryuse = ch->memoryuse;
			newd->maxproc = ch->maxproc;
			newd->openfiles = ch->openfiles;
			newd->readbps = ch->readbps;
			newd->writebps = ch->writebps;
			newd->readiops = ch->readiops;
			newd->writeiops = ch->writeiops;
			newd->pmem = ch->pmem;
			newd->next = sum_item_list;
			sum_item_list = newd;
			strncpy(newd->name, ch->orig_name, sizeof(newd->name) - 1);
			newd->name[sizeof(newd->name) - 1] = '\0';
			tolog(log_level,
			    "[AVGSUM] !! %s struct has beed added\n",
			    newd->name);
		}
	}

	for (sumch = sum_item_list; sumch; sumch = sumch->next) {
		json_capacity += sizeof(json_buf) + 1;
		prometheus_capacity += 2048;
	}
	json_str = calloc(1, json_capacity);
	if (json_str == NULL)
		return 1;
	if (OUTPUT_PROMETHEUS & output_flags) {
		prometheus = calloc(1, prometheus_capacity);
		if (prometheus == NULL) {
			free(json_str);
			return 1;
		}
		prometheus_length = (size_t)snprintf(prometheus,
		    prometheus_capacity, "bhyves_up %u\n", running_bhyves);
	}
	for (sumch = sum_item_list; sumch; sumch = sumch->next) {
		if (strlen(sumch->name) < 1) {
			continue;
		}
		tolog(log_level,
		    " ***[%s]SUM|PCPU:%d,MEM:%ld,PROC:%d,OPENFILES:%d,RBPS:%d,WBPS:%d,RIOPS:%d,WIOPS:%d,PMEM:%d,TIME:%ld\n",
		    sumch->name, sumch->pcpu / sumch->pid,
		    sumch->memoryuse / sumch->pid,
		    sumch->maxproc / sumch->pid,
		    sumch->openfiles / sumch->pid,
		    sumch->readbps / sumch->pid, sumch->writebps / sumch->pid,
		    sumch->readiops / sumch->pid,
		    sumch->writeiops / sumch->pid, sumch->pmem / sumch->pid,
		    sumch->modified / sumch->pid);

		if (prometheus != NULL && prometheus_length < prometheus_capacity) {
			escape_prometheus_label(sumch->name, escaped_name,
			    sizeof(escaped_name));
			int added = snprintf(prometheus + prometheus_length,
			    prometheus_capacity - prometheus_length,
			    "bhyve_openfiles{name=\"%s\"} %u\n"
			    "bhyve_memoryuse{name=\"%s\"} %lu\n"
			    "bhyve_maxproc{name=\"%s\"} %u\n"
			    "bhyve_readbps{name=\"%s\"} %u\n"
			    "bhyve_writebps{name=\"%s\"} %u\n"
			    "bhyve_readiops{name=\"%s\"} %u\n"
			    "bhyve_writeiops{name=\"%s\"} %u\n"
			    "bhyve_pcpu{name=\"%s\"} %u\n",
			    escaped_name, sumch->openfiles / sumch->pid,
			    escaped_name, sumch->memoryuse / sumch->pid,
			    escaped_name, sumch->maxproc / sumch->pid,
			    escaped_name, sumch->readbps / sumch->pid,
			    escaped_name, sumch->writebps / sumch->pid,
			    escaped_name, sumch->readiops / sumch->pid,
			    escaped_name, sumch->writeiops / sumch->pid,
			    escaped_name, sumch->pcpu / sumch->pid);
			if (added < 0 || (size_t)added >=
			    prometheus_capacity - prometheus_length) {
				prometheus_valid = 0;
			} else {
				prometheus_length += (size_t)added;
			}
		}

		if (OUTPUT_BEANSTALKD & output_flags) {
			int json_length;

			memset(json_buf, 0, sizeof(json_buf));
			json_length = snprintf(json_buf, sizeof(json_buf),
			    "{\"name\": \"%s\",\"time\": %d,\"pcpu\": %d,\"pmem\": %d,\"readbps\": %d,\"writebps\": %d,\"readiops\": %d,\"writeiops\": %d }",
			    sumch->name, cur_time, sumch->pcpu / sumch->pid,
			    sumch->pmem / sumch->pid,
			    sumch->readbps / sumch->pid,
			    sumch->writebps / sumch->pid,
			    sumch->readiops / sumch->pid,
			    sumch->writeiops / sumch->pid);
			if (json_length < 0 ||
			    (size_t)json_length >= sizeof(json_buf)) {
				tolog(log_level, "Beanstalk metric is too long for %s\n",
				    sumch->name);
				json_buf[0] = '\0';
			}

			if (json_buf[0] == '\0') {
				/* Other backends still need this metric. */
			} else if (strlen(json_str) > 2) {
				strncat(json_str, ",",
				    json_capacity - strlen(json_str) - 1);
				strncat(json_str, json_buf,
				    json_capacity - strlen(json_str) - 1);
			} else {
				strncpy(json_str, "{ \"tube\":\"racct-bhyve\", \"data\":[",
				    json_capacity - 1);
				strncat(json_str, json_buf,
				    json_capacity - strlen(json_str) - 1);
			}
		}

#ifdef WITH_INFLUX
		if (OUTPUT_INFLUX & output_flags) {
			char influx_line[2048];
			int influx_length;

			influx_length = snprintf(influx_line, sizeof(influx_line),
			    "%s,node=%s,host=%s%s%s memoryuse=%lu,pcpu=%d,pmem=%d,readbps=%d,writebps=%d,readiops=%d,writeiops=%d,maxproc=%d,openfiles=%d %lu\n",
			    influx->tables.bhyve, hostname, sumch->name,
			    (influx->tags.bhyve == NULL ? "" : ","),
			    (influx->tags.bhyve == NULL ? "" :
								influx->tags.bhyve),
			    sumch->memoryuse / sumch->pid,
			    sumch->pcpu / sumch->pid,
			    sumch->pmem / sumch->pid,
			    sumch->readbps / sumch->pid,
			    sumch->writebps / sumch->pid,
			    sumch->readiops / sumch->pid,
			    sumch->writeiops / sumch->pid,
			    sumch->maxproc / sumch->pid,
			    sumch->openfiles / sumch->pid, nanoseconds());
			if (influx_length < 0 ||
			    (size_t)influx_length >= sizeof(influx_line)) {
				tolog(log_level, "Influx metric is too long for %s\n",
				    sumch->name);
			} else {
				if (strlen(influx->buffer) + (size_t)influx_length >=
				    sizeof(influx->buffer)) {
					if (cbsd_influx_transmit_buffer() != 0) {
						tolog(log_level,
						    "RACCT to Influx failed\n");
					} else {
						strncat(influx->buffer, influx_line,
						    sizeof(influx->buffer) -
						    strlen(influx->buffer) - 1);
						influx->items++;
					}
				} else {
					strncat(influx->buffer, influx_line,
					    sizeof(influx->buffer) -
					    strlen(influx->buffer) - 1);
					influx->items++;
				}
			}
		}
#endif

		if (OUTPUT_SQLITE3 & output_flags) {
			memset(sql, 0, sizeof(sql));
			memset(stats_file, 0, sizeof(stats_file));
			snprintf(stats_file, sizeof(stats_file), "%s/jails-system/%s/racct.sqlite",
			    workdir, sumch->name);
			fp = fopen(stats_file, "r");
			if (!fp) {
				tolog(log_level,
				    "RACCT not exist, create via updatesql\n");
			// Use execv instead of system() to prevent command injection
			// Escape the stats_file path to prevent injection
			char escaped_stats_file[1024];
			snprintf(escaped_stats_file, sizeof(escaped_stats_file), "%s", stats_file);
			
			// Simple validation - reject if stats_file contains dangerous characters
			if (strstr(stats_file, ";") || strstr(stats_file, "|") || strstr(stats_file, "&") || 
			    strstr(stats_file, "$") || strstr(stats_file, "`") || strstr(stats_file, "(") ||
			    strstr(stats_file, ")") || strstr(stats_file, "<") || strstr(stats_file, ">")) {
				tolog(log_level, "Dangerous characters detected in stats_file path, skipping\n");
				goto finish_metric;
			}
			
			snprintf(sql, sizeof(sql),
			    "%s %s/misc/updatesql %s %s/share/racct.schema racct",
			    cix_bin, cix_distdir, escaped_stats_file, cix_distdir);
			system(sql);
				goto finish_metric;
			}
			fclose(fp);

			snprintf(sql, sizeof(sql),
			    "INSERT INTO racct ( idx,memoryuse,maxproc,openfiles,pcpu,readbps,writebps,readiops,writeiops,pmem ) VALUES ( '%d', '%lu', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d' );\n",
			    cur_time, sumch->memoryuse / sumch->pid,
			    sumch->maxproc / sumch->pid,
			    sumch->openfiles / sumch->pid,
			    sumch->pcpu / sumch->pid,
			    sumch->readbps / sumch->pid,
			    sumch->writebps / sumch->pid,
			    sumch->readiops / sumch->pid,
			    sumch->writeiops / sumch->pid,
			    sumch->pmem / sumch->pid);
			tolog(log_level, "Save to SQL: %s [%s]\n", stats_file,
			    sql);
			ret = sqlitecmd(stats_file, sql);
		}
	finish_metric:
		sumch->modified = 0;
		sumch->pcpu = 0;
		sumch->memoryuse = 0;
		sumch->maxproc = 0;
		sumch->openfiles = 0;
		sumch->readbps = 0;
		sumch->writebps = 0;
		sumch->readiops = 0;
		sumch->writeiops = 0;
		sumch->pmem = 0;

		remove_bhyve_samples(sumch->name);
	}

	if (OUTPUT_BEANSTALKD & output_flags) {
		if (strlen(json_str) + 2 < json_capacity) {
			strncat(json_str, "]}",
			    json_capacity - strlen(json_str) - 1);
		} else {
			tolog(log_level, "Buffer overflow in json_str\n");
			skip_beanstalk = 1;
		}
	} else {
		skip_beanstalk = 1;
	}
	bs_tick = 0;
	if (prometheus != NULL && prometheus_valid) {
		pthread_mutex_lock(&prometheus_mutex);
		free(prometheus_snapshot);
		prometheus_snapshot = prometheus;
		pthread_mutex_unlock(&prometheus_mutex);
		prometheus = NULL;
	}

	if (cur_round != save_loop_count || skip_beanstalk == 1) {
		free(json_str);
		free(prometheus);
		return (0);
	}

	if (strlen(json_str) > 3) {
		tolog(log_level, "bs_put: (%s)\n", json_str);
		ret = bs_put(bs_socket, 0, 0, 0, json_str, strlen(json_str));
		if (ret > 0) {
			bs_tick = 1;
		} else {
			tolog(log_level,
			    "bs_put failed, trying to reconnect...\n");
			bs_disconnect(bs_socket);
			bs_connected = 0;
			free(json_str);
			free(prometheus);
			return 1;
		}
	} else {
		tolog(log_level, "skip_beanstalk = 1,skipp\n");
	}
	free(json_str);
	free(prometheus);
	return 0;
}

int
get_bhyve_cpus(char *vmname)
{
	sqlite3 *db = NULL;
	char query[40];
	char *err = NULL;
	int maxretry = 10;
	int retry = 0;
	sqlite3_stmt *stmt = NULL;
	int ret;
	char dbfile[512];
	int vm_cpus = 0;

	// Validate vmname to prevent path traversal
	if (strlen(vmname) > 50 || strstr(vmname, "..") || strstr(vmname, "/") || 
	    strstr(vmname, "\\") || strstr(vmname, "\0")) {
		tolog(log_level, "Invalid vmname detected in get_bhyve_cpus\n");
		return 1;
	}

	memset(dbfile, 0, sizeof(dbfile));
	snprintf(dbfile, sizeof(dbfile), "%s/jails-system/%s/local.sqlite", workdir, vmname);

	if (SQLITE_OK != (ret = sqlite3_open(dbfile, &db))) {
		tolog(log_level, "%s: Can't open database file: %s\n", nm(),
		    dbfile);
		sqlite3_close(db);
		return 1;
	}

	snprintf(query, sizeof(query), "SELECT vm_cpus FROM settings LIMIT 1");
	ret = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);

	if (ret == SQLITE_OK) {
		ret = sqlite3_step(stmt);

		while (ret == SQLITE_ROW) {
			vm_cpus = sql_get_int(stmt);
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
	sqlite3 *db = NULL;
	char query[100];
	char *err = NULL;
	int maxretry = 10;
	int retry = 0;
	sqlite3_stmt *stmt = NULL;
	int ret;
	char dbfile[512];
	unsigned long jid = 0;

	memset(dbfile, 0, sizeof(dbfile));
	snprintf(dbfile, sizeof(dbfile), "%s/var/db/local.sqlite", workdir);

	if (SQLITE_OK != (ret = sqlite3_open(dbfile, &db))) {
		tolog(log_level, "%s: Can't open database file: %s\n", nm(),
		    dbfile);
		sqlite3_close(db);
		return 1;
	}

	memset(query, 0, sizeof(query));
	// Validate vmname to prevent SQL injection
	if (strlen(vmname) > 50 || strstr(vmname, "'") || strstr(vmname, "\"") || 
	    strstr(vmname, ";") || strstr(vmname, "--") || strstr(vmname, "/*")) {
		tolog(log_level, "Invalid vmname detected, skipping SQL query\n");
		sqlite3_close(db);
		return 1;
	}
	snprintf(query, sizeof(query), "SELECT jid FROM jails WHERE jname=\"%s\"", vmname);
	// tolog(log_level,"SQL[%s](%s)",query,dbfile);
	ret = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);

	if (ret == SQLITE_OK) {
		ret = sqlite3_step(stmt);

		while (ret == SQLITE_ROW) {
			jid = sql_get_int64(stmt);
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
	sqlite3 *db = NULL;
	int res;
	int i;
	char query[1024];
	char *err = NULL;
	int maxretry = 10;
	int retry = 0;
	sqlite3_stmt *stmt = NULL;
	int ret;
	char dbfile[1024];
	unsigned long maxmem = 0;

	// Validate vmname to prevent path traversal
	if (strlen(vmname) > 50 || strstr(vmname, "..") || strstr(vmname, "/") || 
	    strstr(vmname, "\\") || strstr(vmname, "\0")) {
		tolog(log_level, "Invalid vmname detected in get_bhyve_maxmem\n");
		return 1;
	}

	memset(dbfile, 0, sizeof(dbfile));
	snprintf(dbfile, sizeof(dbfile), "%s/jails-system/%s/local.sqlite", workdir, vmname);

	if (SQLITE_OK != (res = sqlite3_open(dbfile, &db))) {
		tolog(log_level, "%s: Can't open database file: %s\n", nm(),
		    dbfile);
		sqlite3_close(db);
		return 1;
	}

	res = 1024;

	snprintf(query, sizeof(query), "SELECT vm_ram FROM settings LIMIT 1");
	ret = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);

	if (ret == SQLITE_OK) {
		ret = sqlite3_step(stmt);

		while (ret == SQLITE_ROW) {
			maxmem = sql_get_int64(stmt);
			ret = sqlite3_step(stmt);
		}
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return maxmem;
}

int
update_racct_bhyve(char *vmname, char *orig_jname, char *vmpath)
{
	struct item_data *target = NULL;
	struct item_data *ch;
	struct item_data *next_ch;
	struct timeval now_time;
	int cur_time = 0;
	int error;
	char *copy;
	char *outbuf = NULL;
	char *tmp;
	char *var;
	size_t outbuflen = RCTL_DEFAULT_BUFSIZE / 4;
	int store = 0;
	char param_name[512];
	char filter[MAXJNAME + 10];
	char unexpanded_rule[MAXJNAME + 10]; // 10 - extra "process::\0"
	pid_t oldpid = 0;
	int vm_cpus = 0;
	unsigned long maxmem = 0;
	FILE *fp;

	sprintf(filter, "process:%d:", cur_bid);
	sprintf(unexpanded_rule, "process:%d", cur_bid);

	gettimeofday(&now_time, NULL);
	cur_time = (time_t)now_time.tv_sec;

	for (ch = item_list; ch; ch = ch->next) {
		if (!strcmp(vmname, ch->name)) {
			tolog(log_level,
			    "update metrics for bhyve: [%s, pid: %d]\n", vmname,
			    cur_bid);
			oldpid = ch->pid;
			ch->modified = nanoseconds();
			ch->pid = cur_bid;

			if (oldpid != cur_bid) {
				tolog(log_level,
				    "oldpid(%d) != curpidr(%d) for %s\n",
				    oldpid, cur_bid, vmname);

				// if PID change, get CPUs from bhyve table for
				// ncpu value
				vm_cpus = get_bhyve_cpus(orig_jname);
				if (vm_cpus == 0) {
					return 0;
				}

				maxmem = get_bhyve_maxmem(orig_jname);
				if (maxmem == 0) {
					return 0;
				}

				tolog(log_level,
				    "* VM PID WAS CHANGES, UPDATE CPUS: %d, UPDATE MAXMEM: %lu\n",
				    vm_cpus, maxmem);

				ch->cpus = vm_cpus;
				ch->maxmem = maxmem;
			} else {
				if (ch->cpus == 0) {
					ch->cpus = get_bhyve_cpus(orig_jname);
				}
				vm_cpus = ch->cpus;

				if (ch->maxmem == 0) {
					ch->maxmem = get_bhyve_maxmem(
					    orig_jname);
				}
				maxmem = ch->maxmem;
			}

			for (;;) {
				outbuflen *= 4;
				outbuf = realloc(outbuf, outbuflen);
				if (outbuf == NULL) {
					err(1, "realloc");
				}

				error = rctl_get_racct(filter,
				    strlen(filter) + 1, outbuf, outbuflen);
				if (error == 0) {
					break;
				}
				if (errno == ERANGE) {
					continue;
				}
				if (errno == ENOSYS) {
					enosys();
				}

				warn(
				    "failed to show resource consumption for '%s'",
				    unexpanded_rule);

				free(outbuf);
				return (error);
			}

			copy = outbuf;
			int i = 0;

			while ((tmp = strsep(&copy, ",")) != NULL) {
				if (tmp[0] == '\0') {
					break;
				}

				while ((var = strsep(&tmp, "=")) != NULL) {
					i++;
					if (var[0] == '\0') {
						free(tmp);
						break;
					}
					if (i == 1) {
						memset(param_name, 0,
						    sizeof(param_name));
						strcpy(param_name, var);
					}
					if (i == 2) {
						// printf("val* %s\n",var);
						if (!strcmp(param_name,
							"cputime")) {
							ch->cputime = atoi(var);
						} else if (!strcmp(param_name,
							       "datasize")) {
							ch->datasize = atoi(
							    var);
						} else if (!strcmp(param_name,
							       "stacksize")) {
							ch->stacksize = atoi(
							    var);
						} else if (!strcmp(param_name,
							       "memoryuse")) {
							ch->memoryuse = atol(
							    var);
						} else if (
						    !strcmp(param_name,
							"memorylocked")) {
							ch->memorylocked = atoi(
							    var);
						} else if (!strcmp(param_name,
							       "maxproc")) {
							ch->maxproc = atoi(var);
						} else if (!strcmp(param_name,
							       "openfiles")) {
							ch->openfiles = atoi(
							    var);
						} else if (!strcmp(param_name,
							       "vmemoryuse")) {
							ch->vmemoryuse = atol(
							    var);
						} else if (!strcmp(param_name,
							       "swapuse")) {
							ch->swapuse = atoi(var);
						} else if (!strcmp(param_name,
							       "nthr")) {
							ch->nthr = atoi(var);
						} else if (!strcmp(param_name,
							       "readbps")) {
							ch->readbps = atoi(var);
						} else if (!strcmp(param_name,
							       "writebps")) {
							ch->writebps = atoi(
							    var);
						} else if (!strcmp(param_name,
							       "readiops")) {
							ch->readiops = atoi(
							    var);
						} else if (!strcmp(param_name,
							       "writeiops")) {
							ch->writeiops = atoi(
							    var);
						} else if (!strcmp(param_name,
							       "pcpu")) {
							if (vm_cpus > 1) {
								ch->pcpu =
								    (atoi(var) /
									vm_cpus);
							} else {
								ch->pcpu = atoi(
								    var);
							}
							if (ch->pcpu > 100) {
								ch->pcpu = 100;
							}
						} else {
							// calculate pmem
							ch->pmem = 100.0 *
							    ch->memoryuse /
							    ch->maxmem;
							if (ch->pmem > 100) {
								ch->pmem = 100;
							}
						}
						i = 0;
					}
				}
			}

			// Note: tmp and var are pointers from strsep(), not allocated memory
			// copy is a pointer to outbuf, so we only free outbuf
			free(outbuf);
		}
	}

	return 0;
}

int
main(int argc, char **argv)
{
	char *dot;
	char *ep;
	char *jname;
	char *pname;
	int c;
	int i;
	int jflags;
	int jid;
	int lastjid;
	int pflags;
	int spc;
	struct item_data *newd;
	struct item_data *temp;
	struct timeval now_time;
	size_t ncpu_len = 0;
	size_t maxmem_len = 0;
	DIR *dirp = NULL;
	int bhyve_exist = 1;
	char *yaml;
	int current_jobs_ready = 0;
	int jobs_max = 2; // jobs_max per one item, one graph = 25 rec
	int jobs_max_all_items = 0;
	int current_waiting = 0;
	BSJ *job;
	int x;
	char rnum[32]; // Increased size to handle larger numbers safely
	int optcode = 0;
	int option_index = 0;

	running_bhyves = 0;

	struct dirent *dp;
	char vmname[MAXJNAME];
	char vmpath[MAXJNAME];
	char tmpjname[MAXJNAME];
	pid_t vmpid;

	static struct option long_options[] = { { "help", no_argument, 0,
						    C_HELP },
		{ "log_file", required_argument, 0, C_LOG_FILE },
		{ "log_level", required_argument, 0, C_LOG_LEVEL },
		{ "loop_interval", required_argument, 0, C_LOOP_INTERVAL },
		{ "prometheus_exporter", required_argument, 0,
		    C_PROMETHEUS_EXPORTER },
		{ "prometheus_listen4", required_argument, 0,
		    C_PROMETHEUS_LISTEN4 },
		{ "prometheus_port", required_argument, 0,
		    C_PROMETHEUS_PORT },
		{ "save_loop_count", required_argument, 0, C_SAVE_LOOP_COUNT },
		{ "save_beanstalkd", required_argument, 0, C_SAVE_BEANSTALKD },
		{ "save_sqlite3", required_argument, 0, C_SAVE_SQLITE3 },
#ifdef WITH_INFLUX
		{ "save_influx", required_argument, 0, C_SAVE_INFLUX },
#endif
		/* End of options marker */
		{ 0, 0, 0, 0 } };

	while (TRUE) {
		optcode = getopt_long(argc, argv, "", long_options,
		    &option_index);
		if (optcode == -1) {
			break;
		}
		switch (optcode) {
		case C_HELP:
			usage();
			break;
		case C_LOG_FILE:
			log_file = optarg;
			break;
		case C_LOG_LEVEL:
			log_level = atoi(optarg);
			break;
		case C_LOOP_INTERVAL:
			loop_interval = atoi(optarg);
			break;
		case C_PROMETHEUS_EXPORTER:
			if (atoi(optarg) != 0)
				output_flags |= OUTPUT_PROMETHEUS;
			break;
		case C_PROMETHEUS_LISTEN4:
			prometheus_listen4 = optarg;
			break;
		case C_PROMETHEUS_PORT:
			prometheus_port = atoi(optarg);
			break;
		case C_SAVE_LOOP_COUNT:
			save_loop_count = atoi(optarg);
			break;
		case C_SAVE_BEANSTALKD:
			if (atoi(optarg) != 0) {
				output_flags |= OUTPUT_BEANSTALKD;
			}
			break;
		case C_SAVE_SQLITE3:
			if (atoi(optarg) != 0) {
				output_flags |= OUTPUT_SQLITE3;
			}
			break;
#ifdef WITH_INFLUX
		case C_SAVE_INFLUX:
			if (atoi(optarg) != 0)
				output_flags |= OUTPUT_INFLUX;
			break;
#endif
		}
	}

	chdir("/var/empty");

	printf("CBSD bhyve racct statistics exporter\n");
	if (log_file) {
		printf("log_file: %s\n", log_file);
	}
	printf("log_level: %d\n", log_level);
	printf("loop_interval: %d seconds\n", loop_interval);
	printf("save_loop_count: %d\n", save_loop_count);
	printf("beanstalkd enabled: %s\n",
	    (OUTPUT_BEANSTALKD & output_flags) ? "yes" : "no");
	printf("prometheus enabled: %s\n",
	    (OUTPUT_PROMETHEUS & output_flags) ? "yes" : "no");
	printf("sqlite3 enabled: %s\n",
	    (OUTPUT_SQLITE3 & output_flags) ? "yes" : "no");
#ifdef WITH_INFLUX
	printf("influx enabled: %s\n",
	    (OUTPUT_INFLUX & output_flags) ? "yes" : "no");
#endif

	if (output_flags == 0) {
#ifdef WITH_INFLUX
		printf(
		    "Error: select at least one backend ( --save_beanstalkd | --save_sqlite3 | --save_influx )\n");
#else
		printf(
		    "Error: select at least one backend ( --save_beanstalkd | --save_sqlite3 )\n");
#endif
		exit(-1);
	}

	ncpu_len = sizeof(ncpu);
	maxmem_len = sizeof(maxmem);

	jname = NULL;
	pflags = jflags = jid = 0;

	int pipe_fd[2];
	pid_t otherpid;
	char name[] = "racct-bhyve-statsd";

	workdir = getenv("workdir");

	if (workdir == NULL) {
		printf("no workdir env\n");
		exit(1);
	}

	if (pipe(pipe_fd) == -1) {
		printf("pipe");
		exit(-1);
	}

	switch (fork()) {
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
	// close(pipe_fd[0]);
	// close(pipe_fd[1]);

	if (path_my_pidfile == NULL) {
		asprintf(&path_my_pidfile, "%s/var/run/cbsd_statsd_bhyve.pid",workdir);
		if (path_my_pidfile == NULL) {
			printf("asprintf");
			exit(1);
		}
	}
	pidfile = pidfile_open(path_my_pidfile, 0644, &otherpid);
	if (pidfile == NULL) {
		if (errno == EEXIST) {
			printf("racct-bhyve-statsd already running, pid: %d.",
			    otherpid);
			exit(1);
		}
		if (errno == EAGAIN) {
			printf("racct-bhyve-statsd already running.");
			exit(1);
		}
		printf("Cannot open or create pidfile: %s", path_my_pidfile);
	}

	if (pidfile != NULL) {
		pidfile_write(pidfile);
	}

	i = sysctlbyname("hw.physmem", &maxmem, &maxmem_len, NULL, 0);

	if (i != 0) {
		if (errno == ENOENT) {
			errx(1,
			    "Unable to determine hoster physical memory via sysctl hw.physmem");
		}
		err(1, "sysctlbyname");
	}

	i = sysctlbyname("hw.ncpu", &ncpu, &ncpu_len, NULL, 0);

	if (i != 0) {
		if (errno == ENOENT) {
			errx(1, "Unable to determine CPU count via hw.ncpu");
		}
		err(1, "sysctlbyname");
	}

	c = 0;

#ifdef WITH_INFLUX
	cbsd_influx_init();
	load_config();
#endif
	if (OUTPUT_PROMETHEUS & output_flags) {
		signal(SIGPIPE, SIG_IGN);
		if (start_prometheus_exporter() != 0)
			err(1, "cannot start Prometheus exporter");
	}

	while (1) {
		tolog(log_level, "main loop\n");
		if ((OUTPUT_BEANSTALKD & output_flags) && bs_connected != 1) {
			if (bs_socket != -1) {
				bs_disconnect(bs_socket);
			}
			bs_socket = init_bs("racct-bhyve");
		} else if (!(OUTPUT_BEANSTALKD & output_flags)) {
			bs_connected = 0;
		}

		while (true) {
			if ((OUTPUT_BEANSTALKD & output_flags) &&
			    bs_connected != 1) {
				break;
			}

#ifdef WITH_INFLUX
			if (OUTPUT_INFLUX & output_flags) {
				if (influx->items >
					(4 * (1 + running_bhyves)) ||
				    influx->items >
					50) { // 50 rows max to be sure we don't
					      // overflow the buffer.
					int rc;
					if ((rc = cbsd_influx_transmit_buffer()) !=
					    0)
						tolog(log_level,
						    "RACCT to Influx failed! [code:%d]\n",
						    rc);
				}
			}
#endif

			tolog(log_level, " round %d/%d\n ---------------- \n",
			    cur_round, save_loop_count);
			// convert round integer to string
			memset(rnum, 0, sizeof(rnum));
			snprintf(rnum, sizeof(rnum), "%d", cur_round);

			dirp = opendir("/dev/vmm");
			if (dirp == NULL) {
				tolog(log_level,
				    "no vmm exist in /dev/vmm, sleep for 60 sec\n");
				sleep(60);
				bhyve_exist = 0;
				continue;
			}
			tolog(log_level, "vmm exist in /dev/vmm\n");
			bhyve_exist = 1;

			if (bhyve_exist == 1) {
				tolog(log_level, "scan for /dev/vmm\n");
				// rewinddir(dirp);
				running_bhyves =
				    0; // We count the items fou automatic
				       // scaling transmission to influx
				while ((dp = readdir(dirp)) != NULL) {
					if (dp->d_name[0] == '.') {
						continue;
					}
					tolog(log_level, "/dev/vmm found: %s\n",
					    dp->d_name);
					memset(vmname, 0, sizeof(vmname));
					memset(vmpath, 0, sizeof(vmpath));
					snprintf(vmpath, sizeof(vmpath), "/dev/vmm/%s",
					    dp->d_name);
					strncpy(vmname, dp->d_name, sizeof(vmname) - 1);
					vmname[sizeof(vmname) - 1] = '\0';
					cur_bid = 0;
					cur_bid = get_vm_pid(vmpath);
					if (cur_bid == 0) {
						continue;
					}

					memset(tmpjname, 0, sizeof(tmpjname));
					strncpy(tmpjname, vmname, sizeof(tmpjname) - 1);
					tmpjname[sizeof(tmpjname) - 1] = '\0';
					
					// Safely concatenate "_" and rnum to vmname
					size_t vmname_len = strlen(vmname);
					if (vmname_len + 1 + strlen(rnum) < sizeof(vmname)) {
						strcat(vmname, "_");
						strcat(vmname, rnum);
					} else {
						tolog(log_level, "Warning: vmname too long, truncating\n");
						vmname[sizeof(vmname) - 1] = '\0';
					}
					i = jname_exist(vmname);
					running_bhyves++;
					if (i) {
						update_racct_bhyve(vmname,
						    tmpjname, vmpath);
						continue;
					}

					CREATE(newd, struct item_data, 1);
					newd->pid = cur_bid;
					newd->modified = 0; // sign of new jail
					newd->maxmem = 0;
					newd->pmem = 0;
					newd->next = item_list;
					item_list = newd;
					strncpy(newd->name, vmname, sizeof(newd->name) - 1);
					newd->name[sizeof(newd->name) - 1] = '\0';
					strncpy(newd->orig_name, tmpjname, sizeof(newd->orig_name) - 1);
					newd->orig_name[sizeof(newd->orig_name) - 1] = '\0';
					tolog(log_level,
					    "[BHYVE] !! %d [%s (%s)] has beed added\n",
					    cur_bid, vmname, tmpjname);
					update_racct_bhyve(vmname, tmpjname, vmpath);
				}
				free(dp);
			}

			if (dirp != NULL) {
				(void)closedir(dirp);
			}

			c++;
			list_data();

			if (c > 5) {
				prune_inactive_env();
				c = 0;
			}

			if ((OUTPUT_BEANSTALKD & output_flags) &&
			    bs_connected == 1) {
				i = bs_stats_tube(bs_socket, "racct-bhyve",
				    &yaml);
				if (yaml) {
					current_jobs_ready = get_bs_stats(yaml,
					    current_jobs_ready_str);
					current_waiting = get_bs_stats(yaml,
					    current_waiting_str);
					free(yaml);
					if (current_jobs_ready < 0) {
						tolog(log_level,
						    "get_bs_stats failed for current-jobs-ready\n");
						bs_connected = 0;
						sleep(loop_interval);
						break;
					}
					if (current_waiting < 0) {
						tolog(log_level,
						    "get_bs_stats failed for current-waiting\n");
						bs_connected = 0;
						sleep(loop_interval);
						break;
					}
					tolog(log_level,
					    "current-jobs: %d, jobs_max_all: %d, current-waiting: %d\n",
					    current_jobs_ready,
					    jobs_max_all_items,
					    current_waiting);
				} else {
					current_waiting = -1;
					current_jobs_ready = -1;
					bs_connected = 0;
					tolog(log_level,
					    "bs_stats_tube yaml error,reset bs connection\n");
					sleep(1);
					break;
				}

				if (current_waiting == 0) {
					skip_beanstalk = 1;
					// no consumer, (flush old data?)
					tolog(log_level,
					    "[debug]no waiting consumer anymore, clear/flush old jobs: %d\n",
					    current_jobs_ready);
					//					for
					//(i=0;i<current_jobs_ready;i++) {
					////remove
					//						bs_reserve_with_timeout(bs_socket,
					// 1, &job);
					// bs_release(bs_socket, job->id, 0, 0);
					// bs_free_job(job);
					//						bs_peek_ready(bs_socket,
					//&job);
					//bs_delete(bs_socket, job->id);
					//						bs_free_job(job);
					//					}
				} else if (current_jobs_ready > 20) {
					skip_beanstalk = 1;
					tolog(log_level,
					    "[debug]too many ready jobs in bs: %d. skip for beanstalk\n",
					    current_jobs_ready);
				} else {
					skip_beanstalk = 0;
				}
			}

			// giant cycle sleep
			tolog(log_level, "\n");
			//			usleep(100000);
			sleep(loop_interval);
			cur_round++;
			if (cur_round > save_loop_count) {
				cur_round = 0;
			}
			if (cur_round == save_loop_count) {
				sum_data_bhyve();
			}
		}
	}

	if (dirp != NULL) {
		(void)closedir(dirp);
	}
	if (pidfile != NULL) {
		pidfile_remove(pidfile);
	}
#ifdef WITH_INFLUX
	cbsd_influx_free();
#endif

	return 0;
}

int
list_data()
{
	struct item_data *target = NULL;
	struct item_data *ch;
	struct item_data *next_ch;
	int ret = 0;

	tolog(log_level, "---listdata---\n");

	for (ch = item_list; ch; ch = ch->next) {
		if (ch->modified == 0) {
			continue;
		}
		tolog(log_level,
		    "TIME:%ld,NAME:%s,ORIGNAME:%s,PID:%d,PCPU:%d,MEM:%lu,PROC:%d,OPENFILES:%d,RB:%d,WB:%d,RIO:%d,WIO:%d,PMEM:%d\n",
		    ch->modified, ch->name, ch->orig_name, ch->pid, ch->pcpu,
		    ch->memoryuse, ch->maxproc, ch->openfiles, ch->readbps,
		    ch->writebps, ch->readiops, ch->writeiops, ch->pmem);
	}

	return 0;
}
