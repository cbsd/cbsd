#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"

#include <getopt.h>
#include <stdarg.h> //for debugmsg/errmsg

#include "gentools.h"
#include "sqlhelper.h"

int select_callback(void *p_data, int num_fields, char **p_fields, char **p_col_names) {
    int field;
    int i;
    int *p_rn = (int*)p_data;

    if (first_row) {
	first_row = 0;
	for(field=0; field < num_fields; field++) {
	    printf("%s=\"%s\"\n", p_col_names[field],p_fields[field]);
	}
    }
    (*p_rn)++;

    return 0;
}

int select_valcallback(void *p_data, int num_fields, char **p_fields, char **p_col_names) {
    int field;
    int i;
    int *p_rn = (int*)p_data;

    if (first_row) {
	first_row = 0;
	for(field=0; field < num_fields; field++) {
	    printf("%s\n",p_fields[field]);
	}
    }
    (*p_rn)++;

    return 0;
}


int select_stmt(const char* stmt) {
    char *sqlerr;
    int   ret;
    int   nrecs = 0;
    first_row = 1;
    ret = sqlite3_exec(db, stmt, select_callback, &nrecs, &sqlerr);

    if(ret!=SQLITE_OK) {
	errmsg("Error in select statement %s [%s].\n", stmt, sqlerr);
    }
    return ret;
}

int select_valstmt(const char* stmt) {
    char *sqlerr;
    int   ret;
    int   nrecs = 0;
    first_row = 1;
    ret = sqlite3_exec(db, stmt, select_valcallback, &nrecs, &sqlerr);

    if(ret!=SQLITE_OK) {
	errmsg("Error in select statement %s [%s].\n", stmt, sqlerr);
    }
    return ret;
}




int sql_stmt(const char* stmt) {
char *sqlerr;
int   ret;

    ret = sqlite3_exec(db, stmt, 0, 0, &sqlerr);

    if(ret != SQLITE_OK) {
	errmsg("Error in statement: %s [%s].\n", stmt, sqlerr);
    }
    return ret;
}

