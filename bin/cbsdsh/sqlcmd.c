#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "builtins.h"
#include "error.h"
#include "eval.h"
#include "jobs.h"
#include "main.h"
#include "memalloc.h"
#include "myhistedit.h"
#include "mystring.h"
#include "nodes.h" /* for other headers */
#include "options.h"
#include "output.h"
#include "shell.h"
#include "show.h"
#include "sqlcmd.h"
#include "syntax.h"
#include "trap.h"
#include "var.h"

#define CBSD_SQLITE_BUSY_TIMEOUT 25000

char *delim;
#ifdef WITH_DBI
extern cbsddbi_t *databases;

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
char *(
    *_dbi_result_get_as_string_copy_idx)(dbi_result Result, unsigned int idx);
void (*_dbi_conn_error_handler)(dbi_conn Conn,
    dbi_conn_error_handler_func function, void *user_argument);
void (*_dbi_set_verbosity)(int verbosity, dbi_inst Inst);

#endif

char *
nm(void)
{
	return "cbsdsql";
}

int
sql_exec(sqlite3 *s, const char *sql, ...)
{
	va_list ap;
	const char *sql_to_exec;
	char *sqlbuf = NULL;
	char *errmsg;
	int ret = 0;

	if (s == NULL)
		return 1;
	if (sql == NULL)
		return 1;

	if (strchr(sql, '%') != NULL) {
		va_start(ap, sql);
		sqlbuf = sqlite3_vmprintf(sql, ap);
		va_end(ap);
		sql_to_exec = sqlbuf;
	} else {
		sql_to_exec = sql;
	}

	if (sqlite3_exec(s, sql_to_exec, NULL, NULL, &errmsg) != SQLITE_OK) {
		ERROR_SQLITE(s, sql_to_exec);
		sqlite3_free(errmsg);
		goto cleanup;
	}

	ret = 1;

cleanup:
	if (sqlbuf != NULL)
		sqlite3_free(sqlbuf);

	return (ret);
}

int
sqlCB(sqlite3_stmt *stmt)
{
	int icol;
	const char *colname;
	int allcol;
	int printheader = 0;
	char *sqlcolnames = NULL;

	if (stmt == NULL)
		return 1;

	sqlcolnames = getenv("sqlcolnames");
	allcol = sqlite3_column_count(stmt);

	if ((printheader) && (sqlcolnames == NULL)) {
		for (icol = 0; icol < allcol; icol++) {
			colname = sqlite3_column_name(stmt, icol);
			if (icol != (allcol - 1))
				out1fmt("%s%s", colname, delim);
			else
				out1fmt("%s\n", colname);
		}
	}
	for (icol = 0; icol < allcol; icol++) {
		if (sqlcolnames)
			out1fmt("%s=\"%s\"\n", sqlite3_column_name(stmt, icol),
			    sqlite3_column_text(stmt, icol));
		else {
			if (icol == (allcol - 1))
				out1fmt("%s\n",
				    sqlite3_column_text(stmt, icol));
			else
				out1fmt("%s%s", sqlite3_column_text(stmt, icol),
				    delim);
		}
	}

	return 0;
}

// External SQL
#ifdef WITH_DBI

void
sql_error_handler(dbi_conn conn, void *user)
{
	sql_database_t *config = user;
	const char *msg;
	_dbi_conn_error(conn, &msg);
	fprintf(stderr, "SQL error in instance %s: [%s]!\n", config->name, msg);
}

