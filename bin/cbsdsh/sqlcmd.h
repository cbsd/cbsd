#define DEFSQLDELIMER "|"
#define DBPOSTFIX ".sqlite"
#include "sqlite3.h"


#ifdef WITH_DBI
#include <dbi/dbi.h>
#include </usr/include/dlfcn.h>

void dbi_load_config();
void dbi_free();

#define DCF_DISABLED	1
#define DCF_CONNECTED	2

#define _sql_config_t struct _sql_config_t_
typedef struct _sql_config_t_{
        dbi_conn conn;
        dbi_result result;
        char *name;
        char *type;
        char *hostname;
        char *username;
        char *password;
        char *database;
        char *encoding;
	uint16_t port;

        uint32_t flags;
	_sql_config_t *next;
} sql_database_t;
#undef _sql_config_t

typedef struct {
	void 		*lib_handle;		// Connection to the DBI library
        dbi_inst 	instance;		// Instance
	sql_database_t	*list;			// Connections
} cbsddbi_t;


int (*_dbi_initialize)(const char *driverdir, dbi_inst *pInst);
void (*_dbi_shutdown)(dbi_inst Inst);
int (*_dbi_conn_error)(dbi_conn Conn, const char **errmsg_dest);
dbi_conn (*_dbi_conn_new)(const char *name, dbi_inst Inst);
int (*_dbi_conn_set_option)(dbi_conn Conn, const char *key, char *value);
int (*_dbi_conn_connect)(dbi_conn Conn);
void (*_dbi_conn_close)(dbi_conn Conn);
dbi_result (*_dbi_conn_query)(dbi_conn Conn, const char *data);
int (*_dbi_result_next_row)(dbi_result Result);
int (*_dbi_result_free)(dbi_result Result);
unsigned int (*_dbi_result_get_numfields)(dbi_result Result);

const char *(*_dbi_result_get_field_name)(dbi_result Result, unsigned int idx);
char *(*_dbi_result_get_as_string_copy_idx)(dbi_result Result, unsigned int idx);

#endif        





#define ERROR_SQLITE(db, query) do {							\
	fprintf(stderr,"sqlite error while executing %s in file %s:%d: %s", (query),	\
	__FILE__, __LINE__, sqlite3_errmsg(db));					\
} while(0)

int sql_exec(sqlite3 *s, const char *sql, ...);


