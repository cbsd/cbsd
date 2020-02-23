#ifndef CBSDREDIS_H
#define CBSDREDIS_H

#include "contrib/credis.h"

typedef struct {
	REDIS		res;
	char		*hostname;
	char		*password;
	uint16_t	port;
	uint16_t	database;
} cbsdredis_t;

void	redis_free(void);
int redis_load_config(void);

#endif

