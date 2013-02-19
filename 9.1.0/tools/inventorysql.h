// SQL string Maxlen
#define SQLSTRLEN 4096
sqlite3* db;
int first_row;

int select_callback(void *p_data, int num_fields, char **p_fields, char **p_col_names);
int select_stmt(const char* stmt);
int sql_stmt(const char* stmt);
void update_inventory(char *column, char *value);

int debugmsg(int level,const char *format, ...);
int errmsg(const char *format, ...);

char *dbfile = NULL;
int debug=0;
char *workdir = NULL;
char *param = NULL;
char *value = NULL;

#define FALSE 0
#define TRUE 1

// Action list,
enum {
    NOP, //first correct value must not equal 0
    INIT,
    LIST,
    DELETE,
    UPDATE,
};

// Action list array//
char *actionlist[] =
{
    [INIT]="init",
    [LIST]="list",
    [DELETE]="delete",
    [UPDATE]="update"
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
};

