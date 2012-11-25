#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <arpa/inet.h>


#include <math.h>

#include <sys/socket.h>         /* for PF_LINK */
#include <sys/sysctl.h>
#include <sys/stat.h> //stat, stat()
#include <sys/time.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/resource.h>

#include <err.h>
#include <langinfo.h>
#include <locale.h>
#include <pwd.h>

#include <errno.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_mib.h>

#include "ncctld.h"

char subsystem[25];
char prompt[70];
char progpath[255];
char nodeip[18];

int mode=INIT;

int command_interpreter(char *argument);
void skip_spaces(char **string);
char *remove_double_spaces(char *str);
void flush_arg_struct();
void flush_subarg_struct();
int squeezechar(char *l, int ch);
int check_external_cmd(char *,int );
int mysystem(char *path);
char *rmws(char *);
int argum(char *);

//arguments
struct arg_data *arg_list = NULL;
struct subarg_data *subarg_list = NULL;

/* prototypes for all do_x functions. */
ACMD(do_quit);
ACMD(do_version);
ACMD(do_exit);
ACMD(do_help);
ACMD(do_list);
ACMD(do_set);
ACMD(do_unset);
ACMD(do_job);


const struct command_info cmd_info[] = {
{ "RESERVED",  0, 0, 0 },   /* this must be first -- for specprocs */
{ "quit", do_quit, ANY, 0 },
{ "version", do_version, INIT, 0 },
{ "exit", do_exit, INTERACTIVE, 0 },
{ "help", do_help, INTERACTIVE, 0 },  //display needed parameters
{ "list", do_list, INTERACTIVE, 0 },  //display current parameters set
{ "set", do_set, INTERACTIVE, 0 },  //display needed parameters
{ "unset", do_unset, INTERACTIVE, 0 },  //display current parameters set
{ "job", do_job, ANY, 0 },
{ "\n", 0, 0,0 } }; /* this must be last */

const struct syspath_data path_info[] = {
{ "/usr/jails/system/" },
{ "/usr/jails/jailctl/" },
{ "/usr/jails/tools/" },
{ 0 }, //* this must be last */
};


int main(int argc, char *argv[])
{
char argum[1024];
char *args;
int shutdown=0;
unsigned int i=0;
struct sockaddr_in peer;
char remotehost[20];
FILE *fp;

memset(nodeip,0,sizeof(nodeip));
// lol section
//fp=popen("/usr/bin/grep ^nodeip /usr/jails/cbsd.conf | cut -d\" -f 2","r");
memset(argum,0,sizeof(argum));
fp=popen("/usr/bin/grep ^nodeip /usr/jails/cbsd.conf|cut -d'\"' -f 2","r");
fgets(argum,sizeof(argum),fp);
pclose(fp);
sscanf(argum,"%s",nodeip);
// end of lol section

//set buffered terminal type
setvbuf(stdout,NULL,_IOLBF,0);
setvbuf(stdin,NULL,_IOLBF,0);

i=sizeof(peer);
if (getpeername(0, (struct sockaddr *)&peer, &i) < 0) {
// syslog
exit(1);
}
inet_ntop(AF_INET, &((struct sockaddr_in *)&peer)->sin_addr,remotehost,sizeof remotehost);
//New connections from: [%s]",remotehost

printf("200 READY\r\n");

do {
memset(argum,0,sizeof(argum));
memset(prompt,0,sizeof(prompt));
sprintf(prompt,"%s",PROMPT);
if (strlen(subsystem)>1) { strcat(prompt,"/"); strcat(prompt,subsystem); }
if (strlen(nodeip)>8) printf("%s(%s)# ",prompt,nodeip);
else printf("%s# ", prompt);
fgets(argum,sizeof(argum),stdin);
args=rmws(argum);
command_interpreter(args);

} while(!shutdown);

return EXIT_SUCCESS;
}



