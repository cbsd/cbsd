// Part of CBSD Project
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"

#include <getopt.h>
#include <stdarg.h>

#include "gentools.h"
#include "sqlhelper.h"
#include "nodesql.h"

int select_callbacksshopt(void *p_data, int num_fields, char **p_fields, char **p_col_names) {
int field;
int i;
int *p_rn = (int*)p_data;
char buf[SQLSTRLEN];

    first_row = 0;
    (*p_rn)++;

    bzero(buf,SQLSTRLEN);
    strcpy(buf,"-oBatchMode=yes -oStrictHostKeyChecking=no -oConnectTimeout=5 -q");

    for(i=0; i < num_fields; i++) {
	switch (i) {
	    case 2:strcat(buf," -i ");strcat(buf,p_fields[i]);break;
	    case 1:strcat(buf," -oPort=");strcat(buf,p_fields[i]);break;
	}
    }

    strcat(buf," ");
    strcat(buf,p_fields[0]);
    printf("%s",buf);
    return 0;
}


int select_callbackscpopt(void *p_data, int num_fields, char **p_fields, char **p_col_names) {
int field;
int i;
int *p_rn = (int*)p_data;
char buf[SQLSTRLEN];

    first_row = 0;
    (*p_rn)++;

    bzero(buf,SQLSTRLEN);
    strcpy(buf,"-oStrictHostKeyChecking=no -oConnectTimeout=5 -q ");

    for(i=0; i < num_fields; i++) {
	switch (i) {
	    case 0:strcat(buf," -oPort=");strcat(buf,p_fields[i]);break;
	    case 1:strcat(buf," -i ");strcat(buf,p_fields[i]);break;
	}
    }

    printf("%s\n",buf);
    return 0;
}

int select_stmtsshopt(const char* nodename) {
char *sqlerr;
int   ret;
int   nrecs = 0;
first_row = 1;
char buf[SQLSTRLEN];

    bzero(buf,SQLSTRLEN);
    if (rootkeyfile==NULL)
        sprintf(buf,"select ip,port,keyfile from nodelist where nodename=\"%s\" and status=\"0\"",nodename);
    else
        sprintf(buf,"select ip,port,rootkeyfile from nodelist where nodename=\"%s\" and status=\"0\"",nodename);

    debugmsg(1,"SQL: %s\n",buf);
    ret = sqlite3_exec(db, buf, select_callbacksshopt, &nrecs, &sqlerr);

    if(ret!=SQLITE_OK) {
	errmsg("Error in select statement %s [%s].\n", buf, sqlerr);
    }

    sql_stmt(buf);
    return ret;
}


int select_stmtscpopt(const char* nodename) {
char *sqlerr;
int   ret;
int   nrecs = 0;
first_row = 1;
char buf[SQLSTRLEN];

    bzero(buf,SQLSTRLEN);
    if (rootkeyfile==NULL) 
	sprintf(buf,"select port,keyfile from nodelist where nodename=\"%s\" and status=\"0\"",nodename);
    else
	sprintf(buf,"select port,rootkeyfile from nodelist where nodename=\"%s\" and status=\"0\"",nodename);

    debugmsg(1,"SQL: %s\n",buf);
    ret = sqlite3_exec(db, buf, select_callbackscpopt, &nrecs, &sqlerr);

    if(ret!=SQLITE_OK) {
	errmsg("Error in select statement %s [%s].\n", buf, sqlerr);
    }

    sql_stmt(buf);
    return ret;
}

void delete_nodes(char *nodename) {
char buf[SQLSTRLEN];

    bzero(buf,SQLSTRLEN);
    sprintf(buf,"delete from nodelist where nodename=\"%s\"",nodename);
    debugmsg(1,"SQL: %s\n",buf);
    sql_stmt(buf);
}


