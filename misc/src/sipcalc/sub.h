/*
 * sipcalc, sub.h
 *
 * $Id: sub.h,v 1.31 2003/03/19 12:28:15 simius Exp $
 *
 * -
 * Copyright (c) 2003 Simon Ekstrand
 * All rights reserved.
 */

#ifndef SUB_H
#define SUB_H
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_BITYPES_H
#include <sys/bitypes.h>
#endif
#include <sys/socket.h>
#include <net/if.h>

#ifdef PACKAGE
#define NAME PACKAGE
#else
#define NAME "sipcalc"
#endif

#if defined(HAVE_GETHOSTBYNAME_NETDB) && !defined(HAVE_GETHOSTBYNAME)
#define HAVE_GETHOSTBYNAME
#endif
#if defined(HAVE_GETHOSTBYNAME2_NETDB) && !defined(HAVE_GETHOSTBYNAME2)
#define HAVE_GETHOSTBYNAME
#endif
#if defined(HAVE_GETADDRINFO_NETDB) && !defined(HAVE_GETADDRINFO)
#define HAVE_GETADDRINFO
#endif

#if !defined(HAVE_U_INT8_T) && defined(HAVE_UINT8_T)
#define u_int8_t uint8_t
#endif
#if defined(HAVE_U_INT8_T) && !defined(HAVE_UINT8_T)
#define uint8_t u_int8_t
#endif

#if !defined(HAVE_U_INT16_T) && defined(HAVE_UINT16_T)
#define u_int16_t uint16_t
#endif
#if defined(HAVE_U_INT16_T) && !defined(HAVE_UINT16_T)
#define uint16_t u_int16_t
#endif

#if !defined(HAVE_U_INT32_T) && defined(HAVE_UINT32_T)
#define u_int32_t uint32_t
#endif
#if defined(HAVE_U_INT32_T) && !defined(HAVE_UINT32_T)
#define uint32_t u_int32_t
#endif

#ifndef PF_UNSPEC
#define PF_UNSPEC AF_UNSPEC
#endif

#ifndef PF_INET
#define PF_INET AF_INET
#endif

#ifndef PF_INET6
#define PF_INET6 AF_INET6
#endif

#define V4ADDR_VAL "0123456789."
#define V6ADDR_VAL "0123456789ABCDEFabcdef:"
#define NETMASK_VAL "0123456789"

#define TERMINATE(x) (x[sizeof(x)-1]='\0')
#define safe_strncpy(dest,src) strncpy(dest,src,sizeof(dest)-1+TERMINATE(dest)*0)
#define safe_strncat(dest,src) strncat(dest,src,sizeof(dest)-1+TERMINATE(dest)*0)
#define safe_snprintf(dest,whatever...) snprintf(dest,sizeof(dest),## whatever)
#define safe_bzero(dest) bzero((char *)dest,sizeof(dest))

/*
 * Easier to define this ourselves then to use all the different
 * versions from different platforms.
 */
struct sip_in6_addr {
	union {
		uint8_t u6_addr8[16];
		uint16_t u6_addr16[8];
		uint32_t u6_addr32[4];
	} sip_in6_u;
#define sip6_addr	sip_in6_u.u6_addr8
#define sip6_addr8	sip_in6_u.u6_addr8
#define sip6_addr16	sip_in6_u.u6_addr16
#define sip6_addr32	sip_in6_u.u6_addr32
};

struct v4addr {
	char class;
	char class_remark[64];
	char pres_bitmap[36];
	int n_nmaskbits;
	u_int32_t n_cbroadcast;
	u_int32_t n_broadcast;
	u_int32_t n_cnaddr;
	u_int32_t n_naddr;
	u_int32_t n_cnmask;
	u_int32_t n_nmask;
	u_int32_t n_haddr;
	u_int32_t i_broadcast;
};

/*
 * Broadcast in this structure is sort of missleading, since ipv6 networks
 * don't have broadcast addresses, but it's as good a name as any for the
 * top address of a subnet.
 *
 * Prefix can also be directly translated to a ipv4 network address.
 */
struct v6addr {
	char class_remark[64];
	char comment[64];
	struct sip_in6_addr haddr;
	int nmaskbits;
	struct sip_in6_addr nmask;
	struct sip_in6_addr prefix;
	struct sip_in6_addr suffix;
	struct sip_in6_addr broadcast;
	int type;
	int real_v4;
};

