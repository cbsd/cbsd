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
char *sqlquery = NULL;
char *invfile = NULL;

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
    GET,
    SCPOPT,
    UPGRADE,
    UPDATE,
    MUST_BE_LAST,
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
    [GET]="get",
    [SCPOPT]="scpopt",
    [UPGRADE]="upgrade",
    [UPDATE]="update",
    [MUST_BE_LAST]="NULL",
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
    C_SQLQUERY,
    C_INVFILE
};

struct nodes_db {
    char *rowname;
    char *rowtype;
};

//fields for sqlite scheme and upgrade procedure
// "row name", "type of row", status (1 -actual, 0 - not)
const struct nodes_db sqldb_info[] = {
{ "nodename", "TEXT UNIQUE PRIMARY KEY" },
{ "ip", "TEXT" },
{ "port", "INTEGER" },
{ "keyfile", "TEXT" },
{ "rootkeyfile", "TEXT" },
{ "status", "INTEGER" },
{ "invfile", "TEXT" },
{ "idle", "TIMESTAMP DATE DEFAULT (datetime('now','localtime'))" },
{ "\n", NULL } };       // this must be last


int select_callbacksshopt(void *p_data, int num_fields, char **p_fields, char **p_col_names);
int select_stmtsshopt(const char* nodename);
int select_stmtscpopt(const char* nodename);
int sql_stmt(const char* stmt);
void delete_nodes(char *nodename);
void insert_nodes(char *nodename, char *ip, int port, char *keyfile, char *rootkeyfile, char *);
void usage();
