// detect for ip collision by "arp delete -> ping -> arg get" sequence
//
// This is part of the CBSD Project
//
// return 1 if records exist
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
#include <net/iso88025.h>

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
#define DEFDATALEN	56              /* default data length */

useconds_t pingtimeout=0;
int pingnum=0;
char ip[15];
int debug=0;
int noarp=0;

#define FALSE 0
#define TRUE 1

/* List of all cmd */
enum {
    C_PINGNUM,
    C_IP,
    C_PINGTIMEOUT,
    C_HELP,
    C_NOARP,
    C_DEBUG,
};


int ident=-1;
struct protoent *proto=NULL;
u_char outpackhdr[IP_MAXPACKET], *outpack;
int phdr_len = 0;
int datalen = DEFDATALEN;
u_char icmp_type_rsp = ICMP_ECHOREPLY;
int ping(struct sockaddr_in *);
static int      flags,doing_proxy;
static time_t   expire_time;
static int delete(struct sockaddr_in *);
static int valid_type(int);
static struct rt_msghdr *rtmsg(int, struct sockaddr_in *, struct sockaddr_dl *);
typedef void (action_fn)(struct sockaddr_dl *,struct sockaddr_in *, struct rt_msghdr *);
static int get(struct sockaddr_in *);
static char *rifname;
static int      flags,doing_proxy,nflag;
static time_t   expire_time;
static struct sockaddr_in *getaddr(char *host);
static int search(u_long);

int
usage(char *myname)
{
    printf("Check for ip address availability. Still IPv4 only");
    printf("require: --ip=x.y.n.m\n");
    printf("opt: --pingnum=N, --pingtimeout=M, --noarp\n");
    printf("--pingnum = number of icmp packet send, 2 is default\n");
    printf("--pingtimeout = interval between icmp packet send in seconds, default = 0.5\n");
    printf("--noarp = skip for ARP get, just ping\n");
    printf("Return value: 0 - IP(and/or ARP) is not exist, 1 - IP(and/or ARP) - exist\n");
    printf("usage: %s --ip=127.0.0.1\n",myname);
    exit(0);
}

int debugmsg(int level,const char *format, ...)
{
va_list arg;
int done;

    if(debug<level) return 0;
    va_start (arg, format);
    done = vfprintf (stdout, format, arg);
    va_end (arg);

return 0;
}

int errmsg(const char *format, ...)
{
   va_list arg;
   int done;

   va_start (arg, format);
   done = vfprintf (stderr, format, arg);
   va_end (arg);

   return 0;
}


//  Checksum routine for Internet Protocol family headers (C Version)
u_short
in_cksum(u_short *addr, int len)
{
	int nleft, sum;
	u_short *w;
	union {
	    u_short us;
	    u_char  uc[2];
	} last;
	u_short answer;

	nleft = len;
	sum = 0;
	w = addr;

	// Our algorithm is simple, using a 32 bit accumulator (sum), we add
	// sequential 16 bit words to it, and at the end, fold back all the
	// carry bits from the top 16 bits into the lower 16 bits.
	while (nleft > 1)  {
	    sum += *w++;
	    nleft -= 2;
	}

	// mop up an odd byte, if necessary
	if (nleft == 1) {
	    last.uc[0] = *(u_char *)w;
	    last.uc[1] = 0;
	    sum += last.us;
	}

	// add back carry outs from top 16 bits to low 16 bits
	sum = (sum >> 16) + (sum & 0xffff);     // add hi 16 to low 16
	sum += (sum >> 16);                     // add carry
	answer = ~sum;                          // truncate to 16 bits

	return(answer);
}

//return 0 if not icmp reply got
//return 1 if icmp reply caughted
int ping(struct sockaddr_in *addr)
{	const int val=255;
	int i, sd, cnt=1;
	u_char *packet;

	outpack = outpackhdr + sizeof(struct ip);

	packet = outpack;
	unsigned char buf[1024];

	struct icmp *icp;
	struct icmp *ocp; //for listener/reply
	int cc;
	icp = (struct icmp *)outpack;
	ocp = (struct icmp *)outpack; //for listener/reply
	icp->icmp_type = ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_id = ident; /* ID */

	struct sockaddr_in r_addr;
	struct sockaddr_in from;
	int fromlen;
	struct ip	*ip;

	sd = socket(PF_INET, SOCK_RAW, proto->p_proto);
	if ( sd < 0 )
	{
		perror("socket");
		return;
	}
	if ( setsockopt(sd, SOL_SOCKET, IP_TTL, &val, sizeof(val)) != 0)
		perror("Set TTL option");

	if ( fcntl(sd, F_SETFL, O_NONBLOCK) != 0 )
		perror("Request nonblocking I/O");
	for (;;)
	{	socklen_t len=sizeof(r_addr);

//		printf("Msg #%d\n", cnt++);
		cc = ICMP_MINLEN + phdr_len + datalen;

		icp->icmp_cksum = in_cksum((u_short *)icp, cc);
		if ( sendto(sd, (char *)packet, cc, 0, (struct sockaddr *)addr, sizeof(*addr)) <=0 )
			return 0;

		fromlen = sizeof(from);
		if ( recvfrom(sd, buf, sizeof(buf), 0, (struct sockaddr*)&from, &len) > 0 ) {
		    //got message
		    ip = (struct ip *)buf;
		    fromlen = ip->ip_hl << 2;
		    ocp = (struct icmp *)(buf + fromlen);
		    if (ocp->icmp_type == icmp_type_rsp) {
	//	    printf("My Id: %d, foreign id: %d, seq: %d\n",ident,ocp->icmp_id,ocp->icmp_seq);
		    if (ocp->icmp_id != ident)
			continue;                 /* 'Twas not our ECHO */
		    return 1;
		}
		}
		usleep(500000);
	if (cnt>=pingnum) return 0;
	cnt++;
	}

return 0;
}

