/*
 * sipcalc, sub-func.c
 *
 * $Id: sub-func.c,v 1.28 2003/03/19 12:28:15 simius Exp $
 *
 * -
 * Copyright (c) 2003 Simon Ekstrand
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

static const char rcsid[] =
    "$Id: sub-func.c,v 1.28 2003/03/19 12:28:15 simius Exp $";

//#ifdef HAVE_CONFIG_H
//#include <config.h>
//#endif
//#ifdef HAVE_NETDB_H
#include <netdb.h>
//#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sub.h"
#include "sub-o.h"

int
count(char *buf, char ch)
{
	int x, y;

	y = 0;
	for (x = 0; x < strlen(buf); x++)
		if (buf[x] == ch)
			y++;

	return y;
}

/*
 * 0 error
 * 1 normal
 */
int
validate_v4addr(char *addr)
{
	int x, y, z, m;
	char buf[16];

	if (strlen(addr) < 7 || strlen(addr) > 15)
		return 0;

	x = 0;
	y = 0;
	while (x < strlen(addr)) {
		if (addr[x] == '.')
			y++;
		x++;
	}
	if (y != 3)
		return 0;

	x = 0;
	y = 0;
	while (x < strlen(addr)) {
		z = 0;
		y = 0;
		while (z < strlen(V4ADDR_VAL) && !y) {
			if (addr[x] == V4ADDR_VAL[z])
				y = 1;
			z++;
		}
		if (!y)
			return 0;
		x++;
	}

	if (addr[0] == '.' || addr[strlen(addr) - 1] == '.')
		return 0;

	x = 0;
	while (x < (strlen(addr) - 1)) {
		if (addr[x] == '.' && addr[x + 1] == '.')
			return 0;
		x++;
	}

	y = 0;
	for (x = 0; x < 4; x++) {
		z = 0;
		safe_bzero(buf);
		while (y < strlen(addr) && addr[y] != '.') {
			buf[z] = addr[y];
			y++;
			z++;
		}
		if (z > 3)
			return 0;
		m = atoi(buf);
		if (m < 0 || m > 255)
			return 0;
		y++;
	};

	return 1;
}

/*
 * 0 error
 * 1 normal
 * 2 /x
 * 3 hex
 */
int
validate_netmask(char *in_addr)
{
	int x, y, z, m;
	char addr[16];
	char *sl;
	u_int32_t sm;

	if (strlen(in_addr) > 18)
		return 0;

	x = 0;
	y = 0;
	z = 0;
	while (x < strlen(in_addr) && !y) {
		if (!isxdigit(in_addr[x]) && in_addr[x] != 'x' &&
		    in_addr[x] != 'X')
			y = 1;
		if (in_addr[x] == 'x' || in_addr[x] == 'X') {
			z++;
			y = 0;
		}
		x++;
	}

	x = 0;
	if (!y && z == 1) {
		x = 0;
		if (in_addr[0] == 'x' || in_addr[0] == 'X')
			x = 1;
		if (!x) {
			if (in_addr[0] == '0' &&
			    (in_addr[1] == 'x' || in_addr[1] == 'X'))
				x = 1;
		}
	}
	if (x == 1)
		return 3;

	safe_bzero(addr);
	if (strstr(in_addr, "/")) {
		x = 0;
		while (x < 15 && in_addr[x] != '/') {
			addr[x] = in_addr[x];
			x++;
		}
	} else {
		safe_strncpy(addr, in_addr);
	}

	/*
	 * This also calls validate_v4addr.
	 */
	if (quadtonum(addr, &sm))
		return 0;

	if (!(sl = strstr(in_addr, "/"))) {
		y = 0;
		for (x = 0; x < 32; x++) {
			if (!y && !((sm >> (31 - x)) & 1))
				y = 1;
			if (y == 1 && ((sm >> (31 - x)) & 1))
				y = 2;
		}
		if (y == 2)
			return -1;

		/*
		 * valid
		 */
		return 1;
	}

	sl++;
	if (strlen(sl) < 1 || strlen(sl) > 2)
		return 0;

	if (sl[0] == '.' || sl[1] == '.')
		return 0;

	x = 0;
	y = 0;
	while (x < strlen(sl)) {
		z = 0;
		y = 0;
		while (z < strlen(V4ADDR_VAL) && !y) {
			if (sl[x] == V4ADDR_VAL[z])
				y = 1;
			z++;
		}
		if (!y)
			return 0;
		x++;
	}

	m = atoi(sl);
	if (m < 0 || m > 32)
		return 0;

	return 2;
}