void insert_nodes(char *nodename, char *ip, int port, char *keyfile, char *rootkeyfile, char *invfile) {
char buf[SQLSTRLEN];

    sql_stmt("begin");
    bzero(buf,SQLSTRLEN);
    sprintf(buf,"insert into nodelist ( nodename, ip, port, keyfile, rootkeyfile, status, invfile ) values ( '%s','%s',%d,'%s','%s',0, '%s' )",nodename,ip,port,keyfile,rootkeyfile, invfile);
    debugmsg(1,"SQL: %s\n",buf);
    sql_stmt(buf);
    sql_stmt("commit");
}


void usage() {
    printf("Tools for update node registry in sqlite database\n");
    printf("require: dbfile\n");
    printf("opt: action workdir nodename ip port keyfile rootkeyfile\n");
    printf("dbfile - sqlite database file\n");
    printf("sqlquery - execute direct sql query\n");
    printf("action can be:\n");
    printf("- init - for re-create database (drop and create empty table)\n");
    printf("- upgrade - upgrade table scheme if necessary\n");
    printf("- list - select all records from database\n");
    printf("- insert ( nodename=XXXX ip=XXXX port=XXX keyfile=XXXX ) for insert new records\n");
    printf("- delete ( nodename=XXXX ) for remove records from registry\n");
    printf("- sshopt ( nodename=XXXX ) return ssh options for ssh connection from command line (--rootkeyfile=any for root id_rsa selected)\n");
    printf("- scpopt ( nodename=XXXX ) return ssh options for scp connection from command line (--rootkeyfile=any for root id_rsa selected\n");
    printf("- get ( nodename=XXXX param=ip ) select ip value for nodename\n");
    printf("\n");
}


