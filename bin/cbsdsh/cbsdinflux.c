/* cbsdinflux.c - Influx functions for CBSDSH
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
#include "cbsdinflux.h"

//#define DEBUG_INFLUX

#ifdef DEBUG_INFLUX
#define DEBUG_PRINTF(...) printf(__VA_ARGS__);
#else
#define DEBUG_PRINTF(...)
#endif

extern cbsdinflux_t *influx;

int
function_pt(void *ptr, size_t size, size_t nmemb, void *stream)
{
	DEBUG_PRINTF("Inlfux returned: %s", (char *)ptr);
	return (0);
}

int
cbsd_influx_do(char *query)
{
	CURL *curl; // Can't really persist
	CURLcode result;

	if (NULL == influx || NULL == influx->hostname ||
	    NULL == influx->database)
		return (-2);
	if (ICF_DISABLED & influx->flags)
		return (-3); // Influx is disabled!

	if (NULL == influx->uri) { // Prepare the URI
		size_t len = strlen(influx->hostname) +
		    strlen(influx->database) + 35;
		if (!(influx->uri = malloc(len)))
			return (-1);
		bzero(influx->uri, len);
		sprintf(influx->uri, "http://%s:%d/write?db=%s",
		    influx->hostname, influx->port, influx->database);
	}

	DEBUG_PRINTF("Connecting to: [%s]\n", influx->uri);

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, influx->uri);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, function_pt);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
	if (NULL != influx->token) {
		curl_easy_setopt(curl, CURLOPT_XOAUTH2_BEARER, influx->token);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BEARER);
	}

	result = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	return (result);
}

#ifdef CBSD
int
influx_cmd(int argc, char **argv)
{
	int rc = 0, c;
	unsigned int work_flags = 0;

	size_t buffer_size = 0;
	char *data;

	for (c = 0; c < argc; c++)
		buffer_size += strlen(
		    argv[c]); // This should be more the enough :-)

	if (!(data = malloc(buffer_size)))
		return (-1);
	bzero(data, buffer_size); // BE zero!

	optind = 1; // If we do not do this it fails the 2nd time you try in in
		    // the same session.
	while (rc == 0 && (c = getopt(argc, argv, "t:v:m:")) >= 0) {
		switch (c) {
		case 'm': /* measurement */
			if (1 & work_flags) {
				printf(
				    "You should only select one measurement per line for now!\n");
				rc = -1;
				break;
			}
			work_flags |= 1;
			// TODO: check for valid string?
			sprintf(data, "%s,node=%s", optarg,
			    lookupvar("nodename"));
			break;

		case 't': /* tag=value */
		case 'v': /* key=value */
			if (!(1 & work_flags)) {
				printf("%s: Start with -m measurement!\n",
				    argv[0]);
				rc = -1;
				break;
			}

			if (c == 't') {
				if (4 & work_flags) {
					printf(
					    "%s: Already started using -v, don't use -t after -v please.\n",
					    argv[0]);
					rc = -1;
					break;
				}
				work_flags |= 2;
				sprintf(data + strlen(data), ",%s", optarg);
			} else {
				if (!(4 & work_flags)) {
					data[strlen(data)] = ' ';
					work_flags |= 4;
				} else
					data[strlen(data)] = ',';
				sprintf(data + strlen(data), "%s", optarg);
			}
			break;

			//			case 'd': /* overwrite database
			//*/ 				break;

		default:
			printf(
			    "Invalid parameters\nUse %s -m measurement [-t tag=val [-t tag=val]...] -v key=val [-v key=val]...",
			    argv[0]);
			rc = -1;
		}
	}

	if (rc == 0) {
		DEBUG_PRINTF("DEBUG[Influx-Out]: %s\n", data);
		rc = cbsd_influx_do(data);
	}
	free(data);

	if (rc == CURLE_OK)
		return (0);
	DEBUG_PRINTF("CURL returned error: %i\n", rc);
	return (1);
}
#else // Not CBSD but RACCT
int
cbsd_influx_transmit_buffer()
{
	if (NULL == influx || NULL == influx->buffer || 0 == influx->items)
		return (0); // We call it a success..

	int rc = cbsd_influx_do(influx->buffer);

	if (rc == 0) {
		influx->items = 0;
		influx->buffer[0] = 0;		      // reset length..
	} else if (rc == 400 || influx->items > 20) { // Drop them anyway
		influx->items = 0;
		influx->buffer[0] = 0; // reset length..

		if (rc == 400) {
			fprintf(stderr,
			    "Warning, dropping influx data because of errors!\n");
		} else {
			fprintf(stderr,
			    "Warning, dropping influx data because of overflow!\n");
		}
	}
	return (rc);
}
#endif

int
cbsd_influx_init(void)
{
	if ((influx = malloc(sizeof(cbsdinflux_t))) == NULL)
		return (-1);
	bzero(influx, sizeof(cbsdinflux_t));

#ifndef CBSD
	// TODO fix buffer size or make it grow if needed.

	if ((influx->buffer = malloc(8192)) == NULL) {
		free(influx);
		influx = NULL;
		return (-1);
	}			     // We don't have a buffer, we fail!
	bzero(influx->buffer, 8192); // At least start fresh..
#endif
	return (0);
}

void
cbsd_influx_free(void)
{
	if (!influx)
		return;

#ifndef CBSD
	if (NULL != influx->buffer) {
		if (influx->items > 0)
			cbsd_influx_transmit_buffer();
		free(influx->buffer);
	}
	if (NULL != influx->tables.bhyve)
		free(influx->tables.bhyve);
	if (NULL != influx->tables.jails)
		free(influx->tables.jails);
	if (NULL != influx->tables.nodes)
		free(influx->tables.nodes);
	if (NULL != influx->tags.bhyve)
		free(influx->tags.bhyve);
	if (NULL != influx->tags.jails)
		free(influx->tags.jails);
	if (NULL != influx->tags.nodes)
		free(influx->tags.nodes);

#endif
	if (NULL != influx->uri)
		free(influx->uri);
	if (NULL != influx->token)
		free(influx->token);
	if (NULL != influx->hostname)
		free(influx->hostname);
	if (NULL != influx->database)
		free(influx->database);

	free(influx);
	influx = NULL;
}
