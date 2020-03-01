#define DEFSQLDELIMER "|"
#define DBPOSTFIX ".sqlite"
#include "sqlite3.h"


#ifdef WITH_DBI
#include <dbi/dbi.h>
#include </usr/include/dlfcn.h>

void cbsd_dbi_init();
void cbsd_dbi_free();

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

#endif        





#define ERROR_SQLITE(db, query) do {							\
	fprintf(stderr,"sqlite error while executing %s in file %s:%d: %s", (query),	\
	__FILE__, __LINE__, sqlite3_errmsg(db));					\
} while(0)

int sql_exec(sqlite3 *s, const char *sql, ...);


