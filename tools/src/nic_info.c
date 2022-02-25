/* Part of CBSD Project
Obtain info for NIC

  --mtu: return MTU size
  --phys: return 1, 0 (physical or virtual iface?)
  --media: return media
*/

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <net/if.h>

#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FALSE 0
#define TRUE 1

/* List of all args */
enum {
	C_HELP,
	C_MEDIA,
	C_MTU,
	C_NIC,
	C_PHYS,
	C_QUIET,
};

static void
usage(void)
{
	printf("Obtain NIC information\n");
	printf("require: --nic\n");
	printf("optional: --quiet --mtu --phys --media\n");
	printf("usage: nic_info --nic=em0 --mtu\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	int s, af = AF_INET;
	struct ifreq ifr;
	char *nic = NULL;
	char *nic_family = NULL;
	char *tmpstr = NULL;
	int optcode = 0;
	int option_index = 0;

	int show_media = 0;
	int show_mtu = 0;
	int show_phys = 0;
	int quiet = 0;
	int phys = 0;
	int mtu = 0;
	int media = 0;
	int nic_id = 0;
	int i = 0;
	int x = 0;
	int y = 0;
	int error = 0;
	size_t len = 0;

	static struct option long_options[] = { { "help", no_argument, 0,
						    C_HELP },
		{ "media", no_argument, 0, C_MEDIA },
		{ "mtu", no_argument, 0, C_MTU },
		{ "nic", required_argument, 0, C_NIC },
		{ "phys", no_argument, 0, C_PHYS },
		{ "quiet", no_argument, 0, C_QUIET },
		/* End of options marker */
		{ 0, 0, 0, 0 } };

	while (TRUE) {
		optcode = getopt_long_only(argc, argv, "", long_options,
		    &option_index);
		if (optcode == -1)
			break;
		switch (optcode) {
		case C_HELP:
			usage();
			break;
		case C_MTU:
			show_mtu = 1;
			break;
		case C_MEDIA:
			show_media = 1;
			break;
		case C_NIC:
			i = strlen(optarg) + 1;
			nic = malloc(i);
			tmpstr = malloc(i);
			nic_family = malloc(i);
			memset(nic, 0, i);
			memset(tmpstr, 0, i);
			memset(nic_family, 0, i);
			strcpy(nic, optarg);
			x = 0;
			y = 0;
			for (i = 0; i < strlen(nic); i++)
				if ((nic[i] >= 48) && (nic[i] <= 57)) {
					tmpstr[x] = nic[i];
					x++;
				} else {
					nic_family[y] = nic[i];
					y++;
				}
			nic_id = atoi(tmpstr);
			free(tmpstr);
			break;
		case C_PHYS:
			show_phys = 1;
			break;
		case C_QUIET:
			quiet = 1;
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

	if (show_media == 1)
		if (ioctl(s, SIOCGIFMEDIA, (caddr_t)&ifr) < 0)
			err(1, "ioctl (get media)");

	if (show_mtu == 1)
		if (ioctl(s, SIOCGIFMTU, (caddr_t)&ifr) < 0)
			err(1, "ioctl (get mtu)");

	if (show_phys == 1) {
		i = strlen(nic) + 30;
		tmpstr = malloc(i); // extra dev.<nic>.0.%parent
		memset(tmpstr, 0, i);
		sprintf(tmpstr, "dev.%s.%d.%%parent", nic_family, nic_id);
		error = sysctlbyname(tmpstr, NULL, &len, NULL, 0);
		if (error != 0) {
			phys = 0;
		} else {
			phys = 1;
		}
		free(tmpstr);
	}

	close(s);

	if (quiet) {
		if (show_media == 1)
			fprintf(stdout, "%d\n", ifr.ifr_media);
		if (show_mtu == 1)
			fprintf(stdout, "%d\n", ifr.ifr_mtu);
		if (show_phys == 1)
			fprintf(stdout, "%d\n", phys);
	} else {
		if (show_media == 1)
			fprintf(stdout, "media:%d\n", ifr.ifr_media);
		if (show_mtu == 1)
			fprintf(stdout, "mtu:%d\n", ifr.ifr_mtu);
		if (show_phys == 1)
			fprintf(stdout, "phys:%d\n", phys);
	}

	return (0);
}
