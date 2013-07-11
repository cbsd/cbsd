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
{ "jname", "text default null", 1 },
{ "path", "text default null", 1 },
{ "host_hostname", "text default null", 1 },
{ "ip4_addr", "text default null", 1 },
{ "mount_devfs", "boolean default 0", 1 },
{ "mount_devfs", "boolean default 0", 1 },
{ "mount_devfs", "boolean default 0", 1 },
{ "mount_devfs", "boolean default 0", 1 },

{ "\n", NULL, 0 } };       // this must be last

void update_inventory(char *column, char *value);
void delete_inventory(char *column);
void usage();

