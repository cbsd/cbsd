/* Part of the CBSD Project */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "sqlite3.h"

#include "sqlcli.h"

#define SQLITE_BUSY_TIMEOUT 5000

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
sqlCB(sqlite3_stmt * stmt)
{
	int		icol, irow;
	const char	*colname;
	int		allcol;
	char		*delim;
	char		*cp;
	int		printheader = 0;
	char		*sqlcolnames = NULL;
	int		ret = 0;

	if (stmt == NULL)
		return 1;

	if ((cp = getenv("sqldelimer")) == NULL)
		delim = DEFSQLDELIMER;
	else
		delim = cp;

	sqlcolnames = getenv("sqlcolnames");
	allcol = sqlite3_column_count(stmt);

	if ((printheader) && (sqlcolnames == NULL)) {
		for (icol = 0; icol < allcol; icol++) {
			colname = sqlite3_column_name(stmt, icol);
			if (icol != (allcol - 1))
				printf("%s%s", colname, delim);
			else
				printf("%s\n", colname);
		}
	}
	for (icol = 0; icol < allcol; icol++) {
		if (sqlcolnames)
			printf("%s=\"%s\"\n", sqlite3_column_name(stmt, icol), sqlite3_column_text(stmt, icol));
		else {
			if (icol == (allcol - 1))
				printf("%s\n", sqlite3_column_text(stmt, icol));
			else
				printf("%s%s", sqlite3_column_text(stmt, icol), delim);
		}
	}

	return 0;

}

int
main(int argc, char **argv)
{
	sqlite3        *db;
	int		res;
	int		i;
	char           *query;
	char           *tmp;
	char           *err = NULL;
	int		maxretry = 10;
	int		retry = 0;
	sqlite3_stmt   *stmt;
	int		ret;

	if (argc < 3) {
		usage();
		return 0;
	}
	res = 0;
	for (i = 2; i < argc; i++)
		res += strlen(argv[i]) + 1;

	if (!res)
		return 1;

	if (SQLITE_OK != (res = sqlite3_open(argv[1], &db))) {
		printf("%s: Can't open database file: %s\n", nm(), argv[1]);
		return 1;
	}
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

	ret = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);

	if (ret == SQLITE_OK) {
		ret = sqlite3_step(stmt);

		while ( ret == SQLITE_ROW ) {
			sqlCB(stmt);
			ret = sqlite3_step(stmt);
		}
	}

	sqlite3_finalize(stmt);
	sqlite3_free(query);
	sqlite3_close(db);

	return 0;
}