int
getsplitnumv4(char *buf, u_int32_t *splitmask)
{
	int x, y;
	u_int32_t sm;

	if (strlen(buf) < 1)
		return -1;

	if (strlen(buf) < 4) {
		if (buf[0] == '/')
			buf++;
		y = atoi(buf);
		if (y < 1 || y > 32)
			return -1;
		sm = 0;
		for (x = 0; x < y; x++)
			sm = sm | (1 << (31 - x));
	} else {
		if (!strstr(buf, "."))
			sm = strtoul(buf, (char **)NULL, 16);
		else {
			if (quadtonum(buf, &sm))
				return -1;
		}
	}
	y = 0;
	for (x = 0; x < 32; x++) {
		if (!y && !((sm >> (31 - x)) & 1))
			y = 1;
		if (y == 1 && ((sm >> (31 - x)) & 1))
			y = 2;
	}
	if (y == 2)
		return -1;

	*splitmask = sm;

	return 0;
}

int
getsplitnumv6(char *buf, struct sip_in6_addr *splitmask, int *v6splitnum)
{
	int x, y;

	if (strlen(buf) < 1)
		return -1;
	if (strlen(buf) > 4)
		return -1;

	if (buf[0] == '/')
		buf++;
	y = atoi(buf);
	if (y < 1 || y > 128)
		return -1;
	*v6splitnum = y;
	x = v6masktonum(buf, &y, splitmask);

	return 0;
}

int
quadtonum(char *quad, u_int32_t *num)
{
	char buf[128];
	int x, y, z;

	if (!validate_v4addr(quad))
		return -1;

	safe_bzero(buf);
	x = 0;
	while (quad[x] != '.') {
		buf[x] = quad[x];
		x++;
	}
	*num = 0;
	x++;
	for (y = 0; y < 4; y++) {
		z = atoi(buf);
		if (z > 255 || z < 0)
			return -1;
		*num = *num | (z << (8 * (3 - y)));
		safe_bzero(buf);
		z = 0;
		while (quad[x] != '.' && quad[x] != '\0' && x < strlen(quad)) {
			buf[z] = quad[x];
			x++;
			z++;
		}
		x++;
	}

	return 0;
}

char *
numtoquad(u_int32_t num)
{
	int a[4], x;
	static char quad[17];

	for (x = 0; x < 4; x++)
		a[x] = num >> (8 * (3 - x)) & 0xff;
	safe_bzero(quad);
	safe_snprintf(quad, "%d.%d.%d.%d", a[0], a[1], a[2], a[3]);

	return quad;
}

char *
numtobitmap(u_int32_t num)
{
	static char bitmap[36];
	int x, y, z;

	safe_bzero(bitmap);
	y = 1;
	z = 0;
	for (x = 0; x < 32; x++) {
		if (!((num >> (31 - x)) & 1))
			bitmap[z] = '0';
		else
			bitmap[z] = '1';
		if (y == 8 && z < 34) {
			z++;
			bitmap[z] = '.';
			y = 0;
		}
		y++;
		z++;
	}

	return bitmap;
}

int
parse_addr(struct if_info *ifi)
{
	char buf[128], buf2[128];
	char *s_find;
	int x, y, z;

	safe_bzero(buf);
	safe_bzero(buf2);
	ifi->v4ad.n_nmaskbits = 0;

	/*
	 * netmask
	 */
	if (ifi->p_v4nmask[0] == '\0') {
		/*
		 * "/netmask"
		 */
		if (!(s_find = strstr(ifi->p_v4addr, "/"))) {
			ifi->v4ad.n_nmask = 0xffffffff;
			ifi->v4ad.n_nmaskbits = 32;
		} else {
			*s_find = '\0';
			s_find++;
			if (!*s_find)
				return -1;
			if (strlen(s_find) > 2)
				return -2;

			x = 0;
			y = 0;
			while (x < strlen(s_find)) {
				y = 0;
				z = 0;
				while (z < strlen(NETMASK_VAL) && !y) {
					if (s_find[x] == NETMASK_VAL[z])
						y = 1;
					z++;
				}
				if (!y)
					return -2;
				x++;
			}

			buf[0] = *s_find;
			*s_find = '\0';
			s_find++;
			if (*s_find) {
				buf[1] = *s_find;
				*s_find = '\0';
			}

			ifi->v4ad.n_nmaskbits = atoi(buf);

			ifi->v4ad.n_nmask = 0;
			for (x = 0; x < ifi->v4ad.n_nmaskbits; x++)
				ifi->v4ad.n_nmask = ifi->v4ad.n_nmask |
				    (1 << (31 - x));
		}
	} else {
		/*
		 * hex netmask
		 */
		if (!strstr(ifi->p_v4nmask, "."))
			ifi->v4ad.n_nmask = strtoul(ifi->p_v4nmask,
			    (char **)NULL, 16);
		/*
		 * "normal" netmask
		 */
		else {
			if (quadtonum(ifi->p_v4nmask, &ifi->v4ad.n_nmask))
				return -2;
		}
	}

	if (ifi->v4ad.n_nmaskbits < 0 || ifi->v4ad.n_nmaskbits > 32)
		return -2;

	/*
	 * host address
	 */
	if (quadtonum(ifi->p_v4addr, &ifi->v4ad.n_haddr))
		return -1;

	return 0;
}

