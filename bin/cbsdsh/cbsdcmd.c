/* cbsdcmd.c - Influx functions for CBSDSH
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

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cbsdredis.h"
#include "contrib/ini.h"
#include "mystring.h"
#include "output.h"
#include "sqlcmd.h"
#include "var.h"

#ifdef DEBUG_CMD
#define DEBUG_PRINTF(...) fprintf(stderr, __VA_ARGS__);
#else
#define DEBUG_PRINTF(...)
#endif

int
cbsd_node_ls(int argc, char **argv)
{
	int rc, output = 0; // Console or JSON
	char *_nodeop[1] = { "node:*" };

	int valc = 0;
	char **valv = NULL;
	const char *nodename = getenv("HOST");

	if (argc > 0) {
		if (strcmp(argv[0], "--json") == 0)
			output = 1;
		else if (strcmp(argv[0], "--xml") == 0)
			output = 3;
		else {
			fprintf(stderr,
			    "Invalid parameters! only --json | --xml will work!\n");
			return (2);
		}
	}

	// Get a list of all nodes from Redis
	rc = redis_do("KEYS", CR_MULTIBULK, RF_ARRAY, 1, _nodeop, &valc,
	    (char **)&valv);
	if (rc == 0) {
		if (valc == 0) {
			fprintf(stderr,
			    "There are no nodes found in Redis, this should not happen!\n");
			free(valv);
			return (5); // todo: check error
		}

		if (output == 0)
			out1fmt(
			    "\033[0;33mNode                    IP              State        ARCH          RAM       CPU           Jails   VMs\033[0m\n");
		else if (output == 1)
			out1fmt("[");
		else if (output == 3)
			out1fmt("<nodes>");

		// enum results..
		for (int i = 0; i < valc; i++) {
			const char *node_name = valv[i] +
			    5; // Skip the 'node:' part
			const char *node_arch = "Unknown", *node_ip = NULL,
				   *node_state = NULL, *row_color = NULL;
			unsigned int node_id = 0, node_jails = 0, node_vms = 0,
				     node_cpu = 0, node_ram = 0;
			char numbuf[20] = "                    ";

			// Get node information
			int nodevc = 0;
			char **nodevv = NULL;
			rc = redis_do("HGETALL", CR_MULTIBULK,
			    RF_KEYLIST | RF_ARRAY, 1, &valv[i], &nodevc,
			    (char **)&nodevv);
			if (rc == 0) {
				row_color = "\033[0;39m";

				//				out1fmt("Found node:
				//%s\n",valv[i]);
				for (int j = 0; j < nodevc; j++) {
					if (strcmp(nodevv[j], "ip") == 0)
						node_ip = nodevv[j] +
						    strlen(nodevv[j]) + 1;
					else if (strcmp(nodevv[j], "id") == 0)
						node_id = atoi(nodevv[j] +
						    strlen(nodevv[j]) + 1);
					else if (strcmp(nodevv[j], "jails") ==
					    0)
						node_jails = atoi(nodevv[j] +
						    strlen(nodevv[j]) + 1);
					else if (strcmp(nodevv[j], "vms") == 0)
						node_vms = atoi(nodevv[j] +
						    strlen(nodevv[j]) + 1);
					else if (strcmp(nodevv[j], "cpu") == 0)
						node_cpu = atoi(nodevv[j] +
						    strlen(nodevv[j]) + 1);
					else if (strcmp(nodevv[j], "mem") == 0)
						node_ram = atoi(nodevv[j] +
						    strlen(nodevv[j]) + 1);
					else if (strcmp(nodevv[j], "arch") == 0)
						node_arch = nodevv[j] +
						    strlen(nodevv[j]) + 1;
					else if (strcmp(nodevv[j], "state") ==
					    0)
						node_state = nodevv[j] +
						    strlen(nodevv[j]) + 1;
					else
						out1fmt("-%s = %s\n", nodevv[j],
						    nodevv[j] +
							strlen(nodevv[j]) + 1);
				}
				if (node_ip == NULL) {
					fprintf(stderr,
					    "Incomplete node in redis, this should not happen!\n");
				} else if (output == 0) {
					const char *state_color =
					    "\033[1;33m"; // Yellow
					if (!node_state)
						node_state = "unknown";
					if (strcmp(node_state, "up") == 0)
						state_color =
						    "\033[0;32m"; // Green
					else if (strcmp(node_state, "down") ==
					    0)
						state_color =
						    "\033[0;31m"; // Red

					if (strlen(node_name) ==
						strlen(nodename) &&
					    strcmp(node_name, nodename) == 0)
						row_color = "\033[0;35m";

					sprintf(numbuf, "%i%%", node_ram);
					numbuf[strlen(numbuf)] = ' ';
					sprintf(numbuf + 10, "%i%%", node_cpu);

					out1fmt(
					    "%s%-24s%-16s%s%-12s\033[0m %s%-14s%s\t\t%i\t%i\033[0m\n",
					    row_color, node_name, node_ip,
					    state_color, node_state, row_color,
					    node_arch, numbuf, node_jails,
					    node_vms);
				} else if (output < 3) {
					out1fmt(
					    "%s{\"node_id\":%i, \"node_name\":\"%s\",\"node_ip\":\"%s\",\"node_ram\":%i,\"node_cpu\":%i,\"node_jails\":%i,\"node_vms\":%i,\"state\":\"%s\",\"arch\":\"%s\"}",
					    (output > 1 ? "," : ""), node_id,
					    node_name, node_ip, node_ram,
					    node_cpu, node_jails, node_vms,
					    node_state, node_arch);
					if (output == 1)
						output = 2; // add a ,
				} else if (output == 3) {
					out1fmt(
					    "<node><id>%i</id><name>%s</name><ip>%s</ip><ram>%i</ram><cpu>%i</cpu><jails>%i</jails><vms>%i</vms><state>%s</state><arch>%s</arch></node>",
					    node_id, node_name, node_ip,
					    node_ram, node_cpu, node_jails,
					    node_vms, node_state, node_arch);
				}
				free(nodevv);
			} else {
				rc = -1;
			}
		}
		free(valv);

		if (rc != 0) { // Use SQL
			fprintf(stderr,
			    "\033[0;35mWARNING!\033[0m Problems with Redis, you should look into that!\n");

			// Try SQL?
		}
		switch (output) {
		case 1:
		case 2:
			out1fmt("]\n");
			break;
		case 3:
			out1fmt("</nodes>\n");
			break;
		}
		return (0);
	}
	fprintf(stderr,
	    "\033[0;35mWARNING!\033[0m Problems with Redis, you should look into that!\n");
	return (5); // todo:check!
}

int
cbsd_node_cmd(int argc, char **argv)
{
	if (argc < 1)
		return (8);

	if (strcmp(argv[0], "ls") == 0)
		return (cbsd_node_ls(argc - 1, &argv[1]));
	return (8);
}

int
cbsd_cmd(int argc, char **argv)
{
	int rc = 8;
	if (argc < 2)
		return (8);

	if (strcmp(argv[1], "node") == 0)
		rc = cbsd_node_cmd(argc - 2, &argv[2]);
	//	elseif(strcmp(argv[1], "jail") == 0) rc=cbsd_jail_cmd(argc-2,
	//&argv[2]);

	switch (rc) {
	case 8:
		fprintf(stderr, "Invalid or missing parameters.\n");
		break;
	}
	return (rc);
}

// int cbsd_cmd_init(void){
//
// }

// void cbsd_cmd_free(void){
//
// }
