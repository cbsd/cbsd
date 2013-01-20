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
//	for(field=0; field < num_fields; field++) {
//    	    printf("%s ", p_col_names[field]);
//	}
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

    for(i=0; i < num_fields; i++) {
	printf("%s", p_fields[i]);
    }
    printf("\n");
    return 0;
}


int select_callbacksshopt(void *p_data, int num_fields, char **p_fields, char **p_col_names) {
    int field;
    int i;
    int *p_rn = (int*)p_data;
    char buf[SQLSTRLEN];

	first_row = 0;

    (*p_rn)++;

    bzero(buf,SQLSTRLEN);
//    strcpy(buf,"-t -oBatchMode=yes -oPort=${PORT} -oStrictHostKeyChecking=no -oConnectTimeout=5 -F ${sshdir}/config -q -i ${SSHKEY} ${cbsduser}@${IP}");
    strcpy(buf,"-t -oBatchMode=yes -oStrictHostKeyChecking=no -oConnectTimeout=5 -q");

    for(i=0; i < num_fields; i++) {
//	printf("SSH %s", p_fields[i]);
	switch (i) {
//	    case 0:strcat(buf," -i ");strcat(buf,p_fields[i]);break;
	    case 2:strcat(buf," -i ");strcat(buf,p_fields[i]);break;
	    case 1:strcat(buf," -oPort=");strcat(buf,p_fields[i]);break;
	}
    }

    strcat(buf," ");
    strcat(buf,p_fields[0]);
    printf("%s",buf);
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
    else {
//	printf("\n    %d records returned.\n", nrecs);
    }
    return ret;
}


int select_stmtsshopt(const char* nodename) {
char *errmsg;
int   ret;
int   nrecs = 0;
first_row = 1;
char buf[SQLSTRLEN];

    bzero(buf,SQLSTRLEN);
    sprintf(buf,"select ip,port,keyfile from nodelist where nodename=\"%s\" and status=\"0\"",nodename);

    ret = sqlite3_exec(db, buf, select_callbacksshopt, &nrecs, &errmsg);

    if(ret!=SQLITE_OK) {
	printf("Error in select statement %s [%s].\n", buf, errmsg);
    }

    sql_stmt(buf);
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

void delete_nodes(char *nodename) {
    char buf[SQLSTRLEN];
    bzero(buf,SQLSTRLEN);
    sprintf(buf,"delete from nodelist where nodename=\"%s\"",nodename);
    sql_stmt(buf);
}

void insert_nodes(char *nodename, char *ip, int port, char *keyfile) {
    char buf[SQLSTRLEN];
//    bzero(buf,SQLSTRLEN);
//    sprintf(buf,"delete from nodelist where nodename='%s'",nodename);
    sql_stmt("begin");
//    sql_stmt(buf);
//    delete_nodes(nodename);
    bzero(buf,SQLSTRLEN);
    sprintf(buf,"insert into nodelist ( nodename, ip, port, keyfile, status ) values ('%s','%s',%d,'%s',0)",nodename,ip,port,keyfile);
    sql_stmt(buf);
    sql_stmt("commit");
}




int main(int argc, char **argv)
{
//    char* dbfile;
//    char* cmd;
    int ret=0;

    if (argc<3) {
	printf("Usage %s dbfile cmd\n",argv[0]);
	return 1;
    }

//    dbfile = (char*)malloc(sizeof(char) * strlen(argv[1]));
//    cmd = (char*)malloc(sizeof(char) * strlen(argv[2]));

//    strcpy(dbfile,argv[1]);
    sqlite3_open(argv[1], &db);

    if(db == 0) {
	printf("Could not open database.");
//	free(cmd);
//	free(dbfile);
	return 1;
    }

//    strcpy(cmd,argv[2]);

    if (!strcmp(argv[2],"init")) {
	ret=sql_stmt("drop table if exists nodelist");
	ret=sql_stmt("create table nodelist (id integer primary key, nodename text not null unique , ip text not null, port integer, keyfile text not null, status integer)"); // ON CONFLICT FAIL");
	goto closeexit;
    }

    else if (!strcmp(argv[2],"list")) {
	ret=select_stmt("select nodename from nodelist");
	goto closeexit;
    }
    else if (!strcmp(argv[2],"insert")) {
	if (argc!=7) {
	    printf("Usage: %s workdir insert nodename ip port keyfile\n",argv[0]);
	    goto closeexit;
	}

	int port=0;
//	char* nodename;
//	char* keyfile;
//	char* ip;
//	nodename=(char *)malloc(sizeof(char) * strlen(argv[3]));
//	ip=(char *)malloc(sizeof(char) * strlen(argv[4]));
//	keyfile=(char *)malloc(sizeof(char) * strlen(argv[5]));
//        strcpy(nodename,argv[3]);
//	strcpy(ip,argv[4]);
//	strcpy(keyfile,argv[5]);
	port= atoi(argv[5]);
	insert_nodes(argv[3],argv[4],port,argv[6]);
//	free(nodename);
//	free(ip);
//	free(keyfile);
	goto closeexit;
    } //insert
    else if (!strcmp(argv[2],"delete")) {
	if (argc!=4) {
	    printf("Usage: %s workdir delete nodename\n",argv[0]);
	    goto closeexit;
	}

//	char* nodename;
//	nodename=(char *)malloc(sizeof(char) * strlen(argv[3]));
//	strcpy(nodename,argv[3]);
	delete_nodes(argv[3]);
//	free(nodename);
	goto closeexit;
    } //delete
    else if (!strcmp(argv[2],"sshopt")) {
	if (argc!=4) {
	    printf("Usage: %s workdir sshopt nodename\n",argv[0]);
	    goto closeexit;
	}
	select_stmtsshopt(argv[3]);
	goto closeexit;
    } //delete


closeexit:
    sqlite3_close(db);
//    free(cmd);
//   free(dbfile);
    return ret;
}