struct if_info {
	char name[IFNAMSIZ + 1];
	char p_v4addr[19], p_v4nmask[16];
	char p_v6addr[44];
	char errorstr[64];
	char cmdstr[128];
	short flags;
	short type;
	struct v4addr v4ad;
	struct v6addr v6ad;
	struct if_info *next;
};

struct misc_args {
	int numnets;
	u_int32_t splitmask;
	struct sip_in6_addr v6splitmask;
	int v6splitnum;
};

struct ipv6_split {
	char ipv6addr[40];
	char ipv4addr[16];
	char nmask[4];
};

struct argbox {
	char str[128];
	int type;
	int resolv;
	struct argbox *next;
};

struct dnsresp {
	char str[128];
	int type;
	struct dnsresp *next;
};

#define AT_V4 1
#define AT_V6 2
#define AT_INT 3
#define AT_UNKWN 4

#define IFT_V4 1
#define IFT_V6 2
#define IFT_INTV4 3
#define IFT_INTV6 4
#define IFT_UNKWN 5

/* v4 args */
#define CF_INFO     0x01
#define CF_BITMAP   0x02
#define CIDR_INFO   0x04
#define CIDR_BITMAP 0x08
#define NET_INFO    0x10
#define V4SPLIT     0x20
#define V4VERBSPLIT 0x40
#define C_WILDCARD  0x80

/* v6 args */
#define V6_INFO     0x01
#define V4INV6      0x02
#define V6SPLIT     0x04
#define V6REV       0x08
#define V6VERBSPLIT 0x10

#define V6TYPE_STANDARD 1
#define V6TYPE_V4INV6 2

/*
 * prototypes
 */
/*
 * sub.c
 */
int out_int (struct if_info *if_cur, struct if_info *ifarg_cur, int v4args,
	     struct misc_args m_argv4, int v6args, struct misc_args m_argv6);
int out_cmdline (struct if_info *ifarg_cur, int v4args,
		 struct misc_args m_argv4, int v6args,
		 struct misc_args m_argv6, int recurse, int index);
int cleanline (char *sbuf, char *dbuf);
int get_stdin (char *args[]);

/*
 * sub-func.c
 */
int count (char *buf, char ch);
int validate_v4addr (char *addr);
int validate_netmask (char *in_addr);
int getsplitnumv4 (char *buf, u_int32_t * splitmask);
int getsplitnumv6 (char *buf, struct sip_in6_addr *splitmask, int *v6splitnum);
int quadtonum (char *quad, u_int32_t * num);
char *numtoquad (u_int32_t num);
char *numtobitmap (u_int32_t num);
int parse_addr (struct if_info *ifi);
int get_addrv4 (struct if_info *ifi);
int get_addrv6 (struct if_info *ifi);
int split_ipv6addr (char *addr, struct ipv6_split *spstr);
int validate_s_v6addr (char *addr, int type);
int getcolon (char *addr, int pos, int type);
int v6addrtonum (struct ipv6_split spstr, struct v6addr *in6_addr, int type);
int v6masktonum (char *nmask, int *nmaskbits, struct sip_in6_addr *in6_addr);
int validate_v6addr (char *addr);
int v6addrtoprefix (struct v6addr *in6_addr);
int v6addrtosuffix (struct v6addr *in6_addr);
int v6addrtobroadcast (struct v6addr *in6_addr);
void v6_type (struct v6addr *in6_addr);
void v6_comment (struct v6addr *in6_addr);
int v6verifyv4 (struct sip_in6_addr addr);
char *get_comp_v6 (struct sip_in6_addr addr);
int mk_ipv6addr (struct v6addr *in6_addr, char *addr);
struct dnsresp *new_dnsresp (struct dnsresp *d_resp);
void free_dnsresp (struct dnsresp *d_resp);
char *resolve_addr (char *addr, int family, struct dnsresp *);

/*
 * interface.c
 */
struct if_info *new_if (struct if_info *ifarg_cur);
void free_if (struct if_info *ifa);
struct if_info *get_if_ext ();

#endif				/* SUB_H */