bool
sql_connect(sql_database_t *config)
{
	if (DCF_DISABLED & config->flags) {
		fprintf(stderr, "SQL Instance %s is disabled!\n", config->name);
		return (false);
	}
	if (!config->conn) {

		if (!(config->conn = _dbi_conn_new(config->type,
			  databases->instance))) {
			fprintf(stderr,
			    "%s: Problem initializing dbi-%s instance!\n",
			    config->name, config->type);
			return (false);
		}

		if (strcmp("sqlite3", config->type) == 0) {
			_dbi_conn_set_option(config->conn, "dbname",
			    config->database ? config->database : "local.db");
			_dbi_conn_set_option(config->conn, "sqlite3_dbdir",
			    config->username ? config->username : "/var/db/");
		} else {
			_dbi_conn_set_option(config->conn, "host",
			    config->hostname ? config->hostname : "");
			_dbi_conn_set_option(config->conn, "username",
			    config->username ? config->username : "cbsd");
			_dbi_conn_set_option(config->conn, "password",
			    config->password ? config->password : "cbsd");
			_dbi_conn_set_option(config->conn, "dbname",
			    config->database ? config->database : "cbsd");
			_dbi_conn_set_option(config->conn, "encoding",
			    config->encoding ? config->encoding : "UTF-8");
		}

		_dbi_conn_error_handler(config->conn, sql_error_handler,
		    (void *)config);
	}

	if (!(DCF_CONNECTED & config->flags) &&
	    _dbi_conn_connect(config->conn) < 0) {
		const char *er;
		_dbi_conn_error(config->conn, &er);
		fprintf(stderr,
		    "Could not connect to the database. Please check the database settings.\nSQL error: '%s'\n",
		    er);
		return (false);
	}

	config->flags |= DCF_CONNECTED;
	return (true);
}

int
sql_result(dbi_result result)
{
	int index;
	const char *colname;
	int printheader = 0;
	char *delim;
	const char *sqlcolnames = NULL;

	sqlcolnames = getenv("sqlcolnames");
	if ((delim = lookupvar("sqldelimer")) == NULL)
		delim = DEFSQLDELIMER;

	while (_dbi_result_next_row(result)) {
		unsigned int amount = _dbi_result_get_numfields(result);

		if ((printheader == 1) && (sqlcolnames == NULL)) {
			for (index = 1; index <= amount; index++) {
				colname = _dbi_result_get_field_name(result,
				    index);
				if (index != amount)
					out1fmt("%s%s", colname, delim);
				else
					out1fmt("%s\n", colname);
			}
			printheader = 2;
		}

		for (index = 1; index <= amount; index++) {
			char *item = _dbi_result_get_as_string_copy_idx(result,
			    index);
			if (sqlcolnames)
				out1fmt("%s=\"%s\"\n",
				    _dbi_result_get_field_name(result, index),
				    item);
			else if (index == amount)
				out1fmt("%s\n", item);
			else
				out1fmt("%s%s", item, delim);

			free(item);
		}
	}
	_dbi_result_free(result);

	return 0;
}

int
sqlcmd(int argc, char **argv)
{
	size_t len = 0;
	int i, rc;
	char *query;

	for (i = 2; i < argc; i++)
		len += strlen(argv[i]) + 1;
	if (len == 0) {
		fprintf(stderr, "Query is missing!\n");
		return (1);
	}

	sql_database_t *seek;
	for (seek = databases->list; NULL != seek; seek = seek->next)
		if (strlen(seek->name) == strlen(argv[1] + 1) &&
		    strcmp(seek->name, argv[1] + 1) == 0)
			break;

	if (!seek) {
		fprintf(stderr, "Invalid database!\n");
		return (1);
	}

	if (argc == 3 && strcmp(argv[2], "gettype") == 0) {
		printf("%s\n", seek->type);
		return (0);
	}

	if (!sql_connect(seek))
		return (1);

	// Build the query.. [todo add escaping?]
	query = malloc(len);
	query[0] = 0;
	char *tmp = query;
	for (i = 2; i < argc; i++) {
		strcpy(tmp, argv[i]);
		tmp += strlen(tmp);
		*tmp = ' ';
		tmp++;
	}
	tmp[-1] = 0;

	// Excute query
	dbi_result result = _dbi_conn_query(seek->conn, query);

	if ((rc = sql_result(result)) != 0) {
		printf("Failed Query: [%s]\n", query);
	}

	free(query);
	return (rc);
}
#endif

