#ifndef CBSDREDIS_H
#define CBSDREDIS_H

#include "contrib/credis.h"

#define RF_SETENV	1
#define RF_PRINT	2
#define RF_WITHKEYS	4

//#define RF_SETENV	16
#define RF_INVERT	32
#define RF_KEYLIST	128


typedef struct { 
	REDIS res; 
	char *hostname;
	char *password;
	uint16_t port;
	uint16_t database;
} cbsdredis_t;

void	redis_free(void);
int redis_load_config(void);

#endif

