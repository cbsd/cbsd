// AUDITd-based filesystem watcher
// 1) Have /etc/security/audit_control ~:
/*

dir:/var/audit
dist:off
flags:+fc,+fd,+fw
minfree:5
naflags:+fc,+fd,+fw
policy:cnt,argv
filesz:1M
expire-after:10M

*/
// Part of the CBSD Project

#include <bsm/libbsm.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <getopt.h>
#include <stdarg.h> //for debugmsg/errmsg
#include <pthread.h>

#include "au_router.h"

const char *auditpipe = "/dev/auditpipe";

struct jailcfg *jaillist = NULL;
static struct jailcfg *jailfirst = NULL;

struct clientcfg *clientlist = NULL;
static struct clientcfg *clientfirst = NULL;

struct excludecfg *excludelist = NULL;
static struct excludecfg *excludefirst = NULL;

struct key_items *key_list = NULL;

int debug=0;

char *excludedir = NULL;

int debugmsg(int level,const char *format, ...)
{
va_list arg;
int done;

    if(debug<level) return 0;
    va_start (arg, format);
    done = vfprintf (stdout, format, arg);
    va_end (arg);

return 0;
}

int errmsg(const char *format, ...)
{
   va_list arg;
   int done;

   va_start (arg, format);
   done = vfprintf (stderr, format, arg);
   va_end (arg);

   return 0;
}


//skip for modification under /dev and sockets file
int file_is_special(char *fp)
{
    struct stat sb;
    int n;

    n = stat( fp, &sb );

    if( n < 0 ) return -1;

    if ((sb.st_mode & S_IFMT) == S_IFREG ) n=0;
	else n=1;

    if (S_ISDIR(sb.st_mode)) n=2;

    return n;
}

int regex_match(char *tp,char *t1)
{
regex_t preg;
regmatch_t substmatch[1];

    int match=0;

    if ((strlen(tp)<=1)||(strlen(t1)<=1)) return 1;
    //Basic a little bit faster for this task
    //if (regcomp(&preg, tp, REG_EXTENDED))
    if (regcomp(&preg, tp, REG_BASIC)) {
	errmsg("Bad regex: [%s] - [%s]\n",tp,t1);
	match=0;
	goto endregex;
    }

    if (regexec(&preg, t1, 0, substmatch, 0) == 0 )
        match=1;
    else
        match=0;

endregex:
    regfree(&preg);

return match;
}

// check for path existance and create if not
int check_destination(char *to)
{
struct stat sb;

    return stat(to, &sb);
    //mkdir here
}


//todo: add for skip ^# commented string
int load_excludecfg(char *cfgdir)
{
struct excludecfg *newexclude;
struct jailcfg *jcfg = jailfirst;
int i,record=0,allrecords=0;
char *expath;
FILE *fp;

    while (jcfg!=NULL) {
        if (jcfg->name==NULL) continue;
        expath=malloc(strlen(cfgdir)+strlen(jcfg->name)+strlen(DEFAULT_CONF_EXCLUDE_SKEL)+2);
        sprintf(expath,"%s/%s%s",cfgdir,jcfg->name,DEFAULT_CONF_EXCLUDE_SKEL);
        debugmsg(2,"Exclude sets for %s is %s\n",jcfg->name,expath);

        fp=fopen(expath,"r");

        if (!fp) {
            debugmsg(1,"Exclude set for %s on %s doesn't exist, skipp\n",jcfg->name,expath);
            free(expath);
            jcfg=jcfg->next;
            continue;
        }

        while (!feof(fp)) {
            //make sure we have free slots
            if (record>=MAX_EXCLUDECFG) {
                    errmsg("Exclude cfg slots limit exceed, skip cfg >=%d...\n",record);
                    break;
            }
            CREATE(newexclude, struct excludecfg, 1);
            fscanf(fp,"%s",newexclude->path);
            if (strlen(newexclude->path)<=1) continue;
            newexclude->next= excludefirst;
            excludefirst = newexclude;
            record++;
        }
        free(expath);
        fclose(fp);

	//Finaly, insert into exclude set /var/spool/dfs and /var/audit directory
	//This is protect when we watch on / directory, not jail only
	CREATE(newexclude, struct excludecfg, 1);
	strcpy(newexclude->path, "/var/audit");
	newexclude->next= excludefirst;
	excludefirst = newexclude;

	CREATE(newexclude, struct excludecfg, 1);
	strcpy(newexclude->path, SPOOL_DIR);
	newexclude->next= excludefirst;
	excludefirst = newexclude;

        debugmsg(1,"Excludecfg: Loaded %d configuration\n",record);
        jcfg=jcfg->next;
    }
    return 0;
}

