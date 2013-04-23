//#include <sys/syslimits.h>  //for define ARG_MAX                 262144
#define MAX_HEADERSTR_LENGTH 262144
#define MAX_JAIL_NAME 1024

#define MAX_JAILCFG 1
#define MAX_EXCLUDECFG 15   //maximum exlcude rule (-2 system reserverved)
#define MAX_CLIENTSCFG 15   //maximum clients number (-1 for system reservation)

// pipe basedir ( PIPE_SPOOL_BASE+$jname.sock+PIPE_SPOOL_EXT for post notify )
#define PIPE_SPOOL_BASE "/tmp/s_"
#define PIPE_SPOOL_EXT ".sock"

#define DEFAULT_CONF_DIR "/etc"
#define DEFAULT_CONF_FILE "au_router.cfg"
#define DEFAULT_CONF_EXCLUDE_SKEL "-exclude.cfg"
#define DEFAULT_CONF_CLIENT_SKEL "-client.cfg"

#define FALSE 0
#define TRUE 1

#define TOK_PATH_ID 35

#define FILE_MAIN 0
#define FILE_EXCLUDED 1
#define FILE_NOTMAIN 2

#define SPOOL_DIR "/var/spool/cdfs"   //default dir for spool
#define SPOOL_FILE "files.txt"        //default spool file index

//f_add - file added or modify
//f_del - file removed
//d_del - dir removed
// ATTENTION
// to facilitate the sort of priority on the client, the parameters 
// should go in order "LAST  - win"
// e.g - ADD(modify) must be placed after any DEL
enum {
    F_DEL,
    D_DEL,
    F_ADD,
};

struct jailcfg
{
    char path[MAX_HEADERSTR_LENGTH];
    char name[MAX_JAIL_NAME];
    char spoolfile[MAX_JAIL_NAME];
    struct jailcfg *next;
};

struct clientcfg
{
    char name[MAX_HEADERSTR_LENGTH];
    char spoolfile[MAX_JAIL_NAME];
    struct clientcfg *next;
};


struct excludecfg
{
    char path[MAX_HEADERSTR_LENGTH];
    struct excludecfg *next;
};

struct key_items {
    char path[MAX_HEADERSTR_LENGTH];
    char name[MAX_JAIL_NAME];
    struct key_items *next;
};

int load_jailcfg(char *);
int show_jailcfg();

int load_excludecfg(char *);
int show_excludecfg();

int debugmsg(int level,const char *format, ...);
int errmsg(const char *format, ...);

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
   }

/* List of all au_router */
enum {
        C_CONFIG,
        C_EXCLUDEDIR,
	C_HELP,
	C_DEBUG,
};


//for tospool thread param
struct thread_param {
    char *spoolfile;
    int mode;
    char *path;
    int tid;
};