int
sqlitecmdrw(int argc, char **argv)
{
	sqlite3 *db;
	int res;
	int i;
	char *query;
	char *tmp;
	char *dbdir;
	char *dbfile;
	int ret = 0;
	sqlite3_stmt *stmt;
	char *cp;
	int maxretry = 50;
	int retry = 0;

	//	const char journal_mode_sql[] = "PRAGMA journal_mode = MEMORY;";
	//	const char journal_mode_sql[] = "PRAGMA journal_mode = WAL;"; //
	//SR - not used?

	if (argc < 3) {
		out1fmt("%s: format: %s <dbfile> <query>\n", nm(), nm());
		return (1); // SR: Usage should also give an error for scripting
	}

	if (argv[1][0] == '@') {
#ifndef WITH_DBI
		printf(
		    "External SQL not implemented, recompile cbsdsh WITH_DBI\n");
		return (1);
#else
		return (sqlcmd(argc, argv));
#endif
	}

	if ((cp = lookupvar("sqldelimer")) == NULL)
		delim = DEFSQLDELIMER;
	else
		delim = cp;
	if (argv[1][0] != '/') {
		// search file in dbdir
		dbdir = lookupvar("dbdir");
		i = strlen(dbdir) + strlen(argv[1]);
		dbfile = calloc(strlen(dbdir) + strlen(argv[1]) +
			strlen(DBPOSTFIX) + 1,
		    sizeof(char *));

		if (dbfile == NULL) {
			error("Out of memory!\n");
			return (1);
		}
		sprintf(dbfile, "%s/%s%s", dbdir, argv[1], DBPOSTFIX);
	} else {
		dbfile = calloc(strlen(argv[1]) + 1, sizeof(char *));
		sprintf(dbfile, "%s", argv[1]);
	}

	if (SQLITE_OK !=
	    (res = sqlite3_open_v2(dbfile, &db,
		 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
		     SQLITE_OPEN_SHAREDCACHE,
		 NULL))) {
		//	if (SQLITE_OK != (res = sqlite3_open(dbfile, &db))) {
		out1fmt("%s: Can't open database file: %s\n", nm(), dbfile);
		free(dbfile);
		return 1;
	}
	free(dbfile);

	sql_exec(db, "PRAGMA mmap_size = 209715200;");
	sqlite3_busy_timeout(db, CBSD_SQLITE_BUSY_TIMEOUT);
	sql_exec(db, "PRAGMA journal_mode = WAL;");
	sql_exec(db, "PRAGMA synchronous = NORMAL;");
	//	sql_exec(db, "PRAGMA journal_mode=DELETE;");
	//	sql_exec(db,"PRAGMA journal_mode = OFF;");
	//	sql_exec(db,"PRAGMA journal_mode = TRUNCATE;");

	res = 0;
	for (i = 2; i < argc; i++)
		res += strlen(argv[i]) + 1;
	if (res) {
		query = (char *)sqlite3_malloc(res);
		tmp = query;
		for (i = 2; i < argc; i++) {
			strcpy(tmp, argv[i]);
			tmp += strlen(tmp);
			*tmp = ' ';
			tmp++;
		}
		tmp[-1] = 0;
	}

	do {
		sqlite3_exec(db, "BEGIN", 0, 0, 0);
		ret = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
		sqlite3_exec(db, "COMMIT", 0, 0, 0);
		if (ret == SQLITE_OK)
			break;
		//		if (ret==SQLITE_BUSY) {
		// usleep(15000);
		retry++;

		if (retry > maxretry)
			break;
		//		sqlite3_prepare_v2(db, journal_mode_sql, -1,
		//&stmt, NULL);
	} while (ret != SQLITE_OK);

	if (ret == SQLITE_OK) {
		ret = sqlite3_step(stmt);

		// Handle the results
		while (ret == SQLITE_ROW) {
			sqlCB(stmt);
			ret = sqlite3_step(stmt);
		}
	}

	sqlite3_finalize(stmt);
	sqlite3_free(query);
	sqlite3_close(db);

	return 0;
}

