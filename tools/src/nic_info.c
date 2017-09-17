#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <err.h>
#include <getopt.h>

#define FALSE 0
#define TRUE 1

/* List of all args */
enum {
	C_HELP,
	C_MTU,
	C_NIC,
	C_QUIET,
};

static void
usage(void)
{
	printf("Obtain NIC information\n");
	printf("require: --nic\n");
	printf("optional: --quiet --mtu\n");
	printf("usage: nic_info --nic=em0 --mtu\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	int s, af = AF_INET;
	struct ifreq ifr;
	char *nic = NULL;
	int optcode = 0;
	int option_index = 0;

	int show_mtu=0;
	int quiet=0;

	static struct option long_options[] = {
		{"help", no_argument, 0, C_HELP},
		{"mtu", no_argument, 0, C_MTU},
		{"nic", required_argument, 0, C_NIC},
		{"quiet", no_argument, 0, C_QUIET},
		/* End of options marker */
		{0, 0, 0, 0}
	};

	while (TRUE) {
		optcode = getopt_long_only(argc, argv, "", long_options, &option_index);
		if (optcode == -1)
			break;
		switch(optcode) {
			case C_HELP:
				usage();
				break;
			case C_MTU:
				show_mtu=1;
				break;
			case C_NIC:
				nic = malloc(strlen(optarg) + 1);
				memset(nic, 0, strlen(optarg) + 1);
				strcpy(nic, optarg);
				break;
			case C_QUIET:
				quiet=1;
				break;
		}
	}

	if (!nic) {
		usage();
	}

	if ((s = socket(af, SOCK_DGRAM, 0)) < 0)
		err(1, "socket");

	ifr.ifr_addr.sa_family = AF_INET;
	strcpy(ifr.ifr_name, nic);
	if (ioctl(s, SIOCGIFMTU, (caddr_t)&ifr) < 0)
		err(1,"ioctl (get mtu)");

	close(s);

	if ( quiet ) {
		fprintf(stdout, "%d\n",ifr.ifr_mtu);
	} else {
		fprintf(stdout, "mtu:%d\n", ifr.ifr_mtu);
	}

	return(0);
}