/*
 * return
 * 0  ok
 * -1 invalid address
 * -2 invalid netmask
 */
int
get_addrv4(struct if_info *ifi)
{
	int x, y;

	x = 0;
	if (!ifi->name[0])
		x = parse_addr(ifi);

	if (x)
		return x;

	/*
	 * count netmask bits
	 */
	ifi->v4ad.n_nmaskbits = 0;
	for (x = 0; x < 32; x++) {
		if ((ifi->v4ad.n_nmask >> x) & 1)
			ifi->v4ad.n_nmaskbits++;
	}
	if (ifi->v4ad.n_nmaskbits < 0 || ifi->v4ad.n_nmaskbits > 32)
		return -2;

	/*
	 * validate netmask
	 */
	y = 0;
	for (x = 0; x < 32; x++) {
		if (!y && !((ifi->v4ad.n_nmask >> (31 - x)) & 1))
			y = 1;
		if (y == 1 && ((ifi->v4ad.n_nmask >> (31 - x)) & 1))
			y = 2;
	}
	if (y == 2)
		return -2;

	/*
	 * network class, class remark and classful netmask
	 */
	safe_bzero(ifi->v4ad.class_remark);
	x = ifi->v4ad.n_haddr >> 24;
	ifi->v4ad.n_cnaddr = 0;
	if (!(x & 0x80)) {
		ifi->v4ad.class = 'A';
		ifi->v4ad.n_cnmask = 0xff000000;
	}
	if ((x & 0xc0) == 0x80) {
		ifi->v4ad.class = 'B';
		ifi->v4ad.n_cnmask = 0xffff0000;
	}
	if ((x & 0xe0) == 0xc0) {
		ifi->v4ad.class = 'C';
		ifi->v4ad.n_cnmask = 0xffffff00;
	}
	if ((x & 0xf0) == 0xe0) {
		ifi->v4ad.class = 'D';
		safe_snprintf(ifi->v4ad.class_remark, " (multicast network)");
		ifi->v4ad.n_cnmask = ifi->v4ad.n_nmask;
	}
	if ((x & 0xf8) == 0xf0) {
		ifi->v4ad.class = 'E';
		safe_snprintf(ifi->v4ad.class_remark,
		    " (reserved for future use)");
		ifi->v4ad.n_cnmask = ifi->v4ad.n_nmask;
	}
	if (ifi->v4ad.class == '\0') {
		ifi->v4ad.n_cnmask = ifi->v4ad.n_nmask;
		safe_snprintf(ifi->v4ad.class_remark, "Nonexistant");
	}

	/*
	 * network address (classful + cidr)
	 */
	ifi->v4ad.n_naddr = ifi->v4ad.n_haddr & ifi->v4ad.n_nmask;
	ifi->v4ad.n_cnaddr = ifi->v4ad.n_haddr & ifi->v4ad.n_cnmask;

	/*
	 * cidr broadcast address
	 */
	ifi->v4ad.n_broadcast = 0xffffffff - ifi->v4ad.n_nmask +
	    ifi->v4ad.n_naddr;

	return 0;
}

/*
 * return
 * 0  ok
 * -1 invalid address
 * -2 invalid netmask
 */
int
get_addrv6(struct if_info *ifi)
{
	int x;

	x = mk_ipv6addr(&ifi->v6ad, ifi->p_v6addr);
	if (x < 0)
		return -1;

	return 0;
}

int
split_ipv6addr(char *addr, struct ipv6_split *spstr)
{
	char *split;
	int x, y, z;

	split = strstr(addr, "/");
	if (split && (count(addr, '/') == 1)) {
		if (strlen(split) > 1 && strlen(split) < 5) {
			split++;
			safe_strncpy(spstr->nmask, split);
		}
	}

	x = 0;
	y = 0;
	split = strstr(addr, ".");
	if (split)
		x = strlen(split);
	split = strstr(addr, ":");
	if (split)
		y = strlen(split);
	if (x < y)
		x = 1;
	else
		x = 0;

	if (count(addr, '.') == 3 && x) {
		split = strstr(addr, ".");
		x = strlen(addr) - strlen(split);
		while (addr[x] != ':')
			x--;
		x++;
		split = strstr(addr, "/");
		if (split)
			y = strlen(addr) - strlen(split);
		else
			y = strlen(addr);
		if ((y - x >= 7) && (y - x <= 15)) {
			for (z = 0; z < y - x; z++)
				spstr->ipv4addr[z] = addr[x + z];
		}
	}

	x = strlen(addr) - (strlen(spstr->ipv4addr) + strlen(spstr->nmask));
	if (strlen(spstr->nmask) > 0)
		x--;
	if (x > 1 && x < 40)
		strncpy(spstr->ipv6addr, addr, x);

	return 0;
}

