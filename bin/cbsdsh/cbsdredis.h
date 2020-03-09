#ifndef CBSDREDIS_H
#define CBSDREDIS_H

#include "contrib/credis.h"

#define DEFREDISDELIMER "|"


// Filter flags
#define RF_SETENV	1
#define RF_PRINT	2
#define RF_WITHKEYS	4
#define RF_KEYSONLY	8
#define RF_INVERT	32
#define RF_ARRAY	64
#define RF_KEYLIST	128

// Connection flags
#define RCF_DISABLED	1


typedef struct { 
	REDIS res; 
	char *hostname;
	char *password;
	uint16_t port;
	uint16_t database;
	uint32_t flags;
} cbsdredis_t;

void	cbsd_redis_free(void);
int 	cbsd_redis_init(void);
int 	redis_do(const char *cmd, char ret_type, unsigned int flags, int argc, char **argv, int *valc, char **valv);


#endif

