// SQL string Maxlen
int debug=0;

void update_inventory(char *column, char *value);

char *dbfile = NULL;
char *workdir = NULL;
char *param = NULL;
char *value = NULL;
char *sqlquery = NULL;
char *jname = NULL;

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
    C_SQLQUERY,
    C_JNAME,
};

struct inventory_db {
    char *rowname;
    char *rowtype;
};

//fields for sqlite scheme and upgrade procedure
// "row name", "type of row", status (1 -actual, 0 - not)
const struct inventory_db sqldb_info[] = {
{ "jname", "text default null unique" },
{ "path", "text default null" },
{ "host_hostname", "text default null" },
{ "ip4_addr", "text default null" },
{ "mount_devfs", "boolean default 0" },
{ "allow_mount", "boolean default 0" },
{ "allow_devfs", "boolean default 0" },
{ "allow_nullfs", "boolean default 0" },
{ "mount_fstab", "boolean default 0" },
{ "mkhostsfile", "boolean default 0" },
{ "devfs_ruleset", "integer default 0" },
{ "interface", "text default null" },
{ "basename", "text default null" },
{ "slavenode", "text default null" },
{ "baserw", "boolean default 0" },
{ "mount_src", "boolean default 0" },
{ "mount_obj", "boolean default 0" },
{ "mount_kernel", "text default null" },
{ "mount_ports", "boolean default 0" },
{ "astart", "integer default 0" },
{ "data", "text default null" },
{ "vnet", "boolean default 0" },
{ "applytpl", "boolean default 0" },
{ "mdsize", "integer default 0" },
{ "rcconf", "text default null" },
{ "floatresolv", "boolean default 0" },
{ "ver", "text default null" },
{ "arch", "text default null" },
{ "exec_start", "text default null" },
{ "exec_stop", "text default null" },
{ "exec_poststart", "text default null" },
{ "exec_poststop", "text default null" },
{ "exec_prestart", "text default null" },
{ "exec_prestop", "text default null" },
{ "exec_master_poststart", "text default null" },
{ "exec_master_poststop", "text default null" },
{ "exec_master_prestart", "text default null" },
{ "exec_master_prestop", "text default null" },
{ "status", "integer default 0" },
{ "\n", NULL } };       // this must be last

void update_inventory(char *column, char *value);
void delete_inventory(char *column);
void usage();

