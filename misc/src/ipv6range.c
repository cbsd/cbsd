#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int
main(int argc, char *argv[])
{
	struct in6_addr sn;
	struct in6_addr en;
	char output[64];
	int octet;

	if (argc != 3) {
		printf("usage: v6start v6end\n");
		return 1;
	}

	const char *s = argv[1];
	const char *e = argv[2];

	inet_pton(AF_INET6, s, &sn);
	inet_pton(AF_INET6, e, &en);

	for (;;) {
		/* print the address */
		if (!inet_ntop(AF_INET6, &sn, output, sizeof(output))) {
			perror("inet_ntop");
			break;
		}

		printf("%s\n", output);

		/* break if we hit the last address or (sn > en) */
		if (memcmp(sn.s6_addr, en.s6_addr, 16) >= 0) {
			break;
		}

		/* increment sn, and move towards en */
		for (octet = 15; octet >= 0; --octet) {
			if (sn.s6_addr[octet] < 255) {
				sn.s6_addr[octet]++;
				break;
			}
			sn.s6_addr[octet] = 0;
		}

		if (octet < 0) {
			break; /* top of logical address range */
		}
	}
	return 0;
}
