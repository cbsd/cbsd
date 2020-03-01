#ifndef CBSDINFLUX_H
#define CBSDINFLUX_H

#include <curl/curl.h>


#define ICF_DISABLED	1


typedef struct { 
	char	*uri;			// Uri used to write to the database
	char	*token;			// Token itself if we need to rewrite the header
	char	*hostname;		// Hostname if we need to rewrite the Uri
	char	*database;		// Database ^^

#ifndef CBSD
	char	*buffer;		// Buffer used by racct_stats
	struct {
		char	*bhyve;		// Table for bhyve stats
		char	*jails;		// Table for jail stats
		char	*nodes;		// Table for hoster stats
	} tables;
	uint16_t items;			// Items currently in the buffer
#endif
	uint16_t port;			// Port
        uint16_t flags;			// For some flags..


} cbsdinflux_t;

void	cbsd_influx_free(void);
int 	cbsd_influx_init(void);
#ifndef CBSD
int 	cbsd_influx_transmit_buffer();
#endif


#endif

