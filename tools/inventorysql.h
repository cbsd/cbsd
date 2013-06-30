// SQL string Maxlen
int debug=0;

void update_inventory(char *column, char *value);

char *dbfile = NULL;
char *workdir = NULL;
char *param = NULL;
char *value = NULL;
char *sqlquery = NULL;

#define FALSE 0
#define TRUE 1

// Action list,
enum {
    NOP, //first correct value must not equal 0
    INIT,
    LIST,
    DELETE,
    UPDATE,
    GET,
};

// Action list array//
char *actionlist[] =
{
    [INIT]="init",
    [LIST]="list",
    [DELETE]="delete",
    [UPDATE]="update",
    [GET]="get",
};

/* List of all nodesql */
enum {
    C_DBFILE,
    C_ACTION,
    C_HELP,
    C_DEBUG,
    C_WORKDIR,
    C_PARAM,
    C_VALUE,
    C_SQLQUERY
};

struct inventory_db {
    char *rowname;
    char *rowtype;
    int actual;
};

//fields for sqlite scheme and upgrade procedure
// "row name", "type of row", status (1 -actual, 0 - not)
const struct inventory_db sqldb_info[] = {
{ "nodename", "text default null", 1 },
{ "hostname", "text default null", 1 },
{ "nodeip", "text default null", 1 },
{ "nodedescr", "text default null", 1 },
{ "jnameserver", "text default null", 1 },
{ "nodeippool", "text default null", 1 },
{ "natip", "text default null", 1 },
{ "nat_enable", "text default null", 1 },
{ "fbsdrepo", "boolean default 1", 1 },
{ "mdtmp", "integer default 0", 1 },
{ "repo", "text default null", 1 },
{ "workdir", "text default null", 1 },
{ "ipfw_enable", "boolean default 0", 1 },
{ "fs", "text default null", 1 },
{ "zfsfeat", "boolean default 0", 1 },
{ "jail_interface", "text default null", 1 },
{ "ncpu", "integer default 0", 1 },
{ "physmem", "integer default 0", 1 },
{ "memtype", "text default null", 1 },
{ "disks", "text default null", 1 },
{ "cpumodel", "text default null", 1 },
{ "cpufreq", "integer default 0", 1 },
{ "kernhz", "integer default 0", 1 },
{ "sched", "text default null", 1 },
{ "eventtimer", "text default null", 1 },
{ "nics", "text default null", 1 },
{ "parallel", "integer default 5", 1 },
{ "\n", NULL, 0 } };       // this must be last

void update_inventory(char *column, char *value);
void delete_inventory(char *column);
void usage();