int main(int argc, char **argv)
{
int win = FALSE;
int optcode = 0;
int option_index = 0, ret = 0;
int action=0;
int i=0, firstrow=0;
char buf[SQLSTRLEN];

static struct option long_options[] = {
    { "dbfile", required_argument, 0 , C_DBFILE },
    { "action", required_argument, 0 , C_ACTION },
    { "help", no_argument, 0, C_HELP },
    { "debug", required_argument, 0, C_DEBUG },
    { "workdir", required_argument, 0, C_WORKDIR },
    { "ip", required_argument, 0, C_IP },
    { "port", required_argument, 0, C_PORT },
    { "keyfile", required_argument, 0, C_KEYFILE },
    { "rootkeyfile", required_argument, 0, C_ROOTKEYFILE },
    { "nodename", required_argument, 0, C_NODENAME },
    { "param", required_argument, 0, C_PARAM },
    { "sqlquery", required_argument, 0, C_SQLQUERY },
    { "invfile", required_argument, 0, C_INVFILE },
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
		while (actionlist[++i]!=NULL)
		    if (!strcmp(optarg,actionlist[i])) {
			action=i;
			break;
		    }
		    if (action==0) {
			errmsg("Bad action\r\n");
			exit(1);
		    }
		break;
    	    case C_WORKDIR:
        	workdir=optarg;
                break;
    	    case C_IP:
        	ip=optarg;
                break;
    	    case C_PORT:
        	port=atoi(optarg);
		if (port>65535) {
		    errmsg("Port %d is incorrect\n");
		    exit(1);
		}
                break;
    	    case C_KEYFILE:
        	keyfile=optarg;
		break;
    	    case C_ROOTKEYFILE:
        	rootkeyfile=optarg;
		break;
    	    case C_NODENAME:
        	nodename=optarg;
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
            case C_INVFILE:
                invfile=optarg;
		break;
	    case C_SQLQUERY:
                memset(buf,0,sizeof(buf));
                strcpy(buf,optarg);
                if (optind < argc)
                    while (optind < argc) {
                            strcat(buf," ");
                            strcat(buf,argv[optind++]);
                    }
                sqlquery=malloc(strlen(buf)+1);
                memcpy(sqlquery,buf,strlen(buf));
                sqlquery[strlen(buf)]='\0';
                debugmsg(1,"Execute direct SQL: [%s]\n",sqlquery);
                sqlite3_open(dbfile, &db);
                if (db == 0 ) {
                        errmsg("Could not open database %s\n",dbfile);
                        exit(1);
                }
                ret=select_valstmt(sqlquery);
                goto closeexit;
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
	    ret=sql_stmt("drop table if exists nodelist");
	    memset(buf,0,sizeof(buf));
            strcpy(buf,"create table nodelist ( ");

            firstrow=0;
            //aggregate all actual data for SQL tables
            for (i=0 ; *sqldb_info[i].rowname != '\n'; i++) {
                    if (firstrow) strcat(buf,", ");
                    strcat(buf,sqldb_info[i].rowname);
                    strcat(buf," ");
                    strcat(buf,sqldb_info[i].rowtype);
                    firstrow++;
            }

            debugmsg(2,"DB have %d items\n",firstrow);
            strcat(buf,")");
            debugmsg(1,"SQL Exec: %s\n",buf);
            ret=sql_stmt(buf);

            if (ret==0) debugmsg(1,"table 'nodelist' init successfull\n");
                else errmsg("table 'nodelist' init failed\n");

            goto closeexit;
        break;
    case (UPGRADE):
            //aggregate all actual data for SQL tables
            for (i=0 ; *sqldb_info[i].rowname != '\n'; i++) {
		    memset(buf,0,sizeof(buf));
		    sprintf(buf,"SELECT %s FROM nodelist LIMIT 1",sqldb_info[i].rowname);
		    ret=sql_stmt(buf);
		    if (ret!=0) {
			//probably not exist, try to alter
			memset(buf,0,sizeof(buf));
			sprintf(buf,"ALTER TABLE nodelist ADD COLUMN %s %s",sqldb_info[i].rowname,sqldb_info[i].rowtype);
			ret=sql_stmt(buf);
			if (ret==0) 
			    debugmsg(0,"nodesql table updated: %s column\n",sqldb_info[i].rowname);
			else
			    errmsg("update nodesql table error for %s column\n",sqldb_info[i].rowname);
		    }
            }
            goto closeexit;
        break;
    case (LIST):
	    ret=select_valstmt("select nodename from nodelist");
	    goto closeexit;
	break;
    case (INSERT):
	    if (!nodename||!ip||port==0||!keyfile||!invfile) {
		errmsg("required arguments: --nodename, --ip, --port, --keyfile, --invfile\n");
		ret=1;
		goto closeexit;
	    }
	    if (rootkeyfile==NULL) rootkeyfile="/root/.ssh/id_rsa";
	    insert_nodes(nodename,ip,port,keyfile,rootkeyfile,invfile);
	    goto closeexit;
	break;
    case (DELETE):
	    if (!nodename) {
		errmsg("required arguments: --nodename\n");
		ret=1;
		goto closeexit;
	    }
	    delete_nodes(nodename);
	    goto closeexit;
	break;
    case (SSHOPT):
	    if (!nodename) {
		errmsg("required arguments: --nodename\n");
		ret=1;
		goto closeexit;
	    }
    	    select_stmtsshopt(nodename);
	    goto closeexit;
	break;
    case (SCPOPT):
	    if (!nodename) {
		errmsg("required arguments: --nodename\n");
		ret=1;
		goto closeexit;
	    }
    	    select_stmtscpopt(nodename);
	    goto closeexit;
	break;
    case (GET):
	    if (!nodename||!param) {
		errmsg("required arguments: --nodename --param\n");
		ret=1;
		goto closeexit;
	    }
	    memset(buf,0,sizeof(buf));
	    sprintf(buf,"select %s from nodelist where nodename=\"%s\"",param,nodename);
	    ret=select_valstmt(buf);
	    goto closeexit;
	break;
    default:
	goto closeexit;
} //switch

closeexit:
    sqlite3_close(db);
    return ret;
}
