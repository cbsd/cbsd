#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "shell.h"
#include "main.h"
#include "nodes.h"      /* for other headers */
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
#include "sqlite3.h"

#include "sqlcmd.h"

char *
nm(void)
{
	return "cbsdsql";
}

int
sqlCB(void *none, int rows, char **rowV, char **rowN)
{
	int	 i;
	int	*cnt = (int *)none;
	char *delim;
	int printheader=0;

	delim=lookupvar("sqldelimer");

	if ( delim == NULL ) delim=DEFSQLDELIMER;

	if (printheader) {
		if (!(*cnt)) {
		    for (i = 0; i < rows; i++)
			if (i<rows-1)
			    out1fmt("%s%s",rowN[i],delim);
			else
			    out1fmt("%s\n",rowN[i]);
		}
	}
	(*cnt)++;
	for (i = 0; i < rows; i++)
		if (i<rows-1)
		    out1fmt("%s%s",rowV[i],delim);
		else
		    out1fmt("%s\n", rowV[i]);
	return 0;
}

int
sqlitecmd(int argc, char **argv)
{
	sqlite3	*db;
	int	 res;
	int	 i;
	char	*query;
	char	*tmp;
	char	*err;
	char *dbdir;
	char *dbfile;

	if (argc<3) { 
	    out1fmt("%s: format: %s <dbfile> <query>\n",nm(),nm());
	    return 0;
	}

	dbdir = lookupvar("dbdir");
	i=strlen(dbdir)+strlen(argv[1]);
	dbfile = calloc(strlen(dbdir)+strlen(argv[1])+strlen(DBPOSTFIX)+1, sizeof(char *));

	if (dbfile == NULL) {
	    error("Out of memory!\n");
            return (1);
	}

	sprintf(dbfile,"%s/%s%s",dbdir,argv[1],DBPOSTFIX);

	if (SQLITE_OK != (res = sqlite3_open(dbfile, &db))) {
		out1fmt("%s: Can't open database file: %s\n", nm(), dbfile);
		free(dbfile);
		return 1;
	}

	free(dbfile);

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
		err = 0;
		i = 0;
		sqlite3_exec(db, query, sqlCB, (void *)&i, &err);
		if (err) {
			out1fmt("%s: sqlite_error: %s\n", nm(), err);
			sqlite3_free(err);
			sqlite3_free(query);
			sqlite3_close(db);
			return 1;
		}
		sqlite3_free(query);
	}
	sqlite3_close(db);
	return 0;
}
