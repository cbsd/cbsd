//CBSD Project
// olevole@olevole.ru
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>

int main(int argc, char **argv)
{
	if (argc!=3) {
		printf("Usage: resolv <proto: 4|6> <hostname>\r\n");
		exit(0);
	}

	int proto = atoi(argv[1]);

	char *source=argv[2];
	struct hostent *hp;
	struct hostent *hp6;

	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;

	char text_addr6[INET6_ADDRSTRLEN];
	char *text_addr4;

	if ( proto == 6 ) {
		inet_pton(AF_INET6,source,sin6.sin6_addr.s6_addr);
		hp6 = gethostbyname2(source,AF_INET6);
	} else {
		hp6 = NULL;
	}

	if (hp6 == NULL) {
		//try ipv4
		hp = gethostbyname2(source, AF_INET);
		if (!hp) {
			printf("cannot resolve %s: %s\r\n",source, hstrerror(h_errno));
			exit(1);
		}

		sin.sin_len = sizeof sin;
		if ((unsigned)hp->h_length > sizeof(sin.sin_addr) || hp->h_length < 0) {
			printf("gethostbyname2: illegal address for %s\r\n",source);
			exit(1);
		}

		memcpy(&sin.sin_addr, hp->h_addr_list[0], sizeof(sin.sin_addr));
		text_addr4 = inet_ntoa(sin.sin_addr);
		printf("%s\n", text_addr4);
	} else {
		sin.sin_len = sizeof sin6;
		if ((unsigned)hp6->h_length > sizeof(sin6.sin6_addr) || hp6->h_length < 0) {
			printf("gethostbyname2: illegal address for %s\r\n",source);
			exit(1);
		}

		memset((char *) &sin6, 0, sizeof(sin6));
		sin6.sin6_flowinfo = 0;
		sin6.sin6_family = AF_INET6;
		memmove((char *) &sin6.sin6_addr.s6_addr, (char *) hp6->h_addr, hp6->h_length);

		inet_ntop(AF_INET6, &sin6.sin6_addr, text_addr6,sizeof(text_addr6));
		printf("%s\n", text_addr6);
	}

	return 0;
}