int
validate_s_v6addr(char *addr, int type)
{
	int x, y, z;
	int numcolon;
	int compressed;

	if (strlen(addr) < 2)
		return -1;

	if (strlen(addr) > 39)
		return -1;

	x = 0;
	y = 0;
	while (x < strlen(addr)) {
		y = 0;
		z = 0;
		while (z < strlen(V6ADDR_VAL) && !y) {
			if (addr[x] == V6ADDR_VAL[z])
				y = 1;
			z++;
		}
		if (!y)
			return -1;
		x++;
	}

	x = 0;
	y = 0;
	while (x < strlen(addr)) {
		if (addr[x] == ':')
			y++;
		else
			y = 0;
		if (y == 3)
			return -1;

		x++;
	}

	if (addr[0] == ':' && addr[1] != ':')
		return -1;
	if (type == V6TYPE_STANDARD && addr[strlen(addr) - 1] == ':' &&
	    addr[strlen(addr) - 2] != ':')
		return -1;

	numcolon = 0;
	for (x = 0; x < strlen(addr); x++) {
		if (addr[x] == ':')
			numcolon++;
	}

	compressed = 0;
	x = 0;
	while (x < strlen(addr) - 1) {
		if (addr[x] == ':' && addr[x + 1] == ':')
			compressed++;
		x++;
	}

	if (compressed > 1)
		return -1;

	if (!compressed && numcolon != 7 && type == V6TYPE_STANDARD)
		return -1;

	if (!compressed && numcolon != 6 && type == V6TYPE_V4INV6)
		return -1;

	if (compressed && type == V6TYPE_STANDARD) {
		if (numcolon > 7)
			return -1;
	}

	if (compressed && type == V6TYPE_V4INV6) {
		if (numcolon > 6)
			return -1;
	}

	y = 0;
	for (x = 0; x < strlen(addr); x++) {
		if (addr[x] != ':')
			y++;
		else
			y = 0;

		if (y > 4)
			break;
	}

	if (y > 4)
		return -1;

	return 0;
}

int
getcolon(char *addr, int pos, int type)
{
	int x, y;
	int compressed;
	int cstart, cend;
	int max;
	char str[5];

	if (type == V6TYPE_STANDARD)
		max = 7;
	if (type == V6TYPE_V4INV6)
		max = 5;

	compressed = 0;
	x = 0;
	while (x < strlen(addr) - 1) {
		if (addr[x] == ':' && addr[x + 1] == ':')
			compressed++;
		x++;
	}

	if (compressed) {
		cstart = 0;
		x = 0;
		while (x < strlen(addr) - 1) {
			if (addr[x] == ':' && addr[x + 1] == ':')
				break;
			if (addr[x] == ':')
				cstart++;
			x++;
		}
		x += 2;
		cend = 0;
		while (x < strlen(addr)) {
			if (addr[x] == ':')
				cend++;
			x++;
		}
		if (addr[strlen(addr) - 1] == ':' &&
		    addr[strlen(addr) - 2] != ':')
			cend--;
	}

	if (!compressed) {
		x = 0;
		y = 0;
		while (x < pos) {
			if (addr[y] == ':')
				x++;
			y++;
		}

		safe_bzero(str);
		x = 0;
		while (y < strlen(addr) && addr[y] != ':') {
			str[x] = addr[y];
			x++;
			y++;
		}
	}

	if (compressed) {
		safe_bzero(str);
		if (pos <= cstart) {
			x = 0;
			y = 0;
			while (x < pos) {
				if (addr[y] == ':')
					x++;
				y++;
			}

			x = 0;
			while (y < strlen(addr) && addr[y] != ':') {
				str[x] = addr[y];
				x++;
				y++;
			}
		}

		if ((pos > cstart) && (pos < (max - cend))) {
			str[0] = '0';
		}

		if (pos >= (max - cend)) {
			y = 0;
			while (y < strlen(addr) - 1) {
				if (addr[y] == ':' && addr[y + 1] == ':')
					break;
				y++;
			}
			y += 2;

			x = max - cend;
			while (x < pos) {
				if (addr[y] == ':')
					x++;
				y++;
			}

			safe_bzero(str);
			x = 0;
			while (y < strlen(addr) && addr[y] != ':') {
				str[x] = addr[y];
				x++;
				y++;
			}
		}
	}

	if (str[0] == '\0')
		str[0] = '0';
	x = strtol(str, (char **)NULL, 16);

	return x;
}

