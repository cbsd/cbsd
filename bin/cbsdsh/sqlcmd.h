#define DEFSQLDELIMER "|"
#define DBPOSTFIX ".sqlite"

#define ERROR_SQLITE(db, query) do {							\
	fprintf(stderr,"sqlite error while executing %s in file %s:%d: %s", (query),	\
	__FILE__, __LINE__, sqlite3_errmsg(db));					\
} while(0)

int sql_exec(sqlite3 *s, const char *sql, ...);