int command_interpreter(char *argstr)
{
int cmd=0, length,argnum;
char *line;
char *arg;
char *argument=argstr;
struct arg_data *a;
struct subarg_data *b;
char *st;
int i=0;

squeezechar(argument,' ');

if (!*argument)
return 0;

flush_arg_struct();
//flush_subarg_struct(); //must be flushed when user come away from interactive mode
//printf("[%s]",argument);
//exit;
argnum=argum(argument);

//for (a = arg_list; a; a = a->next)
//{
//printf("[%s]\n",a->arg);
//}

a=arg_list;
st=a->arg;


    for (length = strlen(st), cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
    {
        if (!strncmp(cmd_info[cmd].command, st, length))
         {
          if (argnum==cmd_info[cmd].subcmd) {
             if ((mode!=cmd_info[cmd].minimum_level)&&(cmd_info[cmd].minimum_level!=ANY)) {
             printf("Not allowed in current state\r\n");return 1;
             }
          }
          {((*cmd_info[cmd].command_pointer) (argument,1)); return 0;}
        }
    else {
         printf("510 Need %d arguments for %s command\r\n",cmd_info[cmd].subcmd,cmd_info[cmd].command);return 1;
         }
    }



cmd=check_external_cmd(st,argnum);
if (cmd==1) printf("Sorry [%s]- WTF?\r\n",st);
else
if (cmd==2) printf("Not compatible version, please update\r\n");

return 0;
}

//return 0 if ok
//return 1 when not 
//return 2 when file exist but not response for UTIL_ARG_PREF
int check_external_cmd(char *cmd,int argnum)
{
// max path determine from OS side
char path[(512+strlen(cmd))+MAXARGLEN];
char buf[1024];
FILE *fp;
int sign=strlen(NODESIGNATURE);
int numarg=0;
int i,j=0,num=0;
int found=0;
struct stat info;
int ret = -1;

struct subarg_data *newsubarg;
struct arg_data *a;
struct syspath_ *sysp;

while(path_info[num].path) {
  sprintf(path,"%s%s",path_info[num].path,cmd);
//  printf("Try to open: [%s%s]\r\n",path,cmd);
  ret = stat(path, &info);
  if(ret == 0) { found++; strcpy(progpath,path_info[num].path);break; }
  num++;
}


if (found==0) return 1;
memset(path,0,sizeof(path));
sprintf(path,"%s%s %s 2>/dev/null",progpath,cmd,UTIL_ARG_PREF);
fp=popen(path,"r");

i=0;
while(!feof(fp)) {
  memset(buf,0,sizeof(buf));
  fgets(buf,1024,fp);
    if (feof(fp)) break;
      if (i==0) {
	if (strlen(buf)>sign)
	if (strncmp(buf,NODESIGNATURE,sign)) {
	    pclose(fp); return 2; 
	}
    sscanf(buf,"%s %d",path,&numarg); 
    //prepare for new data
    flush_subarg_struct();
    } //if i;
//if (i>numarg) break;
if (i>0) {  //fill subarg_data

if (subarg_list) {
  newsubarg = subarg_list;
  while (newsubarg->next)
    newsubarg = newsubarg->next;
  CREATE(newsubarg->next, struct subarg_data, 1);
  memset((char *) newsubarg->next, 0, sizeof(struct subarg_data));
  strncpy(newsubarg->arg,buf,strlen(buf)-1);
  newsubarg->next->prev = newsubarg;
  newsubarg = newsubarg->next;
} 
else {
CREATE(newsubarg, struct subarg_data, 1);
memset((char *) newsubarg, 0, sizeof(struct subarg_data));
subarg_list = newsubarg;
CREATE(newsubarg->next, struct subarg_data, 1);
memset((char *) newsubarg->next, 0, sizeof(struct subarg_data));
strncpy(newsubarg->arg,buf,strlen(buf)-1);
newsubarg->next->prev = newsubarg;
newsubarg = newsubarg->next;
}
  
} //if i>0
i++; // this pointer controlhow get parameters
//printf("[%d][%d]\n",numarg,i);
if (i>numarg) break;
  
} //while feof
pclose(fp);

//if ((i==0)||(numarg==0)) return 1;
if (i==0) return 1;

if ((argnum==numarg)&&(mode!=INTERACTIVE))  {
memset(path,0,sizeof(path));
sprintf(path,"%s%s",progpath,cmd);
mysystem(path);
return 0;
}
else {
mode=INTERACTIVE;
strcpy(subsystem,cmd);
}

return 0;
}



void flush_arg_struct()
{
struct arg_data *temp;
struct arg_data *a;

for (a = arg_list; a; a = a->next)
REMOVE_FROM_LIST(a,arg_list,next);
}

void flush_subarg_struct()
{
struct subarg_data *temp;
struct subarg_data *a;

for (a = subarg_list; a; a = a->next)
REMOVE_FROM_LIST(a,subarg_list,next);
}



//remove from l repeating ch
int squeezechar(char *l, int ch)
{
    char *p, *q;

    if (l == NULL)
    return EOF;
    for (p = q = l; (*p = *q) != '\0'; p++, q++)
    if (*p == ch)
    while (*(q+1) == ch)
    q++;
return q-p;
}


char *rmws(char *str) // Remove White Spaces
{
char *obuf, *nbuf;
for (obuf = str, nbuf = str; *obuf && obuf; ++obuf)
{
if (!isspace(*obuf))
*nbuf++ = *obuf;
if (*obuf==' ') *nbuf++ = *obuf;

}
*nbuf = '\0';
return str;
}


int argum(char *str)
{
int i,j=0,argnum=0,len=strlen(str);
struct arg_data *newarg;

if (arg_list) {
  newarg = arg_list;
  while (newarg->next)
    newarg = newarg->next;
  CREATE(newarg->next, struct arg_data, 1);
  memset((char *) newarg->next, 0, sizeof(struct arg_data));
  newarg->next->prev = newarg;
  newarg = newarg->next;
} else {
  CREATE(newarg, struct arg_data, 1);
  memset((char *) newarg, 0, sizeof(struct arg_data));
  arg_list = newarg;
}

for (i=0; i<len; i++ )
{
newarg->arg[j]=str[i];

if (isspace(newarg->arg[j])) {
    newarg->arg[j]='\0';
    j=-1;
    CREATE(newarg->next, struct arg_data, 1);
    memset((char *) newarg->next, 0, sizeof(struct arg_data));
    newarg->next->prev = newarg;
    newarg = newarg->next;
    argnum++;
}
j++;
}

if (j>0) {
  CREATE(newarg->next, struct arg_data, 1);
  memset((char *) newarg->next, 0, sizeof(struct arg_data));
  newarg->next->prev = newarg;
  newarg = newarg->next;
}

return argnum;
}


ACMD(do_quit)
{
printf("200 CUL8R\r\n");
flush_arg_struct();
flush_subarg_struct();
exit(0);
}

ACMD(do_version)
{
printf("200 Version CUL8R\r\n");
}

ACMD(do_exit)
{
mode=INIT;
memset(subsystem,0,sizeof(subsystem));
}

ACMD(do_help)
{
  struct subarg_data *b;
  printf("This parameters needed by %s\r\n",subsystem);

  for (b = subarg_list; b; b = b->next)
  printf("- %s\n",b->arg);
}

ACMD(do_list)
{
  printf("200 list\r\n");
 }

ACMD(do_set)
{
  printf("200 set\r\n");
}

ACMD(do_unset)
{
  printf("200 unset\r\n");
}

ACMD(do_job)
{
  printf("200 job enable\r\n");
}

int mysystem(char *path)
{
struct arg_data *a;
int all=0;
char cmd[5048];

  //printf("Going to executing %s ... with args\r\n",path);

  memset(cmd,0,sizeof(cmd));
  strcpy(cmd,progpath);
  for (a = arg_list; a; a = a->next)
  {
    strcat(cmd,a->arg); 
    strcat(cmd," ");
  }
  //printf("Execution [%s]\n",cmd);
  system(cmd);

return 0;
}