int
v6addrtonum(struct ipv6_split spstr, struct v6addr *in6_addr, int type)
{
	int colon;
	int x, y, z, n;
	char buf[128];

	colon = 0;
	if (type == V6TYPE_STANDARD)
		colon = 8;
	if (type == V6TYPE_V4INV6)
		colon = 6;

	for (x = 0; x < 4; x++) {
		in6_addr->haddr.sip6_addr32[x] = 0;
	}

	for (x = 0; x < colon; x++) {
		y = getcolon(spstr.ipv6addr, x, type);
		in6_addr->haddr.sip6_addr16[x] = y;
	}

	if (type == V6TYPE_V4INV6) {
		safe_bzero(buf);
		x = 0;
		while (spstr.ipv4addr[x] != '.') {
			buf[x] = spstr.ipv4addr[x];
			x++;
		}
		x++;
		for (y = 0; y < 4; y++) {
			if (y == 1) {
				in6_addr->haddr.sip6_addr16[6] = (n << 8) |
				    atoi(buf);
			}
			if (y == 3) {
				in6_addr->haddr.sip6_addr16[7] = (n << 8) |
				    atoi(buf);
			}
			n = atoi(buf);

			safe_bzero(buf);
			z = 0;
			while (spstr.ipv4addr[x] != '.' &&
			    spstr.ipv4addr[x] != '\0' &&
			    x < strlen(spstr.ipv4addr)) {
				buf[z] = spstr.ipv4addr[x];
				x++;
				z++;
			}
			x++;
		}
	}

	return 0;
}

int
v6masktonum(char *nmask, int *nmaskbits, struct sip_in6_addr *in6_addr)
{
	int x, y, z;

	if (nmask[0] == '\0')
		*nmaskbits = 128;
	else
		*nmaskbits = atoi(nmask);

	for (x = 0; x < 4; x++)
		in6_addr->sip6_addr32[x] = 0;

	y = 0;
	z = 0;
	for (x = 0; x < *nmaskbits; x++) {
		in6_addr->sip6_addr16[y] = in6_addr->sip6_addr16[y] |
		    (1 << (15 - z));
		z++;
		if (z == 16) {
			z = 0;
			y++;
		}
	}

	return 0;
}

/*
 * 0 error
 * 1 normal
 */
int
validate_v6addr(char *addr)
{
	int x;
	struct ipv6_split spstr;

	safe_bzero(spstr.ipv6addr);
	safe_bzero(spstr.ipv4addr);
	safe_bzero(spstr.nmask);

	split_ipv6addr(addr, &spstr);

	if (!spstr.ipv6addr[0])
		return 0;

	x = V6TYPE_STANDARD;
	if (spstr.ipv4addr[0])
		x = V6TYPE_V4INV6;
	if (validate_s_v6addr(spstr.ipv6addr, x) < 0) {
		return 0;
	}
	if (spstr.ipv4addr[0]) {
		if (!validate_v4addr(spstr.ipv4addr)) {
			return 0;
		}
	}

	return 1;
}

int
v6addrtoprefix(struct v6addr *in6_addr)
{
	int x;

	for (x = 0; x < 8; x++) {
		in6_addr->prefix.sip6_addr16[x] =
		    in6_addr->haddr.sip6_addr16[x] &
		    in6_addr->nmask.sip6_addr16[x];
	}

	return 0;
}

int
v6addrtosuffix(struct v6addr *in6_addr)
{
	int x;

	for (x = 0; x < 8; x++) {
		in6_addr->suffix.sip6_addr16[x] =
		    in6_addr->haddr.sip6_addr16[x] &
		    (in6_addr->nmask.sip6_addr16[x] ^ 0xffff);
	}

	return 0;
}

int
v6addrtobroadcast(struct v6addr *in6_addr)
{
	int x;

	for (x = 0; x < 8; x++) {
		in6_addr->broadcast.sip6_addr16[x] = 0xffff -
		    in6_addr->nmask.sip6_addr16[x] +
		    in6_addr->prefix.sip6_addr16[x];
	}

	return 0;
}

void
v6_type(struct v6addr *in6_addr)
{
	uint16_t a;

	a = in6_addr->haddr.sip6_addr16[0];

	if (a == 0)
		safe_snprintf(in6_addr->class_remark, "Reserved");
	if (a == 2 || a == 3)
		safe_snprintf(in6_addr->class_remark,
		    "Reserved for NSAP Allocation");
	if (a == 4 || a == 5)
		safe_snprintf(in6_addr->class_remark,
		    "Reserved for IPX Allocation");
	if ((a & 0xe000) == 0x2000)
		safe_snprintf(in6_addr->class_remark,
		    "Aggregatable Global Unicast Addresses");
	if ((a | 0x00ff) == 0x00ff)
		safe_snprintf(in6_addr->class_remark, "Reserved");
	if ((a & 0xff00) == 0xff00)
		safe_snprintf(in6_addr->class_remark, "Multicast Addresses");
	if ((a & 0xff80) == 0xfe80)
		safe_snprintf(in6_addr->class_remark,
		    "Link-Local Unicast Addresses");
	if ((a & 0xffc0) == 0xfec0)
		safe_snprintf(in6_addr->class_remark,
		    "Site-Local Unicast Addresses");

	if (in6_addr->class_remark[0] == '\0')
		safe_snprintf(in6_addr->class_remark, "Unassigned");

	return;
}