int show_excludecfg()
{
struct excludecfg *ecfg = excludefirst;

    while(ecfg!=NULL) {
        printf(":: %s\n",ecfg->path);
        ecfg=ecfg->next;
    }

return 0;
}

//todo: add for skip ^# commented string
int load_clientcfg(char *cfgdir)
{
struct clientcfg *newclient;
struct jailcfg *jcfg = jailfirst;
int i,record=0,allrecords=0;
char *clpath;
FILE *fp;
char cl[MAX_JAIL_NAME];

    while (jcfg!=NULL) {
	if (jcfg->name==NULL) continue;
	    clpath=malloc(strlen(cfgdir)+strlen(jcfg->name)+strlen(DEFAULT_CONF_CLIENT_SKEL)+2);
	    sprintf(clpath,"%s/%s%s",cfgdir,jcfg->name,DEFAULT_CONF_CLIENT_SKEL);
	    debugmsg(2,"Client sets for %s is %s\n",jcfg->name,clpath);

	    fp=fopen(clpath,"r");

	    if (!fp) {
		debugmsg(1,"Client set for %s on %s doesn't exist, skipp\n",jcfg->name,clpath);
		free(clpath);
		jcfg=jcfg->next;
		continue;
	    }

	    while (!feof(fp)) {
		//make sure we have free slots
		if (record>=(MAX_CLIENTSCFG-1)) {   //MAX_CLIENTSCFG-1 - last slot for local usage
		    errmsg("Client cfg slots limit exceed, skip cfg >=%d...\n",record);
		    break;
		}
		CREATE(newclient, struct clientcfg, 1);
		memset(cl,0,sizeof(cl));
		fscanf(fp,"%s",cl);
		if (strlen(cl)<=1) continue;
		sprintf(newclient->spoolfile,"%s/%s",SPOOL_DIR,cl);
		if (check_destination(newclient->spoolfile)!=0) {
		    errmsg("Directory %s doesn't exist. Skip client cfg\n",newclient->spoolfile);
		    continue;
		}
		strcat(newclient->spoolfile,"/");
		strcat(newclient->spoolfile,SPOOL_FILE);
		strcpy(newclient->name,jcfg->name);
		newclient->next= clientfirst;
		clientfirst = newclient;
		record++;
	    }
	free(clpath);
	fclose(fp);
	debugmsg(1,"Clientcfg: Loaded %d configuration\n",record);

	//Finaly, create last structure as local client from jcfg
	//See spoolcast procedure for detail - last thread for follow data
	CREATE(newclient, struct clientcfg, 1);
	strcpy(newclient->spoolfile,jcfg->spoolfile);
	strcpy(newclient->name,jcfg->name);
	newclient->next= clientfirst;
	clientfirst = newclient;
	record++;
	jcfg=jcfg->next;
    }
    return 0;
}

int show_clientcfg()
{
struct clientcfg *ccfg = clientfirst;

    while(ccfg!=NULL) {
	printf(":: clients for %s - %s\n",ccfg->name,ccfg->spoolfile);
	ccfg=ccfg->next;
    }

return 0;
}



//todo: add for skip ^# commented string
int load_jailcfg(char *cfgfile)
{
struct jailcfg *newjail;
int i,record=0,allrecords=0;
FILE *fp;

    fp=fopen(cfgfile,"r");
    if (!fp) return 1;

    while (!feof(fp)) {
        //make sure we have free slots
        if (record>=MAX_JAILCFG) {
                errmsg("Jail cfg slots limit exceed, skip cfg >=%d...\n",record);
                break;
        }
        CREATE(newjail, struct jailcfg, 1);
        fscanf(fp,"%s %s",newjail->name,newjail->path);
	sprintf(newjail->spoolfile,"%s/%s",SPOOL_DIR,newjail->name);

	if (check_destination(newjail->spoolfile)!=0) {
	    errmsg("No spool dir: %s\n",newjail->spoolfile);
	    fclose(fp);
	    return 1;
	}

	strcat(newjail->spoolfile,"/");
	strcat(newjail->spoolfile,SPOOL_FILE);
        //skip for blank
        if ((strlen(newjail->name)<1)||(strlen(newjail->path)<1)) continue;
        newjail->next= jailfirst;
        jailfirst = newjail;
        record++;

    }
    fclose(fp);

    debugmsg(1,"Jailcfg: Loaded %d configuration\n",record);
return 0;
}

