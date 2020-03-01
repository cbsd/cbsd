static int config_handler(void* user, const char* section, const char* name, const char* value){
#ifdef WITH_DBI
        if(strncmp("sql:", section, 4) == 0 && strlen(section) > 4){

		if(!databases->lib_handle) return(1);

		sql_database_t *seek;
		for(seek=databases->list; NULL != seek; seek=seek->next) 
			if(strlen(seek->name) == strlen(section+4) && strcmp(seek->name, section+4) == 0) break; 	

		if(!seek){
//			printf("sql: added database settings '%s'\n",section+4);

			seek=malloc(sizeof(sql_database_t));
			if(!seek){ fprintf(stderr, "sqlcmd.c: Memory error!"); return(0); }
			bzero(seek, sizeof(sql_database_t));
			seek->name=strdup(section+4);
			seek->next=databases->list; databases->list=seek; // Add to the list.
			seek->flags=DCF_DISABLED;
		}

		if(strcmp("type", name) == 0) seek->type=strdup(value);
		else if(!seek->hostname && strcmp("host", name) == 0) seek->hostname=strdup(value);
		else if(!seek->username && strcmp("user", name) == 0) seek->username=strdup(value);
		else if(!seek->username && strcmp("dbdir", name) == 0) seek->username=strdup(value);
		else if(!seek->password && strcmp("password", name) == 0) seek->password=strdup(value);
		else if(seek->encoding && strcmp("encoding", name) == 0) seek->encoding=strdup(value);
		else if(seek->database && strcmp("database", name) == 0) seek->database=strdup(value);
		else if(strcmp("port", name) == 0) seek->port=atoi(value);
	        else if(strcmp("enabled", name) == 0 && (strcmp("yes",value) == 0)) seek->flags&=~DCF_DISABLED;
		return(1);
	}
#endif

#ifdef WITH_REDIS
        if(strcmp("redis", section) == 0){
	        if(strcmp("host", name) == 0) redis->hostname=strdup(value);
	        else if(strcmp("port", name) == 0) redis->port=atoi(value);
	        else if(strcmp("password", name) == 0) redis->password=strdup(value);
	        else if(strcmp("database", name) == 0) redis->database=atoi(value);
	        else if(strcmp("enabled", name) == 0 && !(strcmp("yes",value) == 0)) redis->flags|=RCF_DISABLED; // TODO: Fix this / reverse it.
		return(1);
	}
#endif
#ifdef WITH_INFLUX
	if(strcmp("influx", section) == 0){
		if(strcmp("host", name) == 0) influx->hostname=strdup(value);
	        else if(strcmp("database", name) == 0) influx->database=strdup(value);
	        else if(strcmp("token", name) == 0) influx->token=strdup(value);
		else if(strcmp("port", name) == 0) influx->port=atoi(value);
	        else if(strcmp("enabled", name) == 0 && !(strcmp("yes",value) == 0)) influx->flags|=ICF_DISABLED; // TODO: Fix this / reverse it.
		return(0); // other options may exist we don't use.
	}
#endif
        return 1;
}


void	load_config(){
	// Parse config
	if (ini_parse("/usr/local/etc/cbsd-ext.conf", config_handler, NULL) < 0) {
                fprintf(stderr, "Warning: Can't load '/usr/local/etc/cbsd-ext.conf' using defaults!\n");
#ifdef WITH_REDIS
                redis->hostname=strdup("127.0.0.1");
                redis->password=strdup("cbsd");
                redis->port=6379;
                redis->database=2;
#endif
#ifdef WITH_INFLUX
                influx->hostname=strdup("127.0.0.1");
                influx->database=strdup("cbsd");
                influx->port=8086;
#endif
        }
}