void
v6_comment(struct v6addr *in6_addr)
{
	int x, y;

	y = 0;
	for (x = 0; x < 8; x++) {
		if (in6_addr->haddr.sip6_addr16[x])
			y = 1;
	}
	if (!y)
		safe_snprintf(in6_addr->comment, "Unspecified");

	y = 0;
	for (x = 0; x < 7; x++) {
		if (in6_addr->haddr.sip6_addr16[x])
			y = 1;
	}
	if (!y)
		if (in6_addr->haddr.sip6_addr16[7] == 1)
			safe_snprintf(in6_addr->comment, "Loopback");

	return;
}

int
v6verifyv4(struct sip_in6_addr addr)
{
	int x, y;

	y = 0;
	for (x = 0; x < 5; x++) {
		if (addr.sip6_addr16[x])
			y = 1;
	}

	if (!y) {
		if (!addr.sip6_addr16[5])
			return 1;
		if (addr.sip6_addr16[5] == 0xffff)
			return 2;
	}

	return 0;
}

char *
get_comp_v6(struct sip_in6_addr addr)
{
	static char outad[44];
	char tmpad[5];
	int x, y, z;
	int start, num;

	safe_bzero(outad);

	start = -1;
	num = 0;
	y = 0;
	z = 0;
	for (x = 0; x < 8; x++) {
		if (addr.sip6_addr16[x] == 0) {
			if (y == -1)
				y = x;
			z++;
		} else {
			if (z > num && z > 1) {
				start = y;
				num = z;
			}
			y = -1;
			z = 0;
		}
	}

	if (z > num && z > 1) {
		start = y;
		num = z;
	}

	for (x = 0; x < 8; x++) {
		if (x == start) {
			if (!x)
				safe_strncat(outad, ":");
			safe_strncat(outad, ":");
			x += num - 1;
		} else {
			safe_bzero(tmpad);
			safe_snprintf(tmpad, "%x", addr.sip6_addr16[x]);
			safe_strncat(outad, tmpad);
			if (x != 7)
				safe_strncat(outad, ":");
		}
	}

	return outad;
}

int
mk_ipv6addr(struct v6addr *in6_addr, char *addr)
{
	int x, y, z;
	struct ipv6_split spstr;

	safe_bzero(spstr.ipv6addr);
	safe_bzero(spstr.ipv4addr);
	safe_bzero(spstr.nmask);

	split_ipv6addr(addr, &spstr);

	if (!spstr.ipv6addr[0])
		return -1;

	x = V6TYPE_STANDARD;
	if (spstr.ipv4addr[0])
		x = V6TYPE_V4INV6;
	if (validate_s_v6addr(spstr.ipv6addr, x) < 0) {
		return -1;
	}
	if (spstr.ipv4addr[0]) {
		if (!validate_v4addr(spstr.ipv4addr)) {
			return -1;
		}
	}

	x = 0;
	y = 0;
	while (x < strlen(spstr.nmask)) {
		y = 0;
		z = 0;
		while (z < strlen(NETMASK_VAL) && !y) {
			if (spstr.nmask[x] == NETMASK_VAL[z])
				y = 1;
			z++;
		}
		if (!y) {
			return -1;
		}
		x++;
	}
	x = atoi(spstr.nmask);
	if (x < 0 || x > 128) {
		return -1;
	}

	if (spstr.ipv4addr[0] == '\0')
		in6_addr->type = V6TYPE_STANDARD;
	else
		in6_addr->type = V6TYPE_V4INV6;

	v6addrtonum(spstr, in6_addr, in6_addr->type);
	v6masktonum(spstr.nmask, &in6_addr->nmaskbits, &in6_addr->nmask);
	v6addrtoprefix(in6_addr);
	v6addrtosuffix(in6_addr);
	v6addrtobroadcast(in6_addr);
	in6_addr->real_v4 = v6verifyv4(in6_addr->haddr);

	safe_bzero(in6_addr->class_remark);
	v6_type(in6_addr);
	safe_bzero(in6_addr->comment);
	v6_comment(in6_addr);

	return 0;
}

struct dnsresp *
new_dnsresp(struct dnsresp *d_resp)
{
	d_resp->next = (struct dnsresp *)malloc(sizeof(struct dnsresp));
	d_resp = d_resp->next;
	d_resp->next = NULL;
	safe_bzero(d_resp->str);
	d_resp->type = 0;

	return d_resp;
}

void
free_dnsresp(struct dnsresp *d_resp)
{
	struct dnsresp *old;

	while (d_resp) {
		old = d_resp;
		d_resp = d_resp->next;
		free(old);
	}
}

