/* cbsdredis.c - Redis functions for CBSDSH
 *
 * Copyright (c) 2020, Stefan Rink <stefanrink at yahoo dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>

#include "var.h"
#include "contrib/ini.h"
#include "mystring.h"
#include "cbsdredis.h"

#ifdef DEBUG_REDIS
#define DEBUG_PRINTF(...) printf(__VA_ARGS__);
#else
#define DEBUG_PRINTF(...) 
#endif

extern cbsdredis_t	*redis;

int	redis_error(REDIS res, int num){
	switch(num){
		case CREDIS_OK:
			return(0);

		case CREDIS_ERR_SEND: 
		case CREDIS_ERR_RECV: 
			DEBUG_PRINTF("REDIS: protocol-error\n");
			if(NULL != res) { // persistent, keep buffers
				close(res->fd); 
				res->fd=0;
			}
			return(-1); // retry

		case CREDIS_ERR_PROTOCOL:
			if(strncmp("WRONGTYPE", credis_errorreply(res), 8) == 0){
				printf("Invalid data type!\n");
			}else if(strncmp("ERR ", credis_errorreply(res), 4) == 0){
				printf("Error with command, check you parameters!\n");
			}else{
				DEBUG_PRINTF("REDIS: error %d!\n%s\n", num, credis_errorreply(res));
			}
		case -1: return(1); // Empty

		default:
			if(num < 0){
				DEBUG_PRINTF("REDIS: error %d!\n%s\n", num, credis_errorreply(res));
				return(1); // quit
			}
	}
	return(0); // Ok if > 0
}

REDIS redis_connect(bool persist){
	REDIS	res=NULL;
	int	rc;

	if(NULL == redis || NULL == redis->hostname) return(NULL);
	if(RCF_DISABLED & redis->flags) return(NULL); // Redis is disabled!


	// Reconnect/persist
	if(persist && NULL != redis->res){
		res=redis->res;
		if(redis->res->fd != 0) return(res);

		if(credis_reconnect(redis->res) != 0){
			DEBUG_PRINTF("REDIS: Reconnect failed!\n");
			return(NULL);
		}else DEBUG_PRINTF("REDIS: Reconnected!\n");

	}else{
		if(NULL == (res = credis_connect(redis->hostname, redis->port, 100000))){
			printf("REDIS: Error connecting to Redis server\n");
			return(NULL);
		}
	}

	if(NULL != redis->password && (rc = credis_auth(res, redis->password)) < 0){
		printf("REDIS: Error authenticating to Redis server.\n");
		credis_close(res); 
		return(NULL);
	}

	if((rc = credis_select(res, redis->database)) < 0){
		printf("REDIS: Error selecting Redis database.\n");
		credis_close(res); 
		return(NULL);
	}

	if(persist && NULL == redis->res) redis->res=res; // Persistent connection
	return(res);
}

int redis_do(const char *cmd, char ret_type, unsigned int flags, int argc, char **argv) {
	REDIS	res;
	int rc=-1, i;
	const char	*delim=NULL;

	if(NULL == redis) return(1); // No Redis


	for(rc=-1; rc==-1;){
		if((res=redis_connect(true))==NULL) return(2);

		cr_buffer *buf = &(redis->res->buf);
		buf->len = 0;

		if ((rc = credis_raw_append(buf, "*%zu\r\n$%zu\r\n%s\r\n", argc+1, strlen(cmd), cmd)) != 0) return(rc);
		for (i = 0; i < argc; i++) {

		//	-- Does not seem to work..
		//	if(is_number(argv[i])){	
		//		if ((rc = credis_raw_append(buf, ":%s\r\n", argv[i])) != 0) return(rc);
		//	}else{
				if ((rc = credis_raw_append(buf, "$%zu\r\n%s\r\n", strlen(argv[i]), argv[i])) != 0) return(rc);
		//	}
		}

//		printf("[%s]\n",buf->data);

		if(!delim){
		        if ((delim = lookupvar("radiusdelimer")) == NULL) delim = DEFREDISDELIMER;
		}

		rc = credis_raw_sendandreceive(redis->res, ret_type);
		if((rc=redis_error(redis->res, rc)) == 0){
			switch(ret_type){
				case CR_BULK:
					if(NULL == redis->res->reply.bulk) return(1);

					if(RF_SETENV & flags) setvarsafe(argv[1], redis->res->reply.bulk, 0);
					if(RF_WITHKEYS & flags) printf("%s=%s\n",argv[1], redis->res->reply.bulk);
					else if(RF_PRINT & flags) printf("%s\n",redis->res->reply.bulk);
					return(0);

				case CR_MULTIBULK:
					if(0 != redis->res->reply.multibulk.len){
						for(i=0; i<redis->res->reply.multibulk.len; i++){
							if(RF_KEYLIST & flags){
								if(RF_KEYSONLY & flags){
									if(RF_SETENV & flags) setvarsafe(redis->res->reply.multibulk.bulks[i], NULL, 0); 
									if(RF_PRINT & flags) printf("%s%s",(i==0?"":delim), redis->res->reply.multibulk.bulks[i]);
								}else{
									if(RF_SETENV & flags) setvarsafe(redis->res->reply.multibulk.bulks[i],redis->res->reply.multibulk.bulks[i+1], 0);
									if(RF_WITHKEYS & flags) printf("%s=\"%s\"\n",redis->res->reply.multibulk.bulks[i],redis->res->reply.multibulk.bulks[i+1]);
									else if(RF_PRINT & flags) printf("%s%s",(i==0?"":delim),redis->res->reply.multibulk.bulks[i+1]);
								}
								i++;
							}else if(RF_KEYSONLY & flags){
								if(RF_SETENV & flags) setvarsafe(argv[1+i], NULL, 0); 
								if(RF_PRINT & flags) printf("%s%s", (i==0?"":delim),argv[1+i]);
							}else{
								if(RF_SETENV & flags) setvarsafe(argv[1+i], redis->res->reply.multibulk.bulks[i], 0);
								if(RF_WITHKEYS & flags) printf("%s=\"%s\"\n",argv[1+i], redis->res->reply.multibulk.bulks[i]);
								else if(RF_PRINT & flags) printf("%s%s",(i==0?"":delim),redis->res->reply.multibulk.bulks[i]);
							}
						}
						if(RF_PRINT & flags) printf("\n");
						return(0);
					}else return(1);

				case CR_INT:
					if(RF_PRINT & flags) printf("%i\n",redis->res->reply.integer);
					if(RF_INVERT & flags){
						if(redis->res->reply.integer == 0) return(1); else return(0);
					}else return(redis->res->reply.integer);
			
				case CR_INLINE: return(0); 
					break;

				default:
					printf("REDIS RESPONSE ERROR: %d/%i\n", redis->res->reply.multibulk.len, redis->res->reply.integer);
					break;

			}
			//printf("-- %d\n",redis->res->reply.multibulk.bulks, redis->res->reply.multibulk.len);
		}
	}
	return rc;
}






int redis_bpop(uint8_t left, char *key, char *seconds) {
	REDIS	res;
	int	rc=-1;

	while(rc == -1){
		if(NULL == (res=redis_connect(false))) return(2);

		int rc = cr_sendfandreceive(res, CR_MULTIBULK, "*3\r\n$5\r\n%s\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n", left==1?"BLPOP":"BRPOP", strlen(key), key, strlen(seconds), seconds);
	  	if (rc > 0){
			if(res->reply.multibulk.len != 2) return(0);
	 		printf("%s\n",res->reply.multibulk.bulks[1]);
			rc=0;
		}else if(rc < 0) rc=redis_error(res, rc);
		else rc=1;	// No results / timeout

		credis_close(res); res=NULL; 
	}
 	return rc;
}

int redis_blpop(int argc, char **argv) {
	if(NULL == redis) return(1); // No Redis
	if (argc != 3){ printf("format: %s list timeout\n", argv[0]); return 1; }

	return(redis_bpop(1, argv[1], argv[2]));
}

int redis_brpop(int argc, char **argv) {
	if(NULL == redis) return(1); // No Redis
	if (argc < 3){ printf("format: %s list timeout\n", argv[0]); return 1; }

	return(redis_bpop(0, argv[1], argv[2]));
}


// cbsdredis CMD [options] QUERY
int redis_cmd(int argc, char **argv) {
	unsigned int item=1, flags=0, items=argc-1;

	if(items > 0)
		for(; item < argc; items--){
			if(strncmp(argv[item], "-env", 4) == 0){ item++; flags|=RF_SETENV; }
			else if(strncmp(argv[item], "-show", 5) == 0){ item++; flags|=RF_PRINT; }
			else if(strncmp(argv[item], "-keysonly", 9) == 0){ item++; flags|=RF_KEYSONLY; }
			else if(strncmp(argv[item], "-keys", 5) == 0){ item++; flags|=RF_WITHKEYS; }
			else break;
		}

	if (items < 1) {
		printf("Missing command!\nUse %s CMD [options] [query]\n", argv[0]);
		printf("- hget	  [options] HASH [[field [field...]]\n");
		printf("- hset	  [options] HASH [[field value [field value...]]\n");
		printf("- hdel	  [options] HASH [field]\n");
		printf("- del 	  [options] ITEM\n");
		printf("- publish [options] QUEUE ITEM\n");
		return(1);
	}

	char *cmd=argv[item];
	items--; item++;
	if(strcmp("hget",cmd)==0){
		if (items < 1) { printf("You at least need to give a hash!\n"); return(1); }

		if(!(RF_SETENV & flags)) flags|=RF_PRINT; // If not to env put it on screen.

		// TODO: make keysonly ask for keys only..

		if (items < 2) return(redis_do("HGETALL", CR_MULTIBULK, RF_KEYLIST | flags, items, &argv[item]));
		if (items > 2) return(redis_do("HMGET", CR_MULTIBULK, flags, items, &argv[item]));
		return(redis_do("HGET", CR_BULK, flags, items, &argv[item]));
	}
	if(strcmp("hset",cmd)==0){
		if(1 & flags){ // ENV
			if (items < 2) { printf("You at least need to give a hash and key!\n"); return(1); }

			char **vals;
			int valc=1, rc;

			// Todo: Make this a bit cleaner..

			if((vals=malloc((sizeof(void *)*items*2)+sizeof(void *)))==NULL) return(-1); 

			vals[0]=argv[item++]; items--; // Hash
			for(; items>0; items--){
				vals[valc++]=argv[item];
				char *tmp=lookupvar(argv[item++]);
				if(tmp) vals[valc++]=tmp; else vals[valc++]=""; // Should we delete missing vars from the store?
			}
			if (valc > 4) rc=redis_do("HMSET", CR_INLINE, flags, valc, vals);
			else rc=redis_do("HSET", CR_INT, flags, valc, vals);
			free(vals);
			return(rc);
		}

		if (items < 3) {
			printf("Missing hash/key/value's\n");
			return(1);
		}



//cbsdredis hset test demo, sjaak, joost -vals- joost, sjaak, bla
		unsigned int center=(items / 2);
		if(strncmp(argv[item+center], "-vals", 5) == 0){ 
			int rc;										// Return code (for errors)
			unsigned int valc=0;								// Value Counter
			char **vals;									// Value array
			if((vals=malloc((sizeof(void *)*items)+sizeof(void *)))==NULL) return(-1); 	// Allocate ram

#define removeAfter(what, item) if(item[strlen(item)-1] == what) item[strlen(item)-1]=0;

			vals[valc++]=argv[item];							// Set the hash
			for(int i=1; i<center; i++){							// Set the rest
													// Remove trailing ,
				removeAfter(',', argv[item+i]) removeAfter(',', argv[center+item+i])
				removeAfter('\'', argv[item+i]) removeAfter('\'', argv[center+item+i])

				if(argv[item+i][0]=='\'') vals[valc++]=&argv[item+i][1]; else vals[valc++]=argv[item+i];
				if(argv[item+i+center][0]=='\'') vals[valc++]=&argv[item+i+center][1]; else vals[valc++]=argv[item+i+center];
			}

#undef removeAfter

			if(center > 2) rc=redis_do("HMSET", CR_INLINE, flags, valc, vals);
			else rc=redis_do("HSET", CR_INT, flags, valc, vals);
			free(vals);

			return(rc);
		}

		if (items > 4) return(redis_do("HMSET", CR_INLINE, flags, items, &argv[item]));
		return(redis_do("HSET", CR_INT, flags, items, &argv[item]));
	}


#define REDIS_SIMPLE(f_name,o_name,rettype,xflags,params,msg) \
	if(strcmp(f_name,cmd)==0){ \
		if(items < params){ printf("format: %s %s\n", argv[0], #msg); return(1); } \
		return(redis_do(o_name, rettype, flags|xflags, items, &argv[item])); \
	}

REDIS_SIMPLE("hdel",   "HDEL",     CR_INT,    0, 2, "hash key");
REDIS_SIMPLE("del",    "DEL",      CR_INT,    0, 1, "item");
REDIS_SIMPLE("lpush",  "LPUSH",    CR_INT,    0, 2, "list item");
REDIS_SIMPLE("rpush",  "RPUSH",    CR_INT,    0, 2, "list item");
REDIS_SIMPLE("lpop",   "LPOP",     CR_BULK,   0, 1, "list");
REDIS_SIMPLE("rpop",   "RPOP",     CR_BULK,   0, 1, "list");
REDIS_SIMPLE("exists", "EXISTS",   CR_INT,   32, 1, "item");
REDIS_SIMPLE("hexists","HEXISTS",  CR_INT,   32, 2, "hash key");
REDIS_SIMPLE("ttl",    "TTL",      CR_INT,   36, 1, "item");
REDIS_SIMPLE("expire", "EXPIRE",   CR_INT,   32, 2, "item timeout");
REDIS_SIMPLE("publish","PUBLISH",  CR_INT,   48, 2, "channel data");
REDIS_SIMPLE("ltrim",  "LTRIM",    CR_INLINE, 0, 3, "list from to");
REDIS_SIMPLE("lindex", "LINDEX",   CR_BULK,   0, 2, "list index");
REDIS_SIMPLE("llen",   "LLEN",     CR_INT,   36, 1, "list");
REDIS_SIMPLE("sadd",   "SADD",     CR_INT,   32, 2, "set item");
REDIS_SIMPLE("srem",   "SREM",     CR_INT,   32, 2, "set item");
REDIS_SIMPLE("sexists","SISMEMBER",CR_INT,   32, 2, "set item");
REDIS_SIMPLE("slen",   "SCARD",    CR_INT,   36, 1, "set");
REDIS_SIMPLE("smove",  "SMOVE",    CR_INT,   32, 3, "from-set to-set item");
#undef REDIS_SIMPLE


	fprintf(stderr, "Invalid redis command\n");
	return(1);

}


#ifdef IDLE_USE_REDIS
int update_idlecmd(int argc, char **argv) {
	char		buffer[20];

	fprintf(buffer, "%lu", (unsigned long)time(NULL)); 

	if (argc != 2) {
		out1fmt("usage: update_idle <nodename>\n");
		return 0;
	}else if(NULL == redis) return(1); // No Redis
	else if(NULL == redis->res && NULL == redis_connect()) return(2);

	credis_hset(redis->res, argv[1], "idle", &buffer);

	return 0;
}
#endif


// Used in main()
int cbsd_redis_init(void){
	if((redis=malloc(sizeof(cbsdredis_t)))==NULL) return(-1);
	bzero(redis, sizeof(cbsdredis_t));
	return(0);
}

void cbsd_redis_free(void){
	if(!redis) return;
	
	if(NULL != redis->res) credis_close(redis->res);

	if(NULL != redis->hostname) free(redis->hostname);
	if(NULL != redis->password) free(redis->password);

	free(redis);
}

