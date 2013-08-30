//
//	Send Message to node
//	Part of CBSD Project
//
#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <ctype.h>
#include <err.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFPORT "8106"

static void	cbsd_sendmsg(int, const char *, const char *, const char *,
			   const char *);
static void	usage(void);

struct socks {
    int sock;
    int addrlen;
    struct sockaddr_storage addr;
};

#ifdef INET6
static int family = PF_UNSPEC;	/* protocol family (IPv4, IPv6 or both) */
#else
static int family = PF_INET;	/* protocol family (IPv4 only) */
#endif

int
main(int argc, char *argv[])
{
	int ch, logflags, pri;
	char *tag, *host, buf[1024];
	const char *svcname;

	if (!strcmp(argv[1],"--help")) usage();

	tag = NULL;
	host = NULL;
//	svcname = "1500"; //port number
	svcname=DEFPORT;
	logflags = 0;
	unsetenv("TZ");
	while ((ch = getopt(argc, argv, "46h:P:t:p:n:")) != -1)
		switch((char)ch) {
		case '4':
			family = PF_INET;
			break;
#ifdef INET6
		case '6':
			family = PF_INET6;
			break;
#endif
		case 'h':		/* hostname to deliver to */
			host = optarg;
			break;
		case 'P':		/* service name or port number */
			svcname = optarg;
			break;
		case 't':               /* tag */
			tag = optarg;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (tag == NULL) tag="localnodename";

	/* setup for logging */
	if (host == NULL) {
		printf("host or ip is required\n");
		exit(0);
		}

	/* log input line if appropriate */
	if (argc > 0) {
		char *p, *endp;
		size_t len;

		for (p = buf, endp = buf + sizeof(buf) - 2; *argv;) {
			len = strlen(*argv);
			if (p + len > endp && p > buf) {
				cbsd_sendmsg(pri, tag, host, svcname, buf);
				p = buf;
			}
			if (len > sizeof(buf) - 1)
				cbsd_sendmsg(pri, tag, host, svcname, *argv++);
			else {
				if (p != buf)
					*p++ = ' ';
				bcopy(*argv++, p, len);
				*(p += len) = '\0';
			}
		}
		if (p != buf)
			cbsd_sendmsg(pri, tag, host, svcname, buf);
	}
	exit(0);
}

static void
cbsd_sendmsg(int pri, const char *tag, const char *host, const char *svcname,
	   const char *buf)
{
	static struct socks *socks;
	static int nsock = 0;
	struct addrinfo hints, *res, *r;
	char *line;
	int maxs, len, sock, error, i, lsent;

	if (nsock <= 0) {	/* set up socket stuff */
		/* resolve hostname */
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = family;
		hints.ai_socktype = SOCK_DGRAM;
		error = getaddrinfo(host, svcname, &hints, &res);

		if (error)
			errx(1, "%s: %s", gai_strerror(error), host);
		/* count max number of sockets we may open */
		for (maxs = 0, r = res; r; r = r->ai_next, maxs++);
		socks = malloc(maxs * sizeof(struct socks));
		if (!socks)
			errx(1, "couldn't allocate memory for sockets");
		for (r = res; r; r = r->ai_next) {
			sock = socket(r->ai_family, r->ai_socktype,
				      r->ai_protocol);
			if (sock < 0)
				continue;
			memcpy(&socks[nsock].addr, r->ai_addr, r->ai_addrlen);
			socks[nsock].addrlen = r->ai_addrlen;
			socks[nsock++].sock = sock;
		}
		freeaddrinfo(res);
		if (nsock <= 0)
			errx(1, "socket");
	}

	if ((len = asprintf(&line, "%s:%s", tag, buf)) == -1)
		errx(1, "asprintf");

	lsent = -1;
	for (i = 0; i < nsock; ++i) {
		lsent = sendto(socks[i].sock, line, len, 0,
			       (struct sockaddr *)&socks[i].addr,
			       socks[i].addrlen);
		if (lsent == len)
			break;
	}
	if (lsent != len) {
		if (lsent == -1)
			warn ("sendto");
		else
			warnx ("sendto: short send - %d bytes", lsent);
	}

	free(line);
}

static void usage()
{
    printf("send udp message to nodemsgd on remote host\n");
    printf("nodemsg [-46Ais] [-n nodename] [-h host] [-P port] [-t tag] message\n");
    exit(0);
}
