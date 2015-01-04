// detect for ip
// collision by "arp/ndp delete -> ping -> arg/ndp get" sequence
//
//This is part of the CBSD Project
//
//return 1 if records exist
// return 0 if not
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>

#include <getopt.h>
#include <stdarg.h> //for debugmsg/errmsg

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>

#include <sys/param.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/sysctl.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/if_types.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>

#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <nlist.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define PINGNUM	3
#define PINGTIMEOUT	0.5
#define PACKETSIZE	64
#define DEFDATALEN	56	/* default data length */

#define IP6_HDRLEN 40         // IPv6 header length
#define ICMP_HDRLEN 8         // ICMP header length for echo request, excludes data

#ifdef __DragonFly__
// net/route.h
#define RTF_LLDATA  0x400           /* used by apps to add/del L2 entries */
#define SA_SIZE(sa)                                             \
    (  (!(sa) || ((struct sockaddr *)(sa))->sa_len == 0) ?      \
        sizeof(long)            :                               \
        1 + ( (((struct sockaddr *)(sa))->sa_len - 1) | (sizeof(long) - 1) ) )
#endif

useconds_t	pingtimeout = 0;
int		pingnum = 0;
char		testip    [40];
//max lenght of IPv6 records
int		debug = 0;
int		ipv6 = 0;

#define FALSE 0
#define TRUE 1

/* List of all cmd */
enum {
	C_PINGNUM,
	C_IP,
	C_PINGTIMEOUT,
	C_HELP,
	C_DEBUG,
};


int		ident = -1;
struct protoent *proto = NULL;
u_char		outpackhdr[IP_MAXPACKET], *outpack;
int		phdr_len = 0;
int		datalen = DEFDATALEN;
u_char		icmp_type_rsp = ICMP_ECHOREPLY;
int		ping       (struct sockaddr_in *);
static int	flags, doing_proxy;
static time_t	expire_time;
static int	delete(struct sockaddr_in *);
static int	valid_type(int);
static struct rt_msghdr *rtmsg(int, struct sockaddr_in *, struct sockaddr_dl *);
typedef void    (action_fn) (struct sockaddr_dl *, struct sockaddr_in *, struct rt_msghdr *);
static int	get(struct sockaddr_in *);
static char    *rifname;
static int	flags, doing_proxy, nflag;
static time_t	expire_time;
static struct sockaddr_in *getaddr(char *host);
static int	search(u_long);
int		ping4(struct sockaddr_in *addr);
int		ping6(struct sockaddr *addr, size_t addrlen);

int
usage(char *myname)
{
	printf("Check for ip address availability");
	printf("require: --ip=X\n");
	printf("opt: --pingnum=N, --pingtimeout=M\n");
	printf("--pingnum = number of icmp packet send, 2 is default\n");
	printf("--pingtimeout = interval between icmp packet send in seconds, default = 0.5\n");
	printf("Return value: 0 - IP(and/or ARP) is not exist, 1 - IP(and/or ARP) - exist\n");
	printf("usage: %s --ip=2001:1bb0:e000:b::19\n", myname);
	exit(0);
}

//return 0 if ipv4
//return 1 if ipv6
int		is_ipv6    (char *ip)
{
	if ((isxdigit(ip[0]) || ip[0] == ':') && (strchr(ip, ':') != NULL))
	    return 1;
	return 0;
}


int 
debugmsg(int level, const char *format,...)
{
	va_list		arg;
	int		done;

	if (debug < level)
		return 0;
	va_start(arg, format);
	done = vfprintf(stdout, format, arg);
	va_end(arg);

	return 0;
}


int
errmsg(const char *format,...)
{
	va_list		arg;
	int		done;

	va_start(arg, format);
	done = vfprintf(stderr, format, arg);
	va_end(arg);

	return 0;
}