int show_jailcfg()
{
struct jailcfg *jcfg = jailfirst;

    while(jcfg!=NULL) {
        printf(":: %s %s\n",jcfg->name,jcfg->path);
        jcfg=jcfg->next;
    }

return 0;
}


//void *tospool(void *param, char *spoolfile, int mode, char *path) 
void *tospool(void *param)
{
   int i;
   FILE *fp;

  fp=fopen(((struct thread_param*)param)->spoolfile,"a");

  if(!fp) {
	debugmsg(1,"Thr #%d: can't open %s for append\n",((struct thread_param*)param)->tid,((struct thread_param*)param)->spoolfile);
	pthread_exit(NULL);
    }

    //todo: lock for writing
    fprintf(fp,"%d %s\n",((struct thread_param*)param)->mode,((struct thread_param*)param)->path);
    fclose(fp);

    debugmsg(2,"Thr %d: append %s success\n",((struct thread_param*)param)->tid,((struct thread_param*)param)->spoolfile);

    pthread_exit(NULL);
}

// thread cast to all clients spool file
// in last thread we store primary log (see load_clientcfg area)
int spoolcast(int mode,char *path)
{
struct jailcfg *jcfg = jailfirst;
struct clientcfg *ccfg = clientfirst;
long t=0;
long i=0;
pthread_t threads[MAX_CLIENTSCFG];

    while(ccfg!=NULL) {
	struct thread_param *tp;
	if((tp = malloc(sizeof(*tp))) == NULL){
		debugmsg(0,"malloc for thread %d error\n",t);
	        return (-1);
	}

	tp->mode=mode;
	tp->path=path;
	tp->spoolfile=ccfg->spoolfile;
	tp->tid=t;

	if (pthread_create(&threads[t], NULL, tospool, (void*)tp)) {
	    debugmsg(0,"Error creating thread %d\n",t);
	    return 1;
	}
	t++;
	ccfg=ccfg->next;
	}
	debugmsg(0,"Num of thr: %d\n",t);

	for (i=0;i<t;i++) {
	    if (pthread_join(threads[i], NULL)) {
		debugmsg(0,"Error waiting for thread %i of %d\n", i, t);
		return 1;
	    }
	}

return 0;
}


static int
print_path(FILE *fp, char *dst)
{
	u_char *buf;
	char sbuf[1024];
	tokenstr_t tok;
	int reclen;
	int bytesread;
	int part;
	int raw=1;
	int filestat=0;
	struct excludecfg *ecfg = excludefirst;
	struct jailcfg *jcfg = jailfirst;
	int skipme=0;

	while ((reclen = au_read_rec(fp, &buf)) != -1) {
		bytesread = 0;
		part=0;
		while (bytesread < reclen) {
			if (-1 == au_fetch_tok(&tok, buf + bytesread, reclen - bytesread)) break;
			bytesread += tok.len;
			//token id ?
			if ((tok.id == 35)&&(regex_match(dst,tok.tt.path.path))) {
			//check for exclude
			skipme=0;
			ecfg = excludefirst;
			    while (ecfg!=NULL) {
				if (regex_match(ecfg->path,tok.tt.path.path)) {
				    debugmsg(2,"Router: %s in exclude list for %s, skipp\n",tok.tt.path.path,jcfg->name);
				    skipme=1;  //will be skiped
				    break;
				}
				if (skipme==1) break;
				ecfg=ecfg->next;
			    } //end for exclude set check
			    if (skipme==1) break;
			    filestat=file_is_special(tok.tt.path.path);
//			    if ((filestat==0)||(filestat==3))
			    switch (filestat) {
				case 0:debugmsg(1,"CHG-> + %s\n",tok.tt.path.path); spoolcast(F_ADD, tok.tt.path.path); break;
				case 2:debugmsg(1,"CHG-> d+ %s\n",tok.tt.path.path); spoolcast(D_DEL, tok.tt.path.path); break;
			        case -1: debugmsg(1,"CHG-> - %s\n",tok.tt.path.path); spoolcast(F_DEL, tok.tt.path.path); break;
				default: debugmsg(1,"Unknown filestat: %d for %s\n",filestat,tok.tt.path.path); break;
			    }
//				i=addkey_items(jcfg->name,tok.tt.path.path);
//				if (num_elements==num_queue) update2spool();
			} //if tok.id == 35
		} // while bytesread < reclen
		free(buf);
	} //while au_read_rec
	return (0);
}

