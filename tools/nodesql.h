// Part of CBSD Project
// mailto: olevole at olevole dot ru
// SQL string Maxlen
int debug=0;

char *dbfile = NULL;
char *workdir = NULL;
char *ip = NULL;
unsigned int port = 0;
char *keyfile = NULL;
char *rootkeyfile = NULL;
char *nodename = NULL;
char *param = NULL;

#define FALSE 0
#define TRUE 1

// Action list,
enum {
    NOP, //first correct value must not equal 0
    INIT,
    LIST,
    INSERT,
    DELETE,
    SSHOPT,
    SELECT,
    SCPOPT,
};

#define _ITEM(x) [x] = #x

// Action list array//
char *actionlist[] =
{
    [INIT]="init",
    [LIST]="list",
    [INSERT]="insert",
    [DELETE]="delete",
    [SSHOPT]="sshopt",
    [SELECT]="select",
    [SCPOPT]="scpopt",
};

/* List of all nodesql */
enum {
    C_DBFILE,
    C_ACTION,
    C_HELP,
    C_DEBUG,
    C_WORKDIR,
    C_IP,
    C_PORT,
    C_KEYFILE,
    C_ROOTKEYFILE,
    C_NODENAME,
    C_PARAM,
};

int select_callbacksshopt(void *p_data, int num_fields, char **p_fields, char **p_col_names);
int select_stmtsshopt(const char* nodename);
int select_stmtscpopt(const char* nodename);
int select_param(char *,char *);
int sql_stmt(const char* stmt);
void delete_nodes(char *nodename);
void insert_nodes(char *nodename, char *ip, int port, char *keyfile, char *rootkeyfile);
void usage();