//Checksum routine for Internet
//Protocol family headers(C Version)
u_short
in_cksum(u_short * addr, int len)
{
	int		nleft, sum;
	u_short        *w;

	union {
		u_short		us;
		u_char		uc      [2];
	}		last;
	u_short		answer;

	nleft = len;
	sum = 0;
	w = addr;

	//Our algorithm is simple, using a 32 bit accumulator(sum), we add
	// sequential 16 bit words to it, and at the end, fold back all the
	// carry bits from the top 16 bits into the lower 16 bits.
	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	//mop up an odd byte, if necessary
	if (nleft == 1) {
		last.uc[0] = *(u_char *) w;
		last.uc[1] = 0;
		sum += last.us;
	}
	//add back carry outs from top 16 bits to low 16 bits
	sum = (sum >> 16) + (sum & 0xffff);
	//add hi 16 to low 16
	sum += (sum >> 16);
	//add carry
	answer = ~sum;
	//truncate to 16 bits

	return (answer);
}

// return 0 if not
// icmp reply got
// return 1 if icmp
// reply caughted
int
ping4 (struct sockaddr_in *addr)
{
	const int	val = 255;
	int		i         , sd;
	u_char		*packet;
	outpack =	outpackhdr + sizeof(struct ip);
	packet =	outpack;
	unsigned char	buf[1024];
	struct icmp    *icp;
	int		cc;
	struct sockaddr_in from;
	int		fromlen;
	struct ip	*ip;

	icp = (struct icmp *)outpack;
	icp->icmp_type = ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_id = ident;	/* ID */
	sd = socket(PF_INET, SOCK_RAW, proto->p_proto);

	if (sd < 0) {
	    //socket error
	    return 1;
	}

	if (setsockopt(sd, SOL_SOCKET, IP_TTL, &val, sizeof(val)) != 0)
		errmsg("setsockopt error\r\n");


	if (fcntl(sd, F_SETFL, O_NONBLOCK) != 0) {
		errmsg("Request nonblocking I/O");
		return 1;
	}

	for (i = 0; i < pingnum; i++) {
		socklen_t	len = sizeof(addr);
		cc = ICMP_MINLEN + phdr_len + datalen;

		icp->icmp_cksum = in_cksum((u_short *) icp, cc);
		if (sendto(sd, (char *)packet, cc, 0, (struct sockaddr *)addr, sizeof(*addr)) <= 0)
			return 1;
		usleep(pingtimeout);
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	struct hostent *hname;
	struct sockaddr_in addr;
	char           *myname;
	float		t = 0;
	myname = argv[0];
	int		optcode = 0, option_index = 0, ret = 0, status;
	struct addrinfo	hints, *res;

	pingtimeout = PINGTIMEOUT * 1000000;
	pingnum = PINGNUM;
	memset(testip, 0, sizeof(testip));

	static struct option long_options[] = {
		{"ip", required_argument, 0, C_IP},
		{"pingnum", required_argument, 0, C_PINGNUM},
		{"pingtimeout", required_argument, 0, C_PINGTIMEOUT},
		{"help", no_argument, 0, C_HELP},
		{"debug", required_argument, 0, C_DEBUG},
		/* End of options marker */
		{0, 0, 0, 0}
	};

	while (TRUE) {
		optcode = getopt_long_only(argc, argv, "", long_options, &option_index);
		if (optcode == -1)
			break;
		switch (optcode) {
		case C_IP:
			if (strlen(optarg) >= 40) {
				errmsg("IP is invalid, to long\n");
				exit(1);
			}
			strcpy(testip, optarg);
			break;
		case C_PINGNUM:
			pingnum = atoi(optarg);
			break;
		case C_PINGTIMEOUT:
			t = atof(optarg);
			pingtimeout = t * 1000000;
			break;
		case C_HELP:	/* usage() */
			usage(myname);
			exit(0);
			break;
		case C_DEBUG:	/* debuglevel 0-2 */
			debug = atoi(optarg);
			break;
		}
	}

	if (strlen(testip) < 3) {
		errmsg("--ip argument is mandatory\n");
		exit(1);
	}
	if (is_ipv6(testip))
		ipv6 = 1;
	else
		ipv6 = 0;

	switch (ipv6) {
	case 0:
		ident = getpid() & 0xFFFF;
		proto = getprotobyname("ICMP");
		hname = gethostbyname(testip);
		bzero(&addr, sizeof(addr));
		addr.sin_family = hname->h_addrtype;
		addr.sin_port = 0;
		addr.sin_addr.s_addr = *(long *)hname->h_addr;
		delete(&addr);
		ping4(&addr);
		ret = get(&addr);
		break;
	case 1:
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_INET6;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = hints.ai_flags | AI_CANONNAME;

		if ((status = getaddrinfo(testip, NULL, &hints, &res)) != 0) {
			errmsg("getaddrinfo() failed: %s\n", gai_strerror(status));
			exit(1);
		}
		ping6(res->ai_addr, res->ai_addrlen);
		//ndp here
			freeaddrinfo(res);
		break;
	}

	return ret;
}
/*
 * Delete an arp entry
 */
static int
delete(struct sockaddr_in *dst)
{
	struct sockaddr_in *addr;
	struct rt_msghdr *rtm;
	struct sockaddr_dl *sdl;
	struct sockaddr_dl sdl_m;

	if (dst == NULL)
		return (1);

	/*
	 * Perform a regular entry delete first.
	 */
	flags &= ~RTF_ANNOUNCE;

	/*
	 * setup the data structure to notify the kernel it is the ARP entry
	 * the RTM_GET is interested in
	 */
	bzero(&sdl_m, sizeof(sdl_m));
	sdl_m.sdl_len = sizeof(sdl_m);
	sdl_m.sdl_family = AF_LINK;

	for (;;) {		/* try twice */
		rtm = rtmsg(RTM_GET, dst, &sdl_m);
		if (rtm == NULL) {
			return (1);
		}
		addr = (struct sockaddr_in *)(rtm + 1);
		sdl = (struct sockaddr_dl *)(SA_SIZE(addr) + (char *)addr);

		/*
		 * With the new L2/L3 restructure, the route returned is a
		 * prefix route. The important piece of information from the
		 * previous RTM_GET is the interface index. In the case of
		 * ECMP, the kernel will traverse the route group for the
		 * given entry.
		 */
		if (sdl->sdl_family == AF_LINK &&
		    !(rtm->rtm_flags & RTF_GATEWAY) &&
		    valid_type(sdl->sdl_type)) {
			addr->sin_addr.s_addr = dst->sin_addr.s_addr;
			break;
		} else
			//this is not for me, possible external via RTF_GATEWAY
			return 1;
	}
	rtm->rtm_flags |= RTF_LLDATA;
	if (rtmsg(RTM_DELETE, dst, NULL) != NULL) {
		return (0);
	}
	return (1);
}

/*
 * Given a hostname, fills up a (static) struct sockaddr_in with the address
 * of the host and returns a pointer to the structure.
 */
static struct sockaddr_in *
getaddr(char *host)
{
	struct hostent *hp;
	static struct sockaddr_in reply;

	bzero(&reply, sizeof(reply));
	reply.sin_len = sizeof(reply);
	reply.sin_family = AF_INET;
	reply.sin_addr.s_addr = inet_addr(host);
	if (reply.sin_addr.s_addr == INADDR_NONE) {
		if (!(hp = gethostbyname(host))) {
			warnx("%s: %s", host, hstrerror(h_errno));
			return (NULL);
		}
		bcopy((char *)hp->h_addr, (char *)&reply.sin_addr,
		      sizeof reply.sin_addr);
	}
	return (&reply);
}


/*
 * Returns true if the type is a valid one for ARP.
 */
static int
valid_type(int type)
{

	switch (type) {
	case IFT_ETHER:
	case IFT_FDDI:
	case IFT_ISO88023:
	case IFT_ISO88024:
	case IFT_ISO88025:
	case IFT_L2VLAN:
	case IFT_BRIDGE:
		return (1);
	default:
		return (0);
	}
}


static struct rt_msghdr *
rtmsg(int cmd, struct sockaddr_in *dst, struct sockaddr_dl *sdl)
{
	static int	seq;
	int		rlen;
	int		l;
	struct sockaddr_in so_mask, *som = &so_mask;
	static int	s = -1;
	static pid_t	pid;

	doing_proxy = flags = expire_time = 0;

	static struct {
		struct rt_msghdr m_rtm;
		char		m_space   [512];
	}		m_rtmsg;

	struct rt_msghdr *rtm = &m_rtmsg.m_rtm;
	char           *cp = m_rtmsg.m_space;

	if (s < 0) {		/* first time: open socket, get pid */
		s = socket(PF_ROUTE, SOCK_RAW, 0);
		if (s < 0)
			err(1, "socket");
		pid = getpid();
	}
	bzero(&so_mask, sizeof(so_mask));
	so_mask.sin_len = 8;
	so_mask.sin_addr.s_addr = 0xffffffff;

	errno = 0;
	/*
	 * XXX RTM_DELETE relies on a previous RTM_GET to fill the buffer
	 * appropriately.
	 */
	if (cmd == RTM_DELETE)
		goto doit;
	bzero((char *)&m_rtmsg, sizeof(m_rtmsg));
	rtm->rtm_flags = flags;
	rtm->rtm_version = RTM_VERSION;

	switch (cmd) {
	default:
		errx(1, "internal wrong cmd");
	case RTM_ADD:
		rtm->rtm_addrs |= RTA_GATEWAY;
		rtm->rtm_rmx.rmx_expire = expire_time;
		rtm->rtm_inits = RTV_EXPIRE;
		rtm->rtm_flags |= (RTF_HOST | RTF_STATIC | RTF_LLDATA);
		if (doing_proxy) {
			rtm->rtm_addrs |= RTA_NETMASK;
			rtm->rtm_flags &= ~RTF_HOST;
		}
		/* FALLTHROUGH */
	case RTM_GET:
		rtm->rtm_addrs |= RTA_DST;
	}
#define NEXTADDR(w, s)					   \
	do {						   \
		if ((s) != NULL && rtm->rtm_addrs & (w)) { \
			bcopy((s), cp, sizeof(*(s)));	   \
			cp += SA_SIZE(s);		   \
		}					   \
	} while (0)

	NEXTADDR(RTA_DST, dst);
	NEXTADDR(RTA_GATEWAY, sdl);
	NEXTADDR(RTA_NETMASK, som);

	rtm->rtm_msglen = cp - (char *)&m_rtmsg;
doit:
	l = rtm->rtm_msglen;
	rtm->rtm_seq = ++seq;
	rtm->rtm_type = cmd;
	if ((rlen = write(s, (char *)&m_rtmsg, l)) < 0) {
		if (errno != ESRCH || cmd != RTM_DELETE) {
			//warn("writing to routing socket");
			return (NULL);
		}
	}
	do {
		l = read(s, (char *)&m_rtmsg, sizeof(m_rtmsg));
	} while (l > 0 && (rtm->rtm_seq != seq || rtm->rtm_pid != pid));
	//if (l < 0)
		//warn("read from routing socket");
	return (rtm);
}

static int
get(struct sockaddr_in *addr)
{
	if (addr == NULL)
		return (1);
	if (search(addr->sin_addr.s_addr) == 1) {
		return 1;
	}
	return 0;
}

static int
search(u_long addr)
{
	int		mib        [6];
	size_t		needed;
	char           *lim, *buf, *next;
	struct rt_msghdr *rtm;
	struct sockaddr_in *sin2;
	struct sockaddr_dl *sdl;
	char		ifname    [IF_NAMESIZE];
	int		st        , found_entry = 0;

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;
	mib[3] = AF_INET;
	mib[4] = NET_RT_FLAGS;
#ifdef RTF_LLINFO
	mib[5] = RTF_LLINFO;
#else
	mib[5] = 0;
#endif
	if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)
		err(1, "route-sysctl-estimate");
	if (needed == 0)	/* empty table */
		return 0;
	buf = NULL;
	for (;;) {
		buf = reallocf(buf, needed);
		if (buf == NULL)
			errx(1, "could not reallocate memory");
		st = sysctl(mib, 6, buf, &needed, NULL, 0);
		if (st == 0 || errno != ENOMEM)
			break;
		needed += needed / 8;
	}
	if (st == -1)
		err(1, "actual retrieval of routing table");
	lim = buf + needed;

	for (next = buf; next < lim; next += rtm->rtm_msglen) {
		rtm = (struct rt_msghdr *)next;
		sin2 = (struct sockaddr_in *)(rtm + 1);
		sdl = (struct sockaddr_dl *)((char *)sin2 + SA_SIZE(sin2));
		if (rifname && if_indextoname(sdl->sdl_index, ifname) &&
		    strcmp(ifname, rifname))
			continue;
		if (addr) {
			if (addr != sin2->sin_addr.s_addr)
				continue;
			//olevole: if sdl_alen = 0 it is incomplete records(record exist in fast cache without solve to mac)
			if (sdl->sdl_alen != 0)
				found_entry = 1;
		}
	}
	free(buf);
	return (found_entry);
}



