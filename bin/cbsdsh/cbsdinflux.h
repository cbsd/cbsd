#ifndef CBSDINFLUX_H
#define CBSDINFLUX_H

#include <curl/curl.h>


#define ICF_DISABLED	1


typedef struct { 
	char	*uri;			// Uri used to write to the database
	char	*token;			// Token itself if we need to rewrite the header
	char	*hostname;		// Hostname if we need to rewrite the Uri
	char	*database;		// Database ^^
	uint16_t port;			// Port
        uint16_t flags;			// For some flags..

} cbsdinflux_t;

void	cbsd_influx_free(void);
int 	cbsd_influx_init(void);

#endif

