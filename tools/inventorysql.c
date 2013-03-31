#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"

#include <getopt.h>
#include <stdarg.h> //for debugmsg/errmsg

#include "inventorysql.h"


int debugmsg(int level,const char *format, ...)
{
va_list arg;
int done;

    if(debug<level) return 0;
    va_start (arg, format);
    done = vfprintf (stdout, format, arg);
    va_end (arg);

return 0;
}

int errmsg(const char *format, ...)
{
   va_list arg;
   int done;

   va_start (arg, format);
   done = vfprintf (stderr, format, arg);
   va_end (arg);

   return 0;
}

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

int sql_stmt(const char* stmt) {
char *sqlerr;
int   ret;

    ret = sqlite3_exec(db, stmt, 0, 0, &sqlerr);

    if(ret != SQLITE_OK) {
	errmsg("Error in statement: %s [%s].\n", stmt, sqlerr);
    }
    return ret;
}

void update_inventory(char *column, char *value) {
char buf[SQLSTRLEN];
    sql_stmt("begin");
    bzero(buf,SQLSTRLEN);
    sprintf(buf,"update local set %s = \"%s\"",column,value);
    debugmsg(1,"SQL: %s\n",buf);
    sql_stmt(buf);
    sql_stmt("commit");
}

void delete_inventory(char *column) {
char buf[SQLSTRLEN];

    bzero(buf,SQLSTRLEN);
    sprintf(buf,"delete %s from local",column);
    debugmsg(1,"SQL: %s\n",buf);
    sql_stmt(buf);
    sql_stmt("commit");
}

void usage() {
    printf("Tools for update or view cbsd inventory in sqlite database\n");
    printf("require: dbfile\n");
    printf("opt: action workdir param value\n");
    printf("dbfile - sqlite database file\n");
    printf("action can be:\n");
    printf("- init - for re-create database (drop and create empty table)\n");
    printf("- list - select all records from database\n");
    printf("- update ( param=XXXX value=XXXX ) for insert or update param\n");
//    printf("- delete ( param=XXXX ) for remove records from registry\n");
    printf("\n");
}


int main(int argc, char **argv)
{
int win = FALSE;
int optcode = 0;
int option_index = 0, ret = 0;
int action=0;
int i=0;

static struct option long_options[] = {
    { "dbfile", required_argument, 0 , C_DBFILE },
    { "action", required_argument, 0 , C_ACTION },
    { "help", no_argument, 0, C_HELP },
    { "debug", required_argument, 0, C_DEBUG },
    { "workdir", required_argument, 0, C_WORKDIR },
    { "param", required_argument, 0, C_PARAM },
    { "value", required_argument, 0, C_VALUE },
    /* End of options marker */
    { 0, 0, 0, 0 }
    };

    while (TRUE) {
	optcode = getopt_long_only(argc, argv, "", long_options, &option_index);
        if (optcode == -1) break;
        switch (optcode) {
            case C_DBFILE:
            	dbfile=optarg;
            break;
            case C_ACTION:
                i=0;
                while (actionlist[++i])
                    if (!strcmp(optarg,actionlist[i])) {
                        action=i;
                    break;
                    }
            break;
            case C_HELP:      /* usage() */
                usage();
                exit(0);
            break;
            case C_DEBUG:      /* debuglevel 0-2 */
                debug=atoi(optarg);
            break;
            case C_PARAM:
                param=optarg;
            break;
            case C_VALUE:
                value=optarg;
            break;

        } //case
    } //while

    if (argc<2) exit(0);

    if (dbfile==NULL) {
        errmsg("Please set --dbfile=\n");
        exit(1);
    }

    if (action==0) {
        errmsg("Please set correct --action\n");
        exit(1);
    }

    debugmsg(2,"Action choosed: %d\n",action);
    sqlite3_open(dbfile, &db);

    if(db == 0) {
        errmsg("Could not open database %s\n",dbfile);
        return 1;
    }

    switch (action) {
	case (INIT):
	    ret=sql_stmt("drop table if exists local");
	    ret=sql_stmt("create table local (nodename text default null, nodeip text default  null, \
	    nodeloc text default null, jnameserver text default null, nodeippool text default null, natip text default null, nat_enable text default null,\
	    fbsdrepo boolean default 1, mdtmp integer default 0, repo text default null, workdir text default null, ipfw boolean default 0, \
	    nat boolean default 0, fs text default null, zfsfeat boolean default 0, jail_interface text default null, ncpu integer default 0, physmem integer default 0, disks text default null)"); // ON CONFLICT FAIL");
    	    if (ret==0) ret=sql_stmt("insert into local ( nodename ) values ( 'null' )");
	    if(ret==0) debugmsg(1,"DB initialization successfull\n");
                else errmsg("DB init failed\n");
            goto closeexit;
        break;
	case (LIST):
            ret=select_stmt("select * from local");
            goto closeexit;
        break;
	case (UPDATE):
            if (!param||!value) {
                errmsg("required arguments: --param, --value\n");
                ret=1;
                goto closeexit;
            }
            update_inventory(param,value);
            goto closeexit;
        break;
	case (DELETE):
            if (!param) {
                errmsg("required arguments: --param\n");
                ret=1;
                goto closeexit;
            }
            delete_inventory(param);
            goto closeexit;
        break;
    } //switch

/*
    else if (!strcmp(cmd,"all")) {
	ret=select_stmt("select * from local");
	goto closeexit;
    }
    else if (!strcmp(cmd,"update")) {
	if (argc!=5) {
	    printf("Usage: %s dbfile update column \"values\"\n",argv[0]);
	    goto closeexit;
	}
	update_inventory(argv[3],argv[4]);
	goto closeexit;
    } //update
    else {
	char *column;
	column=(char*)malloc(sizeof(char) * strlen(argv[2]));
	strcpy(column,argv[2]);
	char buf[SQLSTRLEN];
	bzero(buf,SQLSTRLEN);
	sprintf(buf,"select %s from local",column);
	ret=select_stmt(buf);
	free(column);
	goto closeexit;
    }
*/

closeexit:
    sqlite3_close(db);
    return ret;
}
