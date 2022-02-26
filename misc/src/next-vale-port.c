// CBSD Project 2013-2020
// CBSD Team <cbsd+subscribe@lists.tilda.center>
// 0.1
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <net/netmap_user.h>
#include <net/netmap.h>
#include <errno.h>

#define is_digit(c) ((unsigned int)((c) - '0') <= 9)

void
nm_ioctl(struct nmreq_header *hdr)
{
	int fd = open("/dev/netmap", O_RDWR);

	if (fd < 0) {
		fprintf(stderr, "unable to open /dev/netmap\n");
		exit(1);
	}

	if (ioctl(fd, NIOCCTRL, hdr) < 0) {
		int err = errno;
		close(fd);

		if (hdr->nr_reqtype == NETMAP_REQ_VALE_LIST && err == ENOENT)
			return;
		else
			fprintf(stderr, "netmap NIOCTRL\n");
		exit(1);
	}
}

static void
usage(int errcode)
{
	fprintf(stderr,
	    "Return next free VALE port. Usage:\n"
	    "\t-e exclude \"1 2 3\"\n"
	    "\t-i target VALE id (0 1 ..)\n");
	exit(errcode);
}

int
is_number(const char *p)
{
	const char *q;

	if (*p == '\0')
		return 0;
	while (*p == '0')
		p++;
	for (q = p; *q != '\0'; q++)
		if (!is_digit(*q))
			return 0;
	if (q - p > 10000)
		return 0;
	return 1;
}

// return 1 if port in exclude_list
// in_exclude_list(2,"1 2 3");
int
in_exclude_list(int port, char *exclude_list)
{
	int exist = 0;
	int tmp = 0;
	char *pch;
	char ex[strlen(exclude_list)];
	strcpy(ex, exclude_list);
	pch = strtok(ex, " ,.-");
	while (pch != NULL) {
		tmp = atoi(pch);
		if (tmp == port)
			return 1;
		pch = strtok(NULL, " ,.-");
	}
	return 0;
}

int
main(int argc, char *argv[])
{
	struct nmreq_header hdr;
	struct nmreq_vale_list req;
	int i = 0, j = 0, x = 0, sw_id;
	char *token = NULL;
	int target_vale_id = -1;
	int ch, nr_cmd = 0, nr_arg = 0;
	int port;
	int switch_found;
	int first_free = -1;
	char myswitch[7]; // "vale99" max
	int switch_ports[255];
	char *exclude_list = NULL;

	while ((ch = getopt(argc, argv, "e:i:")) != -1) {
		switch (ch) {
		case 'e':
			exclude_list = optarg;
			break;
		case 'i':
			target_vale_id = atoi(optarg);
			if ((target_vale_id < 0) || (target_vale_id > 99)) {
				fprintf(stderr, "id not in range 0-99: %d\n",
				    target_vale_id);
				exit(-1);
			}
			break;
		default:
			fprintf(stderr, "bad option %c %s", ch, optarg);
			usage(-1);
			break;
		}
	}

	if (optind != argc) {
		// fprintf(stderr, "optind %d argc %d\n", optind, argc);
		usage(-1);
	}

	fprintf(stderr, "Search in vale name: %d\n", target_vale_id);

	sprintf(myswitch, "vale%d", target_vale_id);
	fprintf(stderr, "Search for SW: [%s]\n", myswitch);

	// mark exclude ports first
	if (exclude_list) {
		for (i = 0; i < 255; i++) {
			switch_ports[i] = in_exclude_list(i, exclude_list);
		}
	} else {
		for (i = 0; i < 255; i++)
			switch_ports[i] = 0;
	}

	// scan for 255 switch ID
	for (sw_id = 0; sw_id < 64; sw_id++) {
		memset(&hdr, 0, sizeof(hdr));

		hdr.nr_version = NETMAP_API;
		hdr.nr_reqtype = NETMAP_REQ_VALE_LIST;
		hdr.nr_body = (uintptr_t)&req;
		req.nr_bridge_idx = sw_id;
		req.nr_port_idx = 0;

		nm_ioctl(&hdr);

		if (hdr.nr_name[0] == 0) {
			fprintf(stderr, "no such vale id: %d\n", sw_id);
			continue;
		}

		fprintf(stderr, "bridge ID: [vale%d]\n", sw_id);

		// scan for 255 ports per SW
		for (i = 0; i < 255; i++) {

			switch_found = 0;
			memset(&hdr, 0, sizeof(hdr));

			hdr.nr_version = NETMAP_API;
			hdr.nr_reqtype = NETMAP_REQ_VALE_LIST;
			hdr.nr_body = (uintptr_t)&req;
			req.nr_bridge_idx = sw_id;
			req.nr_port_idx = i;

			nm_ioctl(&hdr);

			if (hdr.nr_name[0] == 0 || req.nr_port_idx != i) {
				continue;
				//				break;
			}

			for (j = 0, token = strtok(hdr.nr_name, ":"); token;
			     j++, token = strtok(NULL, ":")) {
				switch (j) {
				case 0:
					fprintf(stderr, "switch name: %s\n",
					    token);
					if (!strcmp(token, myswitch)) {
						fprintf(stderr,
						    "  switch found!\n");
						switch_found = 1;
					} else {
						fprintf(stderr,
						    "  switch NOT found ([%s][%s]!\n",
						    token, myswitch);
						switch_found = 0;
					}
					break;
					;
					;
				case 1:
					if (switch_found == 0)
						continue;
					fprintf(stderr, "  port name: %s\n",
					    token);
					if (!is_number(token)) {
						fprintf(stderr,
						    "    not number, skip: %s\n",
						    token);
						continue;
					}
					port = atoi(token);
					// mark port as use
					switch_ports[port] = 1;
					fprintf(stderr,
					    "    already in use: %d\n", port);
					continue;
					break;
					;
					;
				default:
					fprintf(stderr,
					    "    unknown config: %s\n", token);
					break;
				}
			}
		}
	}
	fprintf(stderr, "SWITCH MAP:\n");
	for (i = 0; i < 255; i++) {
		if ((switch_ports[i] == 0) && (first_free < 0))
			first_free = i;
		fprintf(stderr, "%d ", switch_ports[i]);
	}
	fprintf(stderr, "\n");
	if (first_free > -1) {
		printf("%d", first_free);
		exit(0);
	}

	exit(1);
}