#ifdef HAVE_GETHOSTBYNAME
char *
_resolv_v4_ghbn(char *raddr, struct dnsresp *d_resp, char *extra)
{
	struct hostent *he;
	static char retaddr[1024];
	int x;

	safe_bzero(retaddr);

	he = gethostbyname(raddr);
	if (!he)
		return NULL;

	if (he->h_addrtype == AF_INET) {
		safe_snprintf(retaddr, "%s%s",
		    inet_ntoa(*(struct in_addr *)he->h_addr_list[0]), extra);
		x = 0;
		while (he->h_addr_list[x]) {
			safe_snprintf(d_resp->str, "%s%s",
			    inet_ntoa(*(struct in_addr *)he->h_addr_list[x]),
			    extra);
			d_resp->type = AF_INET;
			x++;
			if (he->h_addr_list[x])
				d_resp = new_dnsresp(d_resp);
		}
	}

	if (retaddr[0] == '\0')
		return NULL;

	return retaddr;
}
#else
char *
_resolv_v4_ghbn(char *raddr, struct dnsresp *d_resp, char *extra)
{
	return NULL;
}
#endif

#if defined(HAVE_GETHOSTBYNAME2) && defined(HAVE_INET_NTOP)
char *
_resolv_v6_ghbn2(char *raddr, struct dnsresp *d_resp, char *extra)
{
	struct hostent *he;
	static char retaddr[1024];
	char ip6addr[128];
	int x;

	safe_bzero(retaddr);

	he = gethostbyname2(raddr, AF_INET6);
	if (!he)
		return NULL;

	if (he->h_addrtype == AF_INET6) {
		safe_bzero(ip6addr);
		safe_snprintf(retaddr, "%s%s",
		    inet_ntop(AF_INET6, he->h_addr_list[0], ip6addr, 128),
		    extra);
		x = 0;
		while (he->h_addr_list[x]) {
			safe_snprintf(d_resp->str, "%s%s",
			    inet_ntop(AF_INET6, he->h_addr_list[x], ip6addr,
				128),
			    extra);
			d_resp->type = AF_INET6;
			x++;
			if (he->h_addr_list[x])
				d_resp = new_dnsresp(d_resp);
		}
	}

	if (retaddr[0] == '\0')
		return NULL;

	return retaddr;
}
#else
char *
_resolv_v6_ghbn2(char *raddr, struct dnsresp *d_resp, char *extra)
{
	return NULL;
}
#endif

#if defined(HAVE_GETADDRINFO) && defined(HAVE_INET_NTOP)
char *
_resolv_v6_gai(char *raddr, struct dnsresp *d_resp, char *extra)
{
	int x;
	struct addrinfo hints;
	struct addrinfo *res, *res_orig;
	struct sockaddr_in6 *sin6;
	static char retaddr[1024];
	char ip6addr[128];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = 0;
	hints.ai_flags = AI_CANONNAME;
	hints.ai_protocol = IPPROTO_TCP;
	x = getaddrinfo(raddr, NULL, &hints, &res);
	if (x)
		return NULL;
	if (!res)
		return NULL;

	res_orig = res;
	while (res) {
		safe_bzero(ip6addr);
		if (res->ai_family == PF_INET6) {
			sin6 = (struct sockaddr_in6 *)res->ai_addr;
			safe_snprintf(retaddr, "%s%s",
			    inet_ntop(AF_INET6, &sin6->sin6_addr, ip6addr, 128),
			    extra);
			safe_snprintf(d_resp->str, "%s%s",
			    inet_ntop(AF_INET6, &sin6->sin6_addr, ip6addr, 128),
			    extra);
			d_resp->type = AF_INET6;
		}
		if (res->ai_next &&
		    (res->ai_family == PF_INET || res->ai_family == PF_INET6))
			d_resp = new_dnsresp(d_resp);
		res = res->ai_next;
	}

	freeaddrinfo(res_orig);

	if (retaddr[0] == '\0')
		return NULL;

	return retaddr;
}
#else
char *
_resolv_v6_gai(char *raddr, struct dnsresp *d_resp, char *extra)
{
	return NULL;
}
#endif

#if defined(HAVE_GETADDRINFO) && defined(HAVE_INET_NTOP)
char *
_resolv_unspec_gai(char *raddr, struct dnsresp *d_resp, char *extra)
{
	int x;
	struct addrinfo hints;
	struct addrinfo *res, *res_orig;
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;
	static char retaddr[1024];
	char ip6addr[128];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = 0;
	hints.ai_flags = AI_CANONNAME;
	hints.ai_protocol = IPPROTO_TCP;
	x = getaddrinfo(raddr, NULL, &hints, &res);
	if (x)
		return NULL;
	if (!res)
		return NULL;

	res_orig = res;

	while (res) {
		safe_bzero(ip6addr);
		if (res->ai_family == PF_INET) {
			sin = (struct sockaddr_in *)res->ai_addr;
			safe_snprintf(retaddr, "%s%s", inet_ntoa(sin->sin_addr),
			    extra);
			safe_snprintf(d_resp->str, "%s%s",
			    inet_ntoa(sin->sin_addr), extra);
			d_resp->type = AF_INET;
		}
		if (res->ai_family == PF_INET6) {
			sin6 = (struct sockaddr_in6 *)res->ai_addr;
			safe_snprintf(retaddr, "%s%s",
			    inet_ntop(AF_INET6, &sin6->sin6_addr, ip6addr, 128),
			    extra);
			safe_snprintf(d_resp->str, "%s%s",
			    inet_ntop(AF_INET6, &sin6->sin6_addr, ip6addr, 128),
			    extra);
			d_resp->type = AF_INET6;
		}
		if (res->ai_next &&
		    (res->ai_family == PF_INET || res->ai_family == PF_INET6))
			d_resp = new_dnsresp(d_resp);
		res = res->ai_next;
	}

	freeaddrinfo(res_orig);

	if (retaddr[0] == '\0')
		return NULL;

	return retaddr;
}
#else
char *
_resolv_unspec_gai(char *raddr, struct dnsresp *d_resp, char *extra)
{
	return NULL;
}
#endif