int 
ping6(struct sockaddr *addr, size_t addrlen)
{
	int		sd        , cmsglen, datalen, hoplimit, psdhdrlen, i;
	struct icmp6_hdr *icmphdr;
	unsigned char  *data, *outpack, *psdhdr;
	struct sockaddr_in6 dst;
	struct msghdr	msghdr;
	struct cmsghdr *cmsghdr1, *cmsghdr2;
	struct in6_pktinfo *pktinfo;
	struct iovec	iov[2];
	struct iovec	riov[2];
	void           *tmp;

	//Maximum ICMP payload size = 65535 - IPv6 header(40 bytes) - ICMP header(8 bytes)
		tmp = (unsigned char *)malloc((IP_MAXPACKET - IP6_HDRLEN - ICMP_HDRLEN) * sizeof(unsigned char));
	if (tmp != NULL) {
		data = tmp;
	} else {
		fprintf(stderr, "ERROR: Cannot allocate memory for array 'data'.\n");
		exit(EXIT_FAILURE);
	}
	memset(data, 0, (IP_MAXPACKET - IP6_HDRLEN - ICMP_HDRLEN) * sizeof(unsigned char));

	tmp = (unsigned char *)malloc((IP_MAXPACKET - IP6_HDRLEN - ICMP_HDRLEN) * sizeof(unsigned char));
	if (tmp != NULL) {
		outpack = tmp;
	} else {
		fprintf(stderr, "ERROR: Cannot allocate memory for array 'outpack'.\n");
		exit(EXIT_FAILURE);
	}
	memset(outpack, 0, (IP_MAXPACKET - IP6_HDRLEN - ICMP_HDRLEN) * sizeof(unsigned char));

	tmp = (unsigned char *)malloc(IP_MAXPACKET * sizeof(unsigned char));
	if (tmp != NULL) {
		psdhdr = tmp;
	} else {
		fprintf(stderr, "ERROR: Cannot allocate memory for array 'psdhdr'.\n");
		exit(EXIT_FAILURE);
	}
	memset(psdhdr, 0, IP_MAXPACKET * sizeof(unsigned char));

	//Submit request for a socket descriptor to look up interface
	if ((sd = socket(AF_INET6, SOCK_RAW, IPPROTO_IPV6)) < 0) {
		perror("socket() failed to get socket descriptor for using ioctl() ");
		exit(EXIT_FAILURE);
	}
	close(sd);

	memcpy(&dst, addr, addrlen);

	memcpy(psdhdr + 16, dst.sin6_addr.s6_addr, 16);
	//Copy to checksum pseudo - header
	// Define first part of buffer outpack to be an ICMPV6 struct.
	icmphdr = (struct icmp6_hdr *)outpack;
	memset(icmphdr, 0, ICMP_HDRLEN);

	//Populate icmphdr portion of buffer outpack.
	icmphdr->icmp6_type = ICMP6_ECHO_REQUEST;
	icmphdr->icmp6_code = 0;
	icmphdr->icmp6_cksum = 0;
	icmphdr->icmp6_id = htons(5);
	icmphdr->icmp6_seq = htons(300);

	//ICMP data
	datalen = 4;
	data[0] = 'T';
	data[1] = 'e';
	data[2] = 's';
	data[3] = 't';

	//Append ICMP data.
	memcpy(outpack + ICMP_HDRLEN, data, datalen);

	//Need a pseudo - header for checksum calculation.Define length.(RFC 2460)
	// Length = source IP(16 bytes) + destination IP(16 bytes)
	// +upper layer packet length(4 bytes) + zero(3 bytes)
	// +next header(1 byte)
	psdhdrlen = 16 + 16 + 4 + 3 + 1 + ICMP_HDRLEN + datalen;

	//Compose the msghdr structure.
	memset(&msghdr, 0, sizeof(msghdr));
	msghdr.msg_name = &dst;
	//pointer to socket address structure
	msghdr.msg_namelen = sizeof(dst);
	//size of socket address structure

		memset(&iov, 0, sizeof(iov));
	iov[0].iov_base = (unsigned char *)outpack;
	iov[0].iov_len = ICMP_HDRLEN + datalen;
	msghdr.msg_iov = iov;
	//scatter / gather array
	msghdr.msg_iovlen = 1;
	//number of elements in scatter / gather array

	// Initialize msghdr and control data to total length of the two messages to be sent.
	// Allocate some memory for our cmsghdr data.
	cmsglen = CMSG_SPACE(sizeof(int)) + CMSG_SPACE(sizeof(struct in6_pktinfo));
	tmp = (unsigned char *)malloc(cmsglen * sizeof(unsigned char));

	if (tmp != NULL) {
		msghdr.msg_control = tmp;
	} else {
		fprintf(stderr, "ERROR: Cannot allocate memory for array 'msghdr.msg_control'.\n");
		exit(EXIT_FAILURE);
	}
	memset(msghdr.msg_control, 0, cmsglen);
	msghdr.msg_controllen = cmsglen;

	//Change hop limit to 255 via cmsghdr data.
	hoplimit = 255;
	cmsghdr1 = CMSG_FIRSTHDR(&msghdr);
	cmsghdr1->cmsg_level = IPPROTO_IPV6;
	cmsghdr1->cmsg_type = IPV6_HOPLIMIT;
	//We want to change hop limit
	cmsghdr1->cmsg_len = CMSG_LEN(sizeof(int));
	*((int *)CMSG_DATA(cmsghdr1)) = hoplimit;

	//Specify source interface index for this packet via cmsghdr data.
	cmsghdr2 = CMSG_NXTHDR(&msghdr, cmsghdr1);
	cmsghdr2->cmsg_level = IPPROTO_IPV6;
	cmsghdr2->cmsg_type = IPV6_PKTINFO;
	//We want to specify interface here
	cmsghdr2->cmsg_len = CMSG_LEN(sizeof(struct in6_pktinfo));
	pktinfo = (struct in6_pktinfo *)CMSG_DATA(cmsghdr2);
	//pktinfo->ipi6_ifindex = ifr.ifr_ifindex;

	//Compute ICMPv6 checksum(RFC 2460).
	// psdhdr[0 to 15] = source IPv6 address, set earlier.
	// psdhdr[16 to 31] = destination IPv6 address, set earlier.
	psdhdr[32] = 0;
	//Length should not be greater than 65535(i.e., 2 bytes)
	psdhdr[33] = 0;
	//Length should not be greater than 65535(i.e., 2 bytes)
	psdhdr[34] = (ICMP_HDRLEN + datalen) / 256;
	//Upper layer packet length
	psdhdr[35] = (ICMP_HDRLEN + datalen) % 256;
	//Upper layer packet length
	psdhdr[36] = 0;
	//Must be zero
	psdhdr[37] = 0;
	//Must be zero
	psdhdr[38] = 0;
	//Must be zero
	psdhdr[39] = IPPROTO_ICMPV6;
	memcpy(psdhdr + 40, outpack, ICMP_HDRLEN + datalen);

	for (i = 0; i < pingnum; i++) {
		//Request a socket descriptor sd.
		if ((sd = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) < 0) {
			fprintf(stderr, "Failed to get socket descriptor.\n");
			exit(EXIT_FAILURE);
		}
		//Send packet.
		if (sendmsg(sd, &msghdr, 0) < 0) {
			perror("sendmsg() failed ");
			return 1;
		}
		usleep(pingtimeout);
	}

	close(sd);
	free(data);
	free(outpack);
	free(psdhdr);
	free(msghdr.msg_control);
	return 0;
}
