#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

#include "shell.h"
#include "main.h"
#include "nodes.h" /* for other headers */
#include "eval.h"
#include "jobs.h"
#include "show.h"
#include "options.h"
#include "syntax.h"
#include "output.h"
#include "memalloc.h"
#include "error.h"
#include "trap.h"
#include "mystring.h"
#include "builtins.h"
#include "myhistedit.h"
#include "var.h"
#include <stdbool.h>

#include "sqlcmd.h"

#include <jv.h>

#define CBSD_SQLITE_BUSY_TIMEOUT 25000

char *delim;

char *
nm(void)
{
	return "cbsdsql";
}

static sqlite3 *
sql_open(const char *dbarg, int open_flags)
{
	char *dbdir;
	const char *dbfile = dbarg;
	char *dbfilebuf = NULL;
	sqlite3 *db = NULL;

	if (dbarg == NULL)
		return NULL;

	if (dbarg[0] != '/') {
		dbdir = lookupvar("dbdir");
		if (!dbdir) {
			out1fmt("dbdir not set!\n");
			return NULL;
		}
		size_t fnlen = strlen(dbdir) + strlen(dbarg) + strlen(DBPOSTFIX) + 2;
		dbfilebuf = calloc(fnlen, sizeof(char));
		if (dbfilebuf == NULL) {
			out1fmt("Out of memory!\n");
			return NULL;
		}
		if (snprintf(dbfilebuf, fnlen, "%s/%s%s", dbdir, dbarg, DBPOSTFIX) < 0) {
			out1fmt("Failed to compose database file name!\n");
			free(dbfilebuf);
			return NULL;
		}
		dbfile = dbfilebuf;
	}

	if (SQLITE_OK != sqlite3_open_v2(dbfile, &db, open_flags, NULL)) {
		out1fmt("%s: Can't open database file: %s\n", nm(), dbfile);

		if (db)
			sqlite3_close(db);

		db = NULL;
	}

	if (dbfilebuf)
		free(dbfilebuf);

	return db;
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


// Helper function to build SQL query from argv
static char *build_query(int argc, char **argv, int start) {
	size_t len = 0;
	for (int i = start; i < argc; i++)
		len += strlen(argv[i]) + 1;
	if (len == 0)
		return NULL;
	char *query = malloc(len);
	if (!query)
		return NULL;
	char *tmp = query;
	for (int i = start; i < argc; i++) {
		strcpy(tmp, argv[i]);
		tmp += strlen(tmp);
		*tmp = ' ';
		tmp++;
	}
	tmp[-1] = 0;
	return query;
}

int
sqlitecmdrw(int argc, char **argv)
{
	sqlite3 *db;
	char *query = NULL;
	int ret = 0;
	sqlite3_stmt *stmt = NULL;
	char *cp;
	int maxretry = 50;
	int retry = 0;

	if (argc < 3) {
		out1fmt("%s: format: %s <dbfile> <query>\n", nm(), nm());
		return 1;
	}

	if ((cp = lookupvar("sqldelimer")) == NULL)
		delim = DEFSQLDELIMER;
	else
		delim = cp;
	db = sql_open(argv[1], 
		SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_SHAREDCACHE);
	if (db == NULL)
		return 1;

	sql_exec(db, "PRAGMA mmap_size = 209715200;");
	sqlite3_busy_timeout(db, CBSD_SQLITE_BUSY_TIMEOUT);
	sql_exec(db, "PRAGMA journal_mode = WAL;");
	sql_exec(db, "PRAGMA synchronous = NORMAL;");

	sqlite3_db_config(db, SQLITE_DBCONFIG_DQS_DDL, 1, (void*)0);
	sqlite3_db_config(db, SQLITE_DBCONFIG_DQS_DML, 1, (void*)0);

	query = build_query(argc, argv, 2);
	if (!query) {
		sqlite3_close(db);
		out1fmt("Failed to build query string!\n");
		return 1;
	}

	do {
		sqlite3_exec(db, "BEGIN", 0, 0, 0);
		ret = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
		sqlite3_exec(db, "COMMIT", 0, 0, 0);
		if (ret == SQLITE_OK)
			break;
		retry++;
		if (retry > maxretry)
			break;
	} while (ret != SQLITE_OK);

	if (ret == SQLITE_OK) {
		ret = sqlite3_step(stmt);
		while (ret == SQLITE_ROW) {
			sqlCB(stmt);
			ret = sqlite3_step(stmt);
		}
	}

	if (stmt)
		sqlite3_finalize(stmt);
	free(query);
	sqlite3_close(db);

	return 0;
}

int
sqlitecmdro(int argc, char **argv)
{
	sqlite3 *db;
	char *query = NULL;
	int ret = 0;
	sqlite3_stmt *stmt = NULL;
	char *cp;
	int maxretry = 50;
	int retry = 0;

	if ((cp = lookupvar("sqldelimer")) == NULL)
		delim = DEFSQLDELIMER;
	else
		delim = cp;

	if (argc < 3) {
		out1fmt("%s: format: %s <dbfile> <query>\n", nm(), nm());
		return 0;
	}
	db = sql_open(argv[1],
	    SQLITE_OPEN_READONLY | SQLITE_OPEN_SHAREDCACHE);
	if (db == NULL)
		return 1;

	sqlite3_busy_timeout(db, CBSD_SQLITE_BUSY_TIMEOUT);

	query = build_query(argc, argv, 2);
	if (!query) {
		sqlite3_close(db);
		out1fmt("Failed to build query string!\n");
		return 1;
	}

	sql_exec(db, "PRAGMA mmap_size = 209715200;");

	sqlite3_db_config(db, SQLITE_DBCONFIG_DQS_DDL, 1, (void*)0);
	sqlite3_db_config(db, SQLITE_DBCONFIG_DQS_DML, 1, (void*)0);

	do {
		ret = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
		if (ret == SQLITE_OK)
			break;
		retry++;
		if (retry > maxretry)
			break;
	} while (ret != SQLITE_OK);

	if (ret == SQLITE_OK) {
		ret = sqlite3_step(stmt);
		while (ret == SQLITE_ROW) {
			sqlCB(stmt);
			ret = sqlite3_step(stmt);
		}
	}

	if (stmt)
		sqlite3_finalize(stmt);
	free(query);
	sqlite3_close(db);

	return 0;
}

/*
 * sqlitecmdquery:
 *   Read-only query against dbfile and print one JSON per row to stdout.
 *
 * Usage (in shell):
 *   cbsdsqlquery <dbfile> "<query>"
 */
int
sqlitecmdquery(int argc, char **argv)
{
	sqlite3 *db;
	int ret = 0;
	sqlite3_stmt *stmt = NULL;
	int maxretry = 50;
	int retry = 0;
	int limit = -1;
	int rowcount = 0;
	const char *dbarg = NULL;
	const char *query = NULL;
	int arg_error = 0;

	for (int i = 1; i < argc; i++) {
		const char *arg = argv[i];
		if (arg[0] == '-' && arg[1] == 'n') {
			if (arg[2] != '\0') {		// -n1
				limit = atoi(arg + 2);
			} else if (i + 1 < argc) {	// -n 1
				limit = atoi(argv[++i]);
			} else {
				arg_error = 1;
				break;
			}
			continue;
		}
		if (dbarg == NULL) {
			dbarg = arg;
		} else if (query == NULL) {
			query = arg;
		} else {
			arg_error = 1;
			break;
		}
	}

	if (dbarg == NULL || query == NULL || arg_error) {
		out1fmt("usage: cbsdsqlquery [-n num] <dbfile> <query>\n");
		return 1;
	}

	db = sql_open(dbarg,
	    SQLITE_OPEN_READONLY | SQLITE_OPEN_SHAREDCACHE);
	if (db == NULL)
		return 1;

	sqlite3_busy_timeout(db, CBSD_SQLITE_BUSY_TIMEOUT);
	sql_exec(db, "PRAGMA mmap_size = 209715200;");
	sqlite3_db_config(db, SQLITE_DBCONFIG_DQS_DDL, 1, (void *)0);
	sqlite3_db_config(db, SQLITE_DBCONFIG_DQS_DML, 1, (void *)0);

	do {
		ret = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
		if (ret == SQLITE_OK)
			break;
		retry++;
		if (retry > maxretry)
			break;
	} while (ret != SQLITE_OK);

	if (ret == SQLITE_OK) {
		ret = sqlite3_step(stmt);
		while (ret == SQLITE_ROW) {
			int icol;
			int allcol = sqlite3_column_count(stmt);
			jv row = jv_object();

			for (icol = 0; icol < allcol; icol++) {
				const char *colname =
				    sqlite3_column_name(stmt, icol);
				switch (sqlite3_column_type(stmt, icol)) {
				case SQLITE_INTEGER:
					row = jv_object_set(row,
					    jv_string(colname),
					    jv_number(
						(double)sqlite3_column_int64(
						    stmt, icol)));
					break;
				case SQLITE_FLOAT:
					row = jv_object_set(row,
					    jv_string(colname),
					    jv_number(
						sqlite3_column_double(stmt,
						    icol)));
					break;
				case SQLITE_TEXT: {
					const unsigned char *txt_uc =
					    sqlite3_column_text(stmt, icol);
					const char *txt = txt_uc
					    ? (const char *)txt_uc
					    : "";
					row = jv_object_set(row,
					    jv_string(colname),
					    jv_string(txt));
					break;
				}
				case SQLITE_NULL:
					row = jv_object_set(row,
					    jv_string(colname), jv_null());
					break;
				case SQLITE_BLOB:
				default:
					row = jv_object_set(row,
					    jv_string(colname), jv_null());
					break;
				}
			}
			jv dumped = jv_dump_string(row, 0);
			if (!jv_is_valid(dumped)) {
				jv msg = jv_invalid_get_msg(jv_copy(dumped));
				out1fmt("%s\n", jv_string_value(msg));
				jv_free(msg);
				jv_free(dumped);
				jv_free(row);
				ret = 1;
				break;
			}
			out1fmt("%s\n", jv_string_value(dumped));
			jv_free(dumped);
			jv_free(row);
			rowcount++;
			if (limit >= 0 && rowcount >= limit) {
				ret = SQLITE_DONE;
				break;
			}
			ret = sqlite3_step(stmt);
		}
	}

	if (stmt)
		sqlite3_finalize(stmt);
	sqlite3_close(db);

	return 0;
}

/*
 * sqlitecmdro_vars:
 *   Read-only query like sqlitecmdro, but instead of printing result,
 *   assign columns into shell variables. If the query returns multiple
 *   rows, values from each column are concatenated into the corresponding
 *   variable separated by '\n'.
 *
 * Usage (in shell):
 *   cbsdsqlro_vars <dbfile> <query> [ var1 ...]
 *
 *  Set of returned variables - matches columns selected ("*" -> all table columns)
 *  You may specify variables names explicitly as argv or let them to be named after columns
 *  If the number of specified variables is less than the number of columns in the query,
 *  column names will be used for rest of variables.
 *
 * Example:
 *   cbsdsqlro_vars "${_sqlfile}" "SELECT mnt_start,mnt_stop FROM bhyve WHERE jname='$1'"
 *   cbsdsqlro_vars "${_sqlfile}" "SELECT jid,jname FROM bhyve WHERE jname='$1'" myjid myjname
 */
int
sqlitecmdro_vars(int argc, char **argv)
{
	sqlite3 *db;
	int ret = 0;
	sqlite3_stmt *stmt = NULL;
	int maxretry = 50;
	int retry = 0;
	int nvars;
	int i;
	int got_row = 0;

	/* Need at least: dbfile query var1 */
	if (argc < 3) {
		out1fmt("usage: cbsdsqlro_vars <dbfile> <query> [ var1 [var2 ...]]\n");
		return 1;
	}

	nvars = argc - 3;
	db = sql_open(argv[1],
	    SQLITE_OPEN_READONLY | SQLITE_OPEN_SHAREDCACHE);
	if (db == NULL)
		return 1;

	sqlite3_busy_timeout(db, CBSD_SQLITE_BUSY_TIMEOUT);
	sql_exec(db, "PRAGMA mmap_size = 209715200;");
	sqlite3_db_config(db, SQLITE_DBCONFIG_DQS_DDL, 1, (void *)0);
	sqlite3_db_config(db, SQLITE_DBCONFIG_DQS_DML, 1, (void *)0);

	/* argv[2] = query (single string) */
	do {
		ret = sqlite3_prepare_v2(db, argv[2], -1, &stmt, NULL);
		if (ret == SQLITE_OK)
			break;
		retry++;
		if (retry > maxretry)
			break;
	} while (ret != SQLITE_OK);

	if (ret == SQLITE_OK) {
		/* init argv vars with empty values */
		for (i = 0; i < nvars; i++)
			setvar(argv[3 + i], "", 0);

		bool cleanup_vars_finished = false;

		ret = sqlite3_step(stmt);
		while (ret == SQLITE_ROW) {
			int j;
			int ncols = sqlite3_column_count(stmt);

			if (!cleanup_vars_finished) {
				// if more columns returned than variables passed, clean up rest of variables
				for (j = nvars; j < ncols; j++) {
					setvar(sqlite3_column_name(stmt, j), "", 0);
				}
				cleanup_vars_finished = true;
			}

			got_row = 1;

			for (j = 0; j < ncols; j++) {
				const char *colname = sqlite3_column_name(stmt, j);
				const unsigned char *txt_uc = sqlite3_column_text(stmt, j);
				const char *txt = txt_uc ? (const char *)txt_uc : "";
				const char *varname = (j < nvars) ? argv[3 + j] : colname;
				const char *old = lookupvar(varname);

				/* Append \n when data exist */
				if (old && *old) {
					size_t oldlen = strlen(old);
					size_t slen = strlen(txt);
					char *nv =
					    malloc(oldlen + 1 + slen + 1);
					if (!nv) {
						goto cleanup;
					}
					memcpy(nv, old, oldlen);
					nv[oldlen] = '\n';
					memcpy(nv + oldlen + 1, txt, slen);
					nv[oldlen + 1 + slen] = '\0';
					setvar(varname, nv, 0);
					free(nv);
				} else {
					setvar(varname, txt, 0);
				}
			}

			ret = sqlite3_step(stmt);
		}
	}

cleanup:
	if (stmt)
		sqlite3_finalize(stmt);
	sqlite3_close(db);

	/* If no row returned, set all requested variables to empty */
	if (!got_row) {
		for (i = 0; i < nvars; i++)
			setvar(argv[3 + i], "", 0);
		return 1;
	}
	return 0;
}

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