char *
resolve_addr(char *addr, int family, struct dnsresp *d_resp)
{
	char extra[32];
	char *tmpstr;
	char raddr[1024];
	static char retaddr[1024];
	struct dnsresp *d_resp_tmp;
	int f_gethostbyname;
	int f_gethostbyname2;
	int f_getaddrinfo;
	int f_inet_ntop;
	int ipv4_cap;
	int ipv6_cap;

	f_gethostbyname = 0;
	f_gethostbyname2 = 0;
	f_getaddrinfo = 0;
	f_inet_ntop = 0;
	ipv4_cap = 0;
	ipv6_cap = 0;

#ifdef HAVE_GETHOSTBYNAME
	f_gethostbyname = 1;
#endif
#ifdef HAVE_GETHOSTBYNAME2
	f_gethostbyname2 = 1;
#endif
#ifdef HAVE_GETADDRINFO
	f_getaddrinfo = 1;
#endif
#ifdef HAVE_INET_NTOP
	f_inet_ntop = 1;
#endif

	if (f_gethostbyname)
		ipv4_cap = 1;

	if ((f_gethostbyname2 || f_getaddrinfo) && f_inet_ntop)
		ipv6_cap = 1;

	if (family != PF_INET && family != PF_INET6 && family != PF_UNSPEC)
		return NULL;

	if (family == PF_INET && !ipv4_cap)
		return NULL;

	if (family == PF_INET6 && !ipv6_cap)
		return NULL;

	if (family == PF_UNSPEC && (!ipv4_cap && !ipv6_cap))
		return NULL;

	if (strlen(addr) > 1023)
		return NULL;

	if (family == PF_UNSPEC && !ipv4_cap)
		family = PF_INET6;

	if (family == PF_UNSPEC && !ipv6_cap)
		family = PF_INET;

	safe_bzero(extra);
	safe_bzero(raddr);
	tmpstr = strstr(addr, "/");
	if (tmpstr) {
		safe_strncpy(extra, tmpstr);
		strncpy(raddr, addr, strlen(addr) - strlen(tmpstr));
	} else {
		tmpstr = strstr(addr, " ");
		if (tmpstr) {
			safe_strncpy(extra, tmpstr);
			strncpy(raddr, addr, strlen(addr) - strlen(tmpstr));
		} else
			safe_strncpy(raddr, addr);
	}

	safe_bzero(retaddr);

	if (family == PF_INET) {
		tmpstr = _resolv_v4_ghbn(raddr, d_resp, extra);
		if (!tmpstr)
			return NULL;
		safe_strncpy(retaddr, tmpstr);
		return retaddr;
	}

	if (family == PF_INET6) {
		if (f_getaddrinfo) {
			tmpstr = _resolv_v6_gai(raddr, d_resp, extra);
			if (!tmpstr)
				return NULL;
			safe_strncpy(retaddr, tmpstr);
			return retaddr;
		}

		if (f_gethostbyname2) {
			tmpstr = _resolv_v6_ghbn2(raddr, d_resp, extra);
			if (!tmpstr)
				return NULL;
			safe_strncpy(retaddr, tmpstr);
			return retaddr;
		}
	}

	if (family == PF_UNSPEC) {
		if (f_getaddrinfo) {
			tmpstr = _resolv_unspec_gai(raddr, d_resp, extra);
			if (!tmpstr)
				return NULL;
			safe_strncpy(retaddr, tmpstr);
			return retaddr;
		}
		if (f_gethostbyname && f_gethostbyname2) {
			tmpstr = _resolv_v4_ghbn(raddr, d_resp, extra);
			if (tmpstr) {
				safe_strncpy(retaddr, tmpstr);
				d_resp_tmp = d_resp;
				d_resp = new_dnsresp(d_resp);
			}
			tmpstr = _resolv_v6_ghbn2(raddr, d_resp, extra);
			if (!tmpstr) {
				d_resp_tmp->next = NULL;
				free(d_resp);
			}
			return retaddr;
		}
	}

	return NULL;
}
