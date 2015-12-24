#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char **argv)
{
	if (argc!=2) {
		printf("Usage: resolv <hostname>\r\n");
		exit(0);
	}

	char *source=argv[1];
	struct hostent *hp;

	struct sockaddr_in sock_in;
	char *text_addr;

	hp = gethostbyname2(source, AF_INET);
	if (!hp) {
		printf("cannot resolve %s: %s\r\n",source, hstrerror(h_errno));
		exit(1);
	}

	sock_in.sin_len = sizeof sock_in;
	if ((unsigned)hp->h_length > sizeof(sock_in.sin_addr) || hp->h_length < 0) {
		printf("gethostbyname2: illegal address for %s\r\n",source);
		exit(1);
	}

	memcpy(&sock_in.sin_addr, hp->h_addr_list[0], sizeof(sock_in.sin_addr));
	text_addr = inet_ntoa(sock_in.sin_addr);
	printf("%s\n", text_addr);
}