main(int argc, char *argv[])
{
	struct hostent *hname;
	struct sockaddr_in addr;
	char *myname;
	float t=0;
	myname = argv[0];
	int optcode = 0, option_index = 0, ret = 0;

	pingtimeout=PINGTIMEOUT*1000000;
	pingnum=PINGNUM;
	memset(ip,0,sizeof(ip));

	static struct option long_options[] = {
	    { "ip", required_argument, 0 , C_IP },
	    { "pingnum", required_argument, 0 , C_PINGNUM },
	    { "pingtimeout", required_argument, 0 , C_PINGTIMEOUT },
	    { "help", no_argument, 0, C_HELP },
	    { "noarp", no_argument, 0, C_NOARP },
	    { "debug", required_argument, 0, C_DEBUG },
	    /* End of options marker */
	    { 0, 0, 0, 0 }
        };

	while (TRUE) {
	    optcode = getopt_long_only(argc, argv, "", long_options, &option_index);
	    if (optcode == -1) break;
		switch (optcode) {
		case C_IP:
		    if (strlen(optarg)>15) { errmsg("IP is invalid, to long\n"); exit(1); }
		    strcpy(ip,optarg);
		    break;
		case C_PINGNUM:
		    pingnum=atoi(optarg);
		    break;
		case C_PINGTIMEOUT:
		    t=atof(optarg);
		    pingtimeout=t*1000000;
		    break;
		case C_HELP:      /* usage() */
		    usage(myname);
		    exit(0);
		    break;
		case C_NOARP:      /* usage() */
		    noarp=1;
		    break;
		case C_DEBUG:      /* debuglevel 0-2 */
		    debug=atoi(optarg);
		    break;
		}
	}

	if (strlen(ip)<5) {
	    errmsg("--ip argument is mandatory\n");
	    exit(1);
	}

	ident = getpid() & 0xFFFF;
	proto = getprotobyname("ICMP");
	hname = gethostbyname(ip);
	bzero(&addr, sizeof(addr));
	addr.sin_family = hname->h_addrtype;
	addr.sin_port = 0;
	addr.sin_addr.s_addr = *(long*)hname->h_addr;
	if (noarp==0)	delete(&addr);
	ret=ping(&addr);
	if(noarp==1) return ret;
	return get(&addr);
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
	 * setup the data structure to notify the kernel
	 * it is the ARP entry the RTM_GET is interested
	 * in
	 */
	bzero(&sdl_m, sizeof(sdl_m));
	sdl_m.sdl_len = sizeof(sdl_m);
	sdl_m.sdl_family = AF_LINK;

	for (;;) {	/* try twice */
		rtm = rtmsg(RTM_GET, dst, &sdl_m);
		if (rtm == NULL) {
			return (1);
		}
		addr = (struct sockaddr_in *)(rtm + 1);
		sdl = (struct sockaddr_dl *)(SA_SIZE(addr) + (char *)addr);

		/*
		 * With the new L2/L3 restructure, the route 
		 * returned is a prefix route. The important
		 * piece of information from the previous
		 * RTM_GET is the interface index. In the
		 * case of ECMP, the kernel will traverse
		 * the route group for the given entry.
		 */
		if (sdl->sdl_family == AF_LINK &&
		    !(rtm->rtm_flags & RTF_GATEWAY) &&
		    valid_type(sdl->sdl_type) ) {
			addr->sin_addr.s_addr = dst->sin_addr.s_addr;
			break;
		} else //this is not for me, possible external via RTF_GATEWAY
		    return 1;
	}
	rtm->rtm_flags |= RTF_LLDATA;
	if (rtmsg(RTM_DELETE, dst, NULL) != NULL) {
		return (0);
	}
	return (1);
}

/*
 * Given a hostname, fills up a (static) struct sockaddr_in with
 * the address of the host and returns a pointer to the
 * structure.
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
	static int seq;
	int rlen;
	int l;
	struct sockaddr_in so_mask, *som = &so_mask;
	static int s = -1;
	static pid_t pid;

	doing_proxy = flags = expire_time = 0;

	static struct	{
		struct	rt_msghdr m_rtm;
		char	m_space[512];
	}	m_rtmsg;

	struct rt_msghdr *rtm = &m_rtmsg.m_rtm;
	char *cp = m_rtmsg.m_space;

	if (s < 0) {	/* first time: open socket, get pid */
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
//			warn("writing to routing socket");
			return (NULL);
		}
	}
	do {
		l = read(s, (char *)&m_rtmsg, sizeof(m_rtmsg));
	} while (l > 0 && (rtm->rtm_seq != seq || rtm->rtm_pid != pid));
//	if (l < 0)
//		warn("read from routing socket");
	return (rtm);
}

static int
get(struct sockaddr_in *addr)
{
	if (addr == NULL)
		return (1);
	if (search(addr->sin_addr.s_addr)==1) {
		return 1;
	}
	return 0;
}

static int
search(u_long addr)
{
        int mib[6];
        size_t needed;
        char *lim, *buf, *next;
        struct rt_msghdr *rtm;
        struct sockaddr_in *sin2;
        struct sockaddr_dl *sdl;
        char ifname[IF_NAMESIZE];
        int st, found_entry = 0;

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
        if (needed == 0)        /* empty table */
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
			    //olevole: if  sdl_alen=0 it is incomplete records (record exist in fast cache without solve to mac)
                            if(sdl->sdl_alen!=0) found_entry = 1;
                }
        }
        free(buf);
        return (found_entry);
}