void usage() {
    printf("Audit router daemon for jail replication\n");
    printf("require: \n");
    printf("opt: config excludedir debug\n");
    printf("config=<path to au_router.cfg> - alternative path to config\n");
    printf("excudedir=<path to dir with exclude fileset> - alternative path for directory\n");
    printf("debug=N - where N can be: 0 (default), 1 (verbose), 2 (more verbose)\n");
    printf("When --config or --excludedir not set, will be use /etc dir of current pwd\n");
}



int
main(int argc, char **argv)
{
	int ch;
	int i;
	FILE *fp;
	char *dst = NULL;
        char *origdir = NULL;
	char *rpath = NULL;
        char *cfgpath = NULL;
        int win = FALSE;
        int optcode = 0;
        int option_index = 0, old_optind = 0;
	struct jailcfg *jcfg = jailfirst;
	char buf[PATH_MAX];

	static struct option long_options[] = {
    	{ "config", required_argument, 0 , C_CONFIG },
        { "excludedir", required_argument, 0 , C_EXCLUDEDIR },
        { "help", no_argument, 0, C_HELP },
        { "debug", required_argument, 0, C_DEBUG },
         /* End of options marker */
        { 0, 0, 0, 0 }
        };

    while (TRUE) {
                optcode = getopt_long_only(argc, argv, "", long_options, &option_index);
                if (optcode == -1)
                        break;
                switch (optcode) {
                        case C_CONFIG:           /* path to config file */
                                cfgpath=optarg;
                                break;
                        case C_EXCLUDEDIR:      /* path to dir with exclude file */
                                excludedir=optarg;
                                break;
                        case C_HELP:      /* usage() */
                                usage();
                                exit(0);
                                break;
                        case C_DEBUG:      /* debuglevel 0-2 */
                                debug=atoi(optarg);
                                break;

                }
    }


//fill default path to cfg file
    rpath=realpath(argv[0],buf);
    origdir=dirname(rpath);
    debugmsg(2,"My realpath: %s\n",rpath);
    debugmsg(2,"My working dir: %s\n",origdir);

    if (cfgpath==NULL) {
        cfgpath=malloc(strlen(origdir)+strlen(DEFAULT_CONF_DIR)+strlen(DEFAULT_CONF_FILE)+2);
        sprintf(cfgpath,"%s%s/%s",origdir,DEFAULT_CONF_DIR,DEFAULT_CONF_FILE);
    }

    if (excludedir==NULL) {
        excludedir=malloc(strlen(origdir)+strlen(DEFAULT_CONF_DIR)+1);
        sprintf(excludedir,"%s%s",origdir,DEFAULT_CONF_DIR);
    }

    debugmsg(2,"My cfgpath: %s",cfgpath);

    if (load_jailcfg(cfgpath)) {
        printf("Bad cfg %s\n",cfgpath);
        return 1;
    }
    debugmsg(2,"My exclude dir: %s\n",excludedir);

    if (load_excludecfg(excludedir)) {
        errmsg("Bad exclude set\n");
        return 1;
    }
//client cfg
    if (load_clientcfg(excludedir)) {
        errmsg("Bad client set\n");
        return 1;
    }

    if (debug==2) {
        printf("Loaded configuration:\n");
        show_jailcfg();
        show_excludecfg();
	show_clientcfg();
    }

    jcfg = jailfirst;
    dst=malloc(strlen(jcfg->path)+2);
    snprintf(dst,strlen(jcfg->path)+2,"^%s",jcfg->path);
    fp = fopen(auditpipe, "r");

    if ((fp == NULL) || (print_path(fp,dst) == -1))
	perror(argv[i]);
    if (fp != NULL)
	fclose(fp);

return (0);
}
