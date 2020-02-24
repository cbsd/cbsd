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
 *   * Neither the name of Credis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
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

int redis_do(const char *cmd, char ret_type, unsigned int skip, unsigned int flags, int argc, char **argv) {
	REDIS	res;
	int rc=-1, i;

	if(NULL == redis) return(1); // No Redis

	for(rc=-1; rc==-1;){
		if((res=redis_connect(true))==NULL) return(2);

		cr_buffer *buf = &(redis->res->buf);
		buf->len = 0;

		if ((rc = credis_raw_append(buf, "*%zu\r\n$%zu\r\n%s\r\n", argc-skip, strlen(cmd), cmd)) != 0) return(rc);
		for (i = skip+1; i < argc; i++) {
			if ((rc = credis_raw_append(buf, "$%zu\r\n%s\r\n", strlen(argv[i]), argv[i])) != 0) return(rc);
		}

		rc = credis_raw_sendandreceive(redis->res, ret_type);
		if((rc=redis_error(redis->res, rc)) == 0){
			switch(ret_type){
				case CR_BULK:
					if(NULL == redis->res->reply.bulk) return(1);

					printf("%s\n",redis->res->reply.bulk);
					return(0);

				case CR_MULTIBULK:
					if(0 != redis->res->reply.multibulk.len){
						for(i=0; i<redis->res->reply.multibulk.len; i++){
							if(RF_KEYLIST & flags){
								if(RF_SETENV & flags) setvarsafe(redis->res->reply.multibulk.bulks[i],redis->res->reply.multibulk.bulks[i+1], 0);
								if(RF_WITHKEYS & flags) printf("%s=%s\n",redis->res->reply.multibulk.bulks[i],redis->res->reply.multibulk.bulks[i+1]);
								else if(RF_PRINT & flags) printf("%s\n",redis->res->reply.multibulk.bulks[i+1]);
								i++;
							}else{
								if(RF_SETENV & flags) setvarsafe(argv[2+i+skip], redis->res->reply.multibulk.bulks[i], 0);
								if(RF_WITHKEYS & flags) printf("%s=%s\n",argv[2+i+skip], redis->res->reply.multibulk.bulks[i]);
								else if(RF_PRINT & flags) printf("%s\n",redis->res->reply.multibulk.bulks[i]);
							}
						}
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

int redis_hget(int argc, char **argv) {
	int	skip=0, flags=0;

	if(argc > 1){
		if(strncmp(argv[1], "-env", 4) == 0){ skip=1; flags=1; }
		else if(strncmp(argv[1], "-show", 5) == 0){ skip=1; flags=3; }
		else if(strncmp(argv[1], "-keys", 5) == 0){ skip=1; flags=4; }
		else if(strncmp(argv[1], "-all", 4) == 0){ skip=1; flags=7; }
		else{ flags=2; }
	}
	if (argc-skip < 2) {
		printf("format: hget [-env|-keys|-show|-all] hash [key] [key]\n");
		return(1);
	}

	if (argc-skip > 3) return(redis_do("HMGET", CR_MULTIBULK, skip, flags, argc, argv));
	if (argc-skip < 3) return(redis_do("HGETALL", CR_MULTIBULK, skip, 128 | flags, argc, argv));
	return(redis_do("HGET", CR_BULK, skip, flags, argc, argv));

}

int redis_hset(int argc, char **argv) {
//	int	skip=0;
	if (argc < 4) {
		printf("format: hset hash key value [key value] [key value]\n");
		return(1);
	}

	if (argc > 4) return(redis_do("HMSET", CR_INLINE, 0, 0, argc, argv));
	return(redis_do("HSET", CR_INT, skip, 0, argc, argv));

}

#define REDIS_SIMPLE(f_name,o_name,rettype,flags,params,msg) \
int redis_##f_name(int argc, char **argv) { \
	if(argc < params){ printf("format: %s %s\n", argv[0], #msg); return(1); } \
	return(redis_do(o_name, rettype, 0, flags, argc, argv)); \
}

REDIS_SIMPLE(hdel,   "HDEL",     CR_INT,    0, 3, "hash key");
REDIS_SIMPLE(kdel,   "DEL",      CR_INT,    0, 2, "item");
REDIS_SIMPLE(lpush,  "LPUSH",    CR_INT,    0, 3, "list item");
REDIS_SIMPLE(rpush,  "RPUSH",    CR_INT,    0, 3, "list item");
REDIS_SIMPLE(lpop,   "LPOP",     CR_BULK,   0, 2, "list");
REDIS_SIMPLE(rpop,   "RPOP",     CR_BULK,   0, 2, "list");
REDIS_SIMPLE(exists, "EXISTS",   CR_INT,   32, 2, "item");
REDIS_SIMPLE(hexists,"HEXISTS",  CR_INT,   32, 3, "hash key");
REDIS_SIMPLE(ttl,    "TTL",      CR_INT,   36, 2, "item");
REDIS_SIMPLE(expire, "EXPIRE",   CR_INT,   32, 3, "item timeout");
REDIS_SIMPLE(publish,"PUBLISH",  CR_INT,   48, 3, "channel data");
REDIS_SIMPLE(ltrim,  "LTRIM",    CR_INLINE, 0, 4, "list from to");
REDIS_SIMPLE(lindex, "LINDEX",   CR_BULK,   0, 3, "list index");
REDIS_SIMPLE(llen,   "LLEN",     CR_INT,   36, 2, "list");
REDIS_SIMPLE(sadd,   "SADD",     CR_INT,   32, 3, "set item");
REDIS_SIMPLE(srem,   "SREM",     CR_INT,   32, 3, "set item");
REDIS_SIMPLE(sexists,"SISMEMBER",CR_INT,   32, 3, "set item");
REDIS_SIMPLE(slen,   "SCARD",    CR_INT,   36, 2, "set");
REDIS_SIMPLE(smove,  "SMOVE",    CR_INT,   32, 4, "from-set to-set item");
#undef REDIS_SIMPLE


int redis_blpop(int argc, char **argv) {
	REDIS	res;
	int	rc=-1;
	char 	*val;

	if (argc < 3) {
		printf("format: %s list timeout\n", argv[0]);
		return 0;
	}else if(NULL == redis) return(1); // No Redis

	while(rc == -1){
		if(NULL == (res=redis_connect(false))) return(2);

		rc=credis_blpop(res, argv[1], argv[2], &val);
		if(rc > 0){
			printf("%s\n", val);
			rc=0;
		}else if(rc < 0) rc=redis_error(res, rc); 
		else rc=1;

		credis_close(res); res=NULL; 
	}
	return rc;
}

int redis_brpop(int argc, char **argv) {
	REDIS	res;
	int	rc=-1;
	char 	*val;

	if (argc < 3) {
		printf("format: %s list timeout\n", argv[0]);
		return 0;
	}else if(NULL == redis) return(1); // No Redis

	while(rc == -1){
		if(NULL == (res=redis_connect(false))) return(2);

		rc=credis_brpop(res, argv[1], argv[2], &val);
		if(rc > 0){
			printf("%s\n", val);
			rc=0;
		}else if(rc < 0) rc=redis_error(res, rc); 
		else rc=1;

		credis_close(res); res=NULL;

	}
	return rc;
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
int redis_load_config(void){
	if((redis=malloc(sizeof(cbsdredis_t)))==NULL) return(-1);
	bzero(redis, sizeof(cbsdredis_t));
	
	redis->hostname=strdup("127.0.0.1");
	redis->password=strdup("password");	
	redis->port=6379;
	redis->database=2;

	return(0);
}

void redis_free(void){
	if(!redis) return;
	
	if(NULL != redis->res) credis_close(redis->res);

	if(NULL != redis->hostname) free(redis->hostname);
	if(NULL != redis->password) free(redis->password);

	free(redis);
}
