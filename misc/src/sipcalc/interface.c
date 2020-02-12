/*
 * sipcalc, interface.c
 * 
 * $Id: interface.c,v 1.14 2003/03/19 12:28:15 simius Exp $
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

static const char rcsid[] = "$Id: interface.c,v 1.14 2003/03/19 12:28:15 simius Exp $";

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
/*
 * For SIOCGIFCONF and friends on Solaris.
 */
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#include "sub.h"

struct if_info *
new_if (struct if_info *ifarg_cur)
{
	struct if_info *n_if;

	ifarg_cur->next = (struct if_info *) malloc (sizeof (struct if_info));
	n_if = ifarg_cur->next;
	n_if->next = NULL;
	bzero ((char *) n_if->name, IFNAMSIZ + 1);
	safe_bzero (n_if->p_v4addr);
	safe_bzero (n_if->p_v4nmask);
	safe_bzero (n_if->errorstr);
	safe_bzero (n_if->cmdstr);
	n_if->type = 0;

	return n_if;
}

void
free_if (struct if_info *if_cur)
{
	struct if_info *if_old;

	while (if_cur) {
		if_old = if_cur;
		if_cur = if_cur->next;
		free (if_old);
	}
}

struct if_info *
get_if_ext ()
{
	int sd, size, prev_size, len, ifreq_sum;
	struct ifreq *ifr, ifr_stat;
	struct ifconf ifc;
	char *buf, *ptr;
	struct if_info *if_start, *if_cur;
	struct sockaddr_in *sin;

	if ((sd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror ("socket");
		return NULL;
	};

	prev_size = 0;
	size = 5 * sizeof (struct ifreq);
	for (;;) {
		if (!(buf = malloc (size))) {
			perror ("malloc");
			return NULL;
		}
		ifc.ifc_len = size;
		ifc.ifc_buf = buf;
		if (ioctl (sd, SIOCGIFCONF, &ifc) < 0) {
			if (errno != EINVAL || prev_size) {
				perror ("ioctl");
				return NULL;
			}
		} else {
			if (ifc.ifc_len == prev_size)
				break;
			prev_size = ifc.ifc_len;
		}
		size += 5 * sizeof (struct ifreq);
		free (buf);
	}

	if_cur = NULL;
	if_start = NULL;
	ptr = buf;
	len = 0;
	ifreq_sum = 0;
	while (ptr < buf + ifc.ifc_len) {
		if (!if_start) {
			if_cur = if_start =
			    (struct if_info *) malloc (sizeof (struct if_info));
			if_cur->next = NULL;
		} else {
			if_cur->next =
			    (struct if_info *) malloc (sizeof (struct if_info));
			if_cur = if_cur->next;
			if_cur->next = NULL;
		}

		ifr = (struct ifreq *) ptr;

		while ((ptr < buf + ifc.ifc_len) && ifr
		       && ifr->ifr_addr.sa_family != AF_INET) {
/*
 * This is how it's done in W. Richard Stevens, Unix Network Programming
 * Volume 1 Second Edition. This doesnt work on certain 64bit machines
 * (linux - alpha). Hopefully the version below works everywhere.
 *
#ifdef HAVE_SA_LEN
			if(ifr->ifr_addr.sa_len > sizeof(struct sockaddr))
				len=ifr->ifr_addr.sa_len+sizeof(ifr->ifr_name);
			else
				len=sizeof(struct sockaddr)+sizeof(ifr->ifr_name);
#else
			len=sizeof(struct sockaddr)+sizeof(ifr->ifr_name);
#endif
*/
#ifdef HAVE_SA_LEN
			if ((ifr->ifr_addr.sa_len > sizeof (struct sockaddr)) &&
			    ifr->ifr_addr.sa_len >=
			    (sizeof (struct ifreq) - sizeof (ifr->ifr_name)))
				len =
				    ifr->ifr_addr.sa_len +
				    sizeof (ifr->ifr_name);
			else
				len = sizeof (struct ifreq);
#else
			len = sizeof (struct ifreq);
#endif
			ptr += len;
			ifr = (struct ifreq *) ptr;
		}

		if (!ifr || ptr >= buf + ifc.ifc_len)
			break;

		/*
		 * We don't know if ifr->ifr_name is NULL terminated.
		 */
		bzero ((char *) if_cur->name, IFNAMSIZ + 1);
		strncpy (if_cur->name, ifr->ifr_name, IFNAMSIZ);
		sin = (struct sockaddr_in *) &ifr->ifr_addr;
		if_cur->v4ad.n_haddr = htonl (sin->sin_addr.s_addr);
		ifr_stat = *ifr;

		ioctl (sd, SIOCGIFFLAGS, &ifr_stat);
		if_cur->flags = ifr_stat.ifr_flags;

		/*
		 * *BSD's struct ifreq doesn't containt struct sockaddr
		 * ifru_netmask, but using ifru_addr should hopefully
		 * be ok on all platforms (ifr_ifru being a union and all).
		 */
		ioctl (sd, SIOCGIFNETMASK, &ifr_stat);
		sin = (struct sockaddr_in *) &ifr_stat.ifr_addr;
		if_cur->v4ad.n_nmask = htonl (sin->sin_addr.s_addr);

#ifdef SIOCGIFBRDADDR
		if ((if_cur->flags & IFF_BROADCAST) == IFF_BROADCAST) {
			ioctl (sd, SIOCGIFBRDADDR, &ifr_stat);
			sin = (struct sockaddr_in *) &ifr_stat.ifr_broadaddr;
			if_cur->v4ad.i_broadcast = htonl (sin->sin_addr.s_addr);
		}
#endif

#ifdef HAVE_SA_LEN
		if ((ifr->ifr_addr.sa_len > sizeof (struct sockaddr)) &&
		    ifr->ifr_addr.sa_len >=
		    (sizeof (struct ifreq) - sizeof (ifr->ifr_name)))
			len = ifr->ifr_addr.sa_len + sizeof (ifr->ifr_name);
		else
			len = sizeof (struct ifreq);
#else
		len = sizeof (struct ifreq);
#endif
		ptr += len;
	}

	free (buf);

	return if_start;
}
