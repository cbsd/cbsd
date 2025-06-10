/* Part of the CBSD Project */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "sqlite3.h"

#include "sqlcli.h"

#define MAX_RETRY 40
#define BUSY_SLEEP_US 5000

char *
nm(void)
{
	return "sqlcli";
}

void
usage()
{
	printf("Tools for execute SQLite query from CLI\n");
	printf("%s: format: %s <dbfile> <query>\n", nm(), nm());
}

int
sqlCB(sqlite3_stmt *stmt)
{
	if (stmt == NULL) {
		return 1;
	}
	const char *delim = getenv("sqldelimer");
	if (!delim) delim = DEFSQLDELIMER;
	const char *sqlcolnames = getenv("sqlcolnames");
	int allcol = sqlite3_column_count(stmt);
	// Optionally print header if requested
	if (getenv("sqlprintheader") && !sqlcolnames) {
		for (int icol = 0; icol < allcol; icol++) {
			printf("%s%s", sqlite3_column_name(stmt, icol),
				   (icol != allcol - 1) ? delim : "\n");
		}
	}
	for (int icol = 0; icol < allcol; icol++) {
		const char *colval = (const char *)sqlite3_column_text(stmt, icol);
		if (sqlcolnames) {
			printf("%s=\"%s\"\n", sqlite3_column_name(stmt, icol), colval ? colval : "NULL");
		} else {
			printf("%s%s", colval ? colval : "NULL", (icol == allcol - 1) ? "\n" : delim);
		}
	}
	return 0;
}

int
main(int argc, char **argv)
{
	if (argc < 3) {
		usage();
		return EXIT_FAILURE;
	}
	// Calculate query length
	size_t query_len = 0;
	for (int i = 2; i < argc; i++) {
		query_len += strlen(argv[i]) + 1;
	}
	if (query_len == 0) {
		fprintf(stderr, "%s: Empty query string.\n", nm());
		return EXIT_FAILURE;
	}
	// Open database
	sqlite3 *db = NULL;
	int res = sqlite3_open(argv[1], &db);
	if (res != SQLITE_OK) {
		fprintf(stderr, "%s: Can't open database file: %s\nError: %s\n", nm(), argv[1], sqlite3_errmsg(db));
		if (db) sqlite3_close(db);
		return EXIT_FAILURE;
	}
	// Set PRAGMAs
	sqlite3_exec(db, "PRAGMA journal_mode = WAL;", NULL, 0, 0);
	sqlite3_exec(db, "PRAGMA synchronous = NORMAL;", NULL, 0, 0);
	sqlite3_db_config(db, SQLITE_DBCONFIG_DQS_DDL, 1, (void*)0);
	sqlite3_db_config(db, SQLITE_DBCONFIG_DQS_DML, 1, (void*)0);
	// Build query string
	char *query = (char *)sqlite3_malloc(query_len);
	if (!query) {
		fprintf(stderr, "%s: Memory allocation failed.\n", nm());
		sqlite3_close(db);
		return EXIT_FAILURE;
	}
	char *tmp = query;
	for (int i = 2; i < argc; i++) {
		size_t len = strlen(argv[i]);
		memcpy(tmp, argv[i], len);
		tmp += len;
		*tmp = ' ';
		tmp++;
	}
	tmp[-1] = '\0';
	// Prepare statement with retry on SQLITE_BUSY
	sqlite3_stmt *stmt = NULL;
	int retry = 0;
	int ret;
	do {
		sqlite3_exec(db, "BEGIN", 0, 0, 0);
		ret = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
		sqlite3_exec(db, "COMMIT", 0, 0, 0);
		if (ret == SQLITE_OK) break;
		if (ret == SQLITE_BUSY) {
			usleep(BUSY_SLEEP_US);
		}
		retry++;
	} while (ret == SQLITE_BUSY && retry <= MAX_RETRY);
	if (ret != SQLITE_OK) {
		fprintf(stderr, "%s: Failed to prepare statement. SQLite error: %s [%s]\n", nm(), sqlite3_errmsg(db), query);
		sqlite3_free(query);
		sqlite3_close(db);
		return EXIT_FAILURE;
	}
	// Execute and print results
	ret = sqlite3_step(stmt);
	while (ret == SQLITE_ROW) {
		sqlCB(stmt);
		ret = sqlite3_step(stmt);
	}
	if (ret != SQLITE_DONE) {
		fprintf(stderr, "%s: SQLite error during execution: %s\n", nm(), sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
	sqlite3_free(query);
	sqlite3_close(db);
	return EXIT_SUCCESS;
}