int
sqlitecmdro(int argc, char **argv)
{
	sqlite3 *db;
	int res;
	int i;
	char *query;
	char *tmp;
	char *dbdir;
	char *dbfile;
	int ret = 0;
	sqlite3_stmt *stmt;
	char *cp;
	int maxretry = 50;
	int retry = 0;

	//	const char journal_mode_sql[] = "PRAGMA journal_mode = MEMORY;";
	//	const char journal_mode_sql[] = "PRAGMA journal_mode=DELETE;";

	if (argv[1][0] == '@') {
#ifndef WITH_DBI
		printf("External SQL not implemented, recompile WITH_DBI\n");
		return (1);
#else
		return (sqlcmd(argc, argv));
#endif
	}

	if ((cp = lookupvar("sqldelimer")) == NULL)
		delim = DEFSQLDELIMER;
	else
		delim = cp;

	if (argc < 3) {
		out1fmt("%s: format: %s <dbfile> <query>\n", nm(), nm());
		return 0;
	}

	if (argv[1][0] != '/') {
		// search file in dbdir
		dbdir = lookupvar("dbdir");
		i = strlen(dbdir) + strlen(argv[1]);
		dbfile = calloc(strlen(dbdir) + strlen(argv[1]) +
			strlen(DBPOSTFIX) + 1,
		    sizeof(char *));

		if (dbfile == NULL) {
			error("Out of memory!\n");
			return (1);
		}
		sprintf(dbfile, "%s/%s%s", dbdir, argv[1], DBPOSTFIX);
	} else {
		dbfile = calloc(strlen(argv[1]) + 1, sizeof(char *));
		sprintf(dbfile, "%s", argv[1]);
	}

	if (SQLITE_OK !=
	    (res = sqlite3_open_v2(dbfile, &db,
		 SQLITE_OPEN_READONLY | SQLITE_OPEN_SHAREDCACHE, NULL))) {
		out1fmt("%s: Can't open database file: %s\n", nm(), dbfile);
		free(dbfile);
		return 1;
	}
	free(dbfile);

	sqlite3_busy_timeout(db, CBSD_SQLITE_BUSY_TIMEOUT);

	res = 0;
	for (i = 2; i < argc; i++)
		res += strlen(argv[i]) + 1;

	if (res) {
		query = (char *)sqlite3_malloc(res);
		tmp = query;
		for (i = 2; i < argc; i++) {
			strcpy(tmp, argv[i]);
			tmp += strlen(tmp);
			*tmp = ' ';
			tmp++;
		}
		tmp[-1] = 0;
	}

	sql_exec(db, "PRAGMA mmap_size = 209715200;");

	do {
		ret = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
		if (ret == SQLITE_OK)
			break;
		//		if (ret==SQLITE_BUSY) {
		// usleep(15000);
		retry++;
		if (retry > maxretry)
			break;
		//		sqlite3_prepare_v2(db, journal_mode_sql, -1,
		//&stmt, NULL);

	} while (ret != SQLITE_OK);

	if (ret == SQLITE_OK) {
		ret = sqlite3_step(stmt);

		while (ret == SQLITE_ROW) {
			sqlCB(stmt);
			ret = sqlite3_step(stmt);
		}
	}

	sqlite3_finalize(stmt);
	sqlite3_free(query);
	sqlite3_close(db);

	return 0;
}

#ifndef IDLE_USE_REDIS

