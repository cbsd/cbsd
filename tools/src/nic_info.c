/* Part of CBSD Project
   Obtain info for NIC

  --mtu: return MTU size
  --phys: return 1, 0 (physical or virtual iface?)
  --media: return media

  FreeBSD: media uses ifr.ifr_media (IFM_xxx encoding)
  Linux: media returns link speed in Mbps via ethtool
*/

#if defined(__FreeBSD__)
#include <sys/sysctl.h>
#endif

#if defined(__linux__)
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <sys/stat.h>
#endif

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

#if defined(__FreeBSD__)
static int
get_media_freebsd(int s, struct ifreq *ifr)
{
	if (ioctl(s, SIOCGIFMEDIA, (caddr_t)ifr) < 0)
		return -1;

	return (ifr->ifr_media);
}

static int
is_physical_freebsd(const char *nic_family, int nic_id)
{
	char sysctl_name[256];
	size_t len = 0;

	snprintf(sysctl_name, sizeof(sysctl_name),
	    "dev.%s.%d.%%parent", nic_family, nic_id);

	if (sysctlbyname(sysctl_name, NULL, &len, NULL, 0) != 0)
		return 0;

	return 1;
}
#endif

#if defined(__linux__)
static int
get_media_linux(int s, struct ifreq *ifr)
{
	struct ethtool_cmd ecmd;

	memset(&ecmd, 0, sizeof(ecmd));
	ecmd.cmd = ETHTOOL_GSET;
	ifr->ifr_data = (char *)&ecmd;

	if (ioctl(s, SIOCETHTOOL, ifr) < 0)
		return -1;

	return (int)ethtool_cmd_speed(&ecmd);
}

static int
is_physical_linux(const char *nic)
{
	char path[256];
	struct stat sb;

	snprintf(path, sizeof(path), "/sys/class/net/%s/device", nic);
	if (stat(path, &sb) == 0)
		return 1;

	return 0;
}
#endif

int
main(int argc, char *argv[])
{
	int s;
	struct ifreq ifr;
	char *nic = NULL;
#if defined(__FreeBSD__)
	char *nic_family = NULL;
	int nic_id = 0;
#endif
	int optcode = 0;
	int option_index = 0;

	int show_media = 0;
	int show_mtu = 0;
	int show_phys = 0;
	int quiet = 0;
	int phys = 0;
	int media = 0;
	int mtu = 0;

	static struct option long_options[] = {
		{ "help", no_argument, 0, C_HELP },
		{ "media", no_argument, 0, C_MEDIA },
		{ "mtu", no_argument, 0, C_MTU },
		{ "nic", required_argument, 0, C_NIC },
		{ "phys", no_argument, 0, C_PHYS },
		{ "quiet", no_argument, 0, C_QUIET },
		{ 0, 0, 0, 0 }
	};

	while (1) {
		optcode = getopt_long(argc, argv, "", long_options,
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
			nic = strdup(optarg);
			if (nic == NULL)
				err(1, "strdup");
#if defined(__FreeBSD__)
			{
				int i, x = 0, y = 0;
				char *tmpstr;
				int len = strlen(nic) + 1;

				nic_family = calloc(1, len);
				tmpstr = calloc(1, len);
				if (nic_family == NULL || tmpstr == NULL)
					err(1, "calloc");

				for (i = 0; i < (int)strlen(nic); i++) {
					if (nic[i] >= '0' && nic[i] <= '9')
						tmpstr[x++] = nic[i];
					else
						nic_family[y++] = nic[i];
				}
				nic_id = atoi(tmpstr);
				free(tmpstr);
			}
#endif
			break;
		case C_PHYS:
			show_phys = 1;
			break;
		case C_QUIET:
			quiet = 1;
			break;
		}
	}

	if (!nic)
		usage();

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err(1, "socket");

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_addr.sa_family = AF_INET;
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", nic);

	if (show_media == 1) {
#if defined(__FreeBSD__)
		media = get_media_freebsd(s, &ifr);
#elif defined(__linux__)
		media = get_media_linux(s, &ifr);
#else
		media = -1;
#endif
		if (media < 0)
			err(1, "ioctl (get media)");
	}

	if (show_mtu == 1) {
		if (ioctl(s, SIOCGIFMTU, (caddr_t)&ifr) < 0)
			err(1, "ioctl (get mtu)");
		mtu = ifr.ifr_mtu;
	}

	if (show_phys == 1) {
#if defined(__FreeBSD__)
		phys = is_physical_freebsd(nic_family, nic_id);
#elif defined(__linux__)
		phys = is_physical_linux(nic);
#else
		phys = 0;
#endif
	}

	close(s);

	if (quiet) {
		if (show_media == 1)
			fprintf(stdout, "%d\n", media);
		if (show_mtu == 1)
			fprintf(stdout, "%d\n", mtu);
		if (show_phys == 1)
			fprintf(stdout, "%d\n", phys);
	} else {
		if (show_media == 1)
			fprintf(stdout, "media:%d\n", media);
		if (show_mtu == 1)
			fprintf(stdout, "mtu:%d\n", mtu);
		if (show_phys == 1)
			fprintf(stdout, "phys:%d\n", phys);
	}

	free(nic);
#if defined(__FreeBSD__)
	free(nic_family);
#endif

	return (0);
}
