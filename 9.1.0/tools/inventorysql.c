#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"

// SQL string Maxlen
#define SQLSTRLEN 4096
sqlite3* db;
int first_row;


int select_callback(void *p_data, int num_fields, char **p_fields, char **p_col_names) {
    int field;
    int i;
    int *p_rn = (int*)p_data;

    if (first_row) {
	first_row = 0;
	// This is the first row, so we print
	// a header with 
	//  
	//   The column names ....
	for(field=0; field < num_fields; field++) {
    	    printf("%s=\"%s\"\n", p_col_names[field],p_fields[field]);
	}
//	printf("\n");

	//  ... and underline these column names:
//	for(field=0; field<num_fields; field++) {
//	    printf("  ");
//	    for (i=0; i<20; i++) {
//    		printf("=");
//    	    }
//	}
//	printf("\n");
    }

    (*p_rn)++;

//    for(i=0; i < num_fields; i++) {
//	printf("  %20s", p_fields[i]);
//    }

//    printf("\n");
    return 0;
}

int select_stmt(const char* stmt) {
    char *errmsg;
    int   ret;
    int   nrecs = 0;
    first_row = 1;
    ret = sqlite3_exec(db, stmt, select_callback, &nrecs, &errmsg);

    if(ret!=SQLITE_OK) {
	printf("Error in select statement %s [%s].\n", stmt, errmsg);
    }
    return ret;
}

int sql_stmt(const char* stmt) {
    char *errmsg;
    int   ret;
    ret = sqlite3_exec(db, stmt, 0, 0, &errmsg);

    if(ret != SQLITE_OK) {
	printf("Error in statement: %s [%s].\n", stmt, errmsg);
    }
    return ret;
}

void update_inventory(char *column, char *value) {
    char buf[SQLSTRLEN];
    sql_stmt("begin");
    bzero(buf,SQLSTRLEN);
    sprintf(buf,"update local set %s = \"%s\"",column,value);
    sql_stmt(buf);
    sql_stmt("commit");
}

int main(int argc, char **argv)
{
    char* dbfile;
    char* cmd;
    int ret=0;

    if (argc<3) {
	printf("Usage %s dbfile [cmd or column]\n",argv[0]);
	return 1;
    }

    dbfile = (char*)malloc(sizeof(char) * strlen(argv[1]));
    cmd = (char*)malloc(sizeof(char) * strlen(argv[2]));

    strcpy(dbfile,argv[1]);
    sqlite3_open(dbfile, &db);

    if(db == 0) {
	printf("Could not open database.");
	free(cmd);
	free(dbfile);
	return 1;
    }

    strcpy(cmd,argv[2]);

    if (!strcmp(cmd,"init")) {
	ret=sql_stmt("drop table if exists local");
	ret=sql_stmt("create table local (nodename text default null, nodeip text default  null, \
	nodeloc text default null, jnameserver text default null, nodeippool text default null, natip text default null, nat_enable text default null,\
	fbsdrepo boolean default 1, mdtmp integer default 0, repo text default null, workdir text default null, ipfw boolean default 0, \
	nat boolean default 0, fs text default null, zfsfeat boolean default 0, jail_interface text default null, ncpu integer default 0, physmem integer default 0, disks text default null)"); // ON CONFLICT FAIL");
	if (ret==0) ret=sql_stmt("insert into local ( nodename ) values ( 'null' )");
	goto closeexit;
    }

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

closeexit:
    sqlite3_close(db);
    free(cmd);
    free(dbfile);
    return ret;
}