int
update_idlecmd(int argc, char **argv)
{
	char *str = NULL;
	char sql[] =
	    "UPDATE nodelist SET idle=datetime('now','localtime') WHERE nodename=''";

	if (argc != 2) {
		out1fmt("%d, usage: update_idle <nodename>\n", argc);
		return 0;
	}
	str = calloc(strlen(sql) + strlen(argv[1]) + 1, sizeof(char *));

	sprintf(str,
	    "UPDATE nodelist SET idle=datetime('now','localtime') WHERE nodename='%s'",
	    argv[1]);

	char *a[] = { NULL, "nodes", str };
	sqlitecmdrw(3, a);

	free(str);

	return 0;
}
#endif

#ifdef WITH_DBI

void
dbi_init()
{
	int rc;
	char *error;

	if ((databases->lib_handle = dlopen("/usr/local/lib/libdbi.so",
		 RTLD_LAZY)) == NULL) {
		fprintf(stderr, "libdbi is not installed!\n");
		return;
	}

#define REGLIB(name, func)                            \
	_##name = dlsym(databases->lib_handle, func); \
	if ((error = dlerror()) != NULL) {            \
		fprintf(stderr, "%s\n", error);       \
		dlclose(databases->lib_handle);       \
		databases->lib_handle = NULL;         \
		return;                               \
	}

	REGLIB(dbi_initialize, "dbi_initialize_r")
	REGLIB(dbi_shutdown, "dbi_shutdown_r")
	REGLIB(dbi_conn_error, "dbi_conn_error")
	REGLIB(dbi_conn_new, "dbi_conn_new_r")
	REGLIB(dbi_conn_set_option, "dbi_conn_set_option")
	REGLIB(dbi_conn_connect, "dbi_conn_connect")
	REGLIB(dbi_conn_close, "dbi_conn_close")
	REGLIB(dbi_conn_query, "dbi_conn_query")
	REGLIB(dbi_result_next_row, "dbi_result_next_row")
	REGLIB(dbi_result_free, "dbi_result_free")
	REGLIB(dbi_result_get_field_name, "dbi_result_get_field_name")
	REGLIB(dbi_result_get_as_string_copy_idx,
	    "dbi_result_get_as_string_copy_idx")
	REGLIB(dbi_result_get_numfields, "dbi_result_get_numfields")
	REGLIB(dbi_conn_error_handler, "dbi_conn_error_handler")
	REGLIB(dbi_set_verbosity, "dbi_set_verbosity_r")

#undef REGLIB

	if ((rc = _dbi_initialize(NULL, &databases->instance)) < 0) {
		fprintf(stderr,
		    "Problem initializing DBI, did you install the drivers?\n");
		dlclose(databases->lib_handle);
		databases->lib_handle = NULL;
		return;
	}

	_dbi_set_verbosity(0, databases->instance);
}

void
cbsd_dbi_init()
{
	//	int	rc;

	// Get some RAM...
	if ((databases = malloc(sizeof(cbsddbi_t))) == NULL)
		return;
	bzero(databases, sizeof(cbsddbi_t));

	// Try and load the library
	dbi_init();
}

void
_dbi_free_db(sql_database_t *item)
{
	if (!item)
		return;

	if (item->conn)
		_dbi_conn_close(item->conn);

		//	fprintf(stderr,"Removing %s\n",item->name);

#define FreeIF(what)    \
	if (item->what) \
		free(item->what);
	FreeIF(name) FreeIF(type) FreeIF(hostname) FreeIF(username)
	    FreeIF(password) FreeIF(database) FreeIF(encoding)
#undef FreeIF

		if (databases->list == item)
	{
		databases->list = item->next;
	}
	else
	{
		sql_database_t *seek = databases->list;
		while (seek && seek->next && seek->next != item)
			;
		if (seek && seek->next && seek->next == item)
			seek->next = item->next;
		else
			fprintf(stderr,
			    "WARNING: Could not find database item in chain!\n");
	}
	free(item);
}

void
cbsd_dbi_free()
{
	// Remove all items..
	while (databases->list)
		_dbi_free_db(databases->list);

	_dbi_shutdown(databases->instance);
	if (databases->lib_handle)
		dlclose(databases->lib_handle);

	free(databases);
}

#endif
