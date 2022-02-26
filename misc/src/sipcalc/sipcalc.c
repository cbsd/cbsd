/*
 * sipcalc, sub.c
 *
 * $Id: sub.c,v 1.39 2003/03/19 12:28:16 simius Exp $
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
    "$Id: sub.c,v 1.39 2003/03/19 12:28:16 simius Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include "sub.h"
#include "sub-o.h"

extern char *optarg;
extern int optind, opterr, optopt;
int resolve;

int
out_cmdline(struct if_info *ifarg_cur, int v4args, struct misc_args m_argv4,
    int v6args, struct misc_args m_argv6, int recurse, int index)
{
	int ret;

	ret = 0;

	if (ifarg_cur->type == IFT_V4) {
		//		printf ("-[ipv4 : %s] - %d\n", ifarg_cur->cmdstr,
		//index);
		ret = get_addrv4(ifarg_cur);
	}

	if (ifarg_cur->type == IFT_V6) {
		//		printf ("-[ipv6 : %s] - %d\n", ifarg_cur->cmdstr,
		//index);
		ret = get_addrv6(ifarg_cur);
	}

	if (ifarg_cur->type == IFT_INTV4 || ifarg_cur->type == IFT_INTV6) {
		//		printf ("-[int-ipv4 : %s] - %d\n",
		//ifarg_cur->cmdstr, index);
		if (ifarg_cur->errorstr[0] != '\0') {
			printf("\n-[ERR : %s]\n\n-\n", ifarg_cur->errorstr);
			return 1;
		}

		ret = get_addrv4(ifarg_cur);
	}

	if (ifarg_cur->type == IFT_UNKWN) {
		printf("-[unknown : %s] - %d\n", ifarg_cur->cmdstr, index);
		printf("\n-[ERR : %s]\n\n-\n", ifarg_cur->errorstr);
		return 1;
	}

	if (ret == -1) {
		printf("\n-[ERR : Invalid address]\n\n-\n");
		return 1;
	}
	if (ret == -2) {
		printf("\n-[ERR : Invalid netmask]\n\n-\n");
		return 1;
	}

	if (ifarg_cur->type == IFT_V4 || ifarg_cur->type == IFT_INTV4) {
		if (!v4args)
			v4args = CIDR_INFO;

		//		printf ("\n");
		if ((v4args & CF_INFO) == CF_INFO)
			print_cf_info_v4(ifarg_cur);
		if ((v4args & CIDR_INFO) == CIDR_INFO)
			print_cidr_info_v4(ifarg_cur);
		if ((v4args & CF_BITMAP) == CF_BITMAP)
			print_cf_bitmap_v4(ifarg_cur);
		if ((v4args & CIDR_BITMAP) == CIDR_BITMAP)
			print_cidr_bitmap_v4(ifarg_cur);
		if ((v4args & NET_INFO) == NET_INFO)
			show_networks_v4(ifarg_cur, m_argv4.numnets);
		if ((v4args & V4SPLIT) == V4SPLIT)
			show_split_networks_v4(ifarg_cur, m_argv4.splitmask,
			    v4args, m_argv4);
		if ((v4args & C_WILDCARD) == C_WILDCARD)
			show_c_wildcard_info_v4(ifarg_cur);
		//		printf ("-\n");
	}

	if (ifarg_cur->type == IFT_V6 || ifarg_cur->type == IFT_INTV6) {
		if (!v6args)
			v6args = V6_INFO;

		//		printf ("\n");
		if ((v6args & V6_INFO) == V6_INFO)
			print_v6(ifarg_cur);
		if ((v6args & V4INV6) == V4INV6)
			print_v4inv6(ifarg_cur);
		if ((v6args & V6REV) == V6REV)
			print_rev_v6(ifarg_cur);
		if ((v6args & V6SPLIT) == V6SPLIT)
			show_split_networks_v6(ifarg_cur, m_argv6.v6splitmask,
			    v6args, m_argv6);
		//		printf ("-\n");
	}

	return 0;
}

int
cleanline(char *sbuf, char *dbuf)
{
	int x, y;

	for (x = 0; x < strlen(sbuf); x++) {
		if (sbuf[x] == '\n')
			sbuf[x] = ' ';
		if (sbuf[x] == '\t')
			sbuf[x] = ' ';
		if (sbuf[x] == '#')
			sbuf[x] = '\0';
	}
	x = strlen(sbuf) - 1;
	while (sbuf[x] == ' ' && x > -1) {
		sbuf[x] = '\0';
		x--;
	}
	if (!strlen(sbuf))
		return 0;

	x = 0;
	y = 0;
	while (x < strlen(sbuf)) {
		if (sbuf[x] == ' ' && x) {
			dbuf[y] = ' ';
			y++;
		}
		while (sbuf[x] == ' ' && x < strlen(sbuf))
			x++;
		while (sbuf[x] != ' ' && x < strlen(sbuf)) {
			dbuf[y] = sbuf[x];
			y++;
			x++;
		}
		if (dbuf[y - 1] == ' ')
			return 0;
	}
	if (dbuf[strlen(dbuf) - 1] == ' ')
		dbuf[strlen(dbuf) - 1] = '\0';

	y = 1;
	for (x = 0; x < strlen(dbuf); x++)
		if (dbuf[x] == ' ')
			y++;

	return y;
}

int
get_stdin(char *args[])
{
	char buf[2], sbuf[128], dbuf[128], *arg1, *arg2;
	int x, y, z, argmax;

	safe_bzero(buf);

	argmax = (IFNAMSIZ + 1 > 19) ? IFNAMSIZ + 1 : 19;
	arg1 = (char *)malloc(argmax);
	arg2 = (char *)malloc(16);
	bzero((char *)arg1, argmax);
	bzero((char *)arg2, 16);
	safe_bzero(sbuf);
	safe_bzero(dbuf);

	while (!sbuf[0]) {
		x = 0;
		y = 0;
		safe_bzero(sbuf);
		do {
			x = read(0, buf, 1);
			if (x == 1)
				sbuf[y] = buf[0];
			y++;
		} while (x > 0 && buf[0] != '\n' && y < (sizeof(sbuf) - 1));
		if (x < 0) {
			free(arg1);
			free(arg2);
			return -1;
		}
		if (!x)
			break;

		while (buf[0] != '\n' && x == 1)
			x = read(0, buf, 1);
		if (x < 0) {
			free(arg1);
			free(arg2);
			return -1;
		}
		if (!x)
			break;
	};
	if (!sbuf[0]) {
		free(arg1);
		free(arg2);
		return -2;
	}

	x = cleanline(sbuf, dbuf);
	if (x < 1) {
		free(arg1);
		free(arg2);
		return x;
	}

	y = 0;
	while (y < strlen(dbuf) && y < argmax && dbuf[y] != ' ') {
		arg1[y] = dbuf[y];
		y++;
	}
	y++;
	z = 0;
	while (y < strlen(dbuf) && z < 15 && dbuf[y] != ' ') {
		arg2[z] = dbuf[y];
		y++;
		z++;
	}

	strncpy(args[0], arg1, 127);
	strncpy(args[1], arg2, 127);

	free(arg1);
	free(arg2);

	return x;
}

struct argbox *
new_arg(struct argbox *abox)
{
	abox->next = (struct argbox *)malloc(sizeof(struct argbox));
	abox = abox->next;
	safe_bzero(abox->str);
	abox->type = 0;
	abox->resolv = 0;
	abox->next = NULL;

	return abox;
}

void
free_boxargs(struct argbox *abox)
{
	struct argbox *old;

	while (abox) {
		old = abox;
		abox = abox->next;
		free(old);
	}
}

/*
 * This function will try to populate an argumentbox.
 * This is slightly difficult due to the numerous different possible
 * input types, ie. v4addr, dotted quad netmask, /netmask, hex netmask,
 * v6addr, interface name etc.
 * This forces to have to try to guess what a user means in some cases.
 * This method can be fairly unforgiving with typos.
 */
struct argbox *
get_boxargs(int argc, char *argv[], int argcount, struct argbox *abox_cur)
{
	char expaddr[128];
	int x, y;
	int error;

	error = 0;

	/*
	 * We use goto's here *gasp*.
	 */
	while (argv[argcount]) {
		safe_bzero(expaddr);

		safe_strncpy(expaddr, argv[argcount]);
		/*
		 * Baaad argument. Error out if this happens.
		 */
		if (strlen(argv[argcount]) > sizeof(expaddr) - 1) {
			printf("-[ERR : INVALID ARG - %s]\n", expaddr);
			error = 1;
			exit(1);
		}

		/*
		 * Is this a v6 address?
		 */
		x = validate_v6addr(expaddr);
		if (x) {
			safe_strncpy(abox_cur->str, expaddr);
			abox_cur->type = AT_V6;
			abox_cur->resolv = 0;
			abox_cur = new_arg(abox_cur);
			goto complete;
		}

		/*
		 * Nope, is it an ipv4 address with a /xx mask?
		 *
		 * NOTE: validate_netmask returns different values if it finds
		 * other types of netmasks to, but we only match on the above
		 * here.
		 */
		x = validate_netmask(expaddr);
		if (x == 2) {
			safe_strncpy(abox_cur->str, expaddr);
			abox_cur->type = AT_V4;
			abox_cur->resolv = 0;
			abox_cur = new_arg(abox_cur);
			goto complete;
		}

		/*
		 * No, so is it a plain ipv4 address?
		 */
		x = validate_v4addr(expaddr);
		if (x) {
			y = 0;
			/*
			 * It is, does that mean the next argument is a
			 * netmask?
			 */
			if (argcount + 1 < argc)
				y = validate_netmask(argv[argcount + 1]);
			/*
			 * 1 == 'normal' netmask
			 * 3 == hex netmask
			 */
			if (y == 1 || y == 3) {
				snprintf(abox_cur->str, 34, "%s %s", expaddr,
				    argv[argcount + 1]);
				argcount++;
			} else
				snprintf(abox_cur->str, 18, "%s", expaddr);
			abox_cur->type = AT_V4;
			abox_cur->resolv = 0;
			abox_cur = new_arg(abox_cur);
			goto complete;
		}

		y = 0;
		if (argcount + 1 < argc)
			y = validate_netmask(argv[argcount + 1]);
		if (y == 1 || y == 3) {
			safe_snprintf(abox_cur->str, "%s %s", expaddr,
			    argv[argcount + 1]);
			argcount++;
		} else
			safe_strncpy(abox_cur->str, expaddr);
		abox_cur->type = AT_UNKWN;
		abox_cur->resolv = 1;
		abox_cur = new_arg(abox_cur);

	complete:
		safe_bzero(expaddr);
		argcount++;
	}

	if (error)
		printf("\n");

	return abox_cur;
}

void
show_abox(struct argbox *a)
{
	while (a) {
		printf("%s - %d - %d\n", a->str, a->type, a->resolv);
		a = a->next;
	}
}

struct if_info *
parse_abox(struct argbox *abox, struct if_info *if_start)
{
	struct if_info *ifarg_start, *ifarg_cur, *ifarg_old;
	struct if_info *if_cur;
	struct dnsresp *d_resp_start, *d_resp_cur;
	char *tmpstr;
	int x, if_found;

	ifarg_old = ifarg_cur = ifarg_start = (struct if_info *)malloc(
	    sizeof(struct if_info));
	ifarg_cur->next = NULL;
	bzero((char *)ifarg_cur->name, IFNAMSIZ);
	safe_bzero(ifarg_cur->p_v4addr);
	safe_bzero(ifarg_cur->p_v4nmask);

	while (abox) {
		if (abox->type == AT_V4 && !abox->resolv) {
			tmpstr = strstr(abox->str, " ");
			if (tmpstr != NULL && (strlen(tmpstr) > 0)) {
				tmpstr++;
				x = 0;
				while (x < 15 && tmpstr[x] != ' ' &&
				    x < strlen(tmpstr)) {
					ifarg_cur->p_v4nmask[x] = tmpstr[x];
					x++;
				}
			}

			x = 0;
			while (x < 18 && abox->str[x] != ' ') {
				ifarg_cur->p_v4addr[x] = abox->str[x];
				x++;
			}
			ifarg_cur->type = IFT_V4;
			safe_strncpy(ifarg_cur->cmdstr, abox->str);
		}

		if (abox->type == AT_V4 && abox->resolv) {
			d_resp_start = d_resp_cur = (struct dnsresp *)malloc(
			    sizeof(struct dnsresp));
			d_resp_start->next = NULL;
			safe_bzero(d_resp_start->str);
			d_resp_start->type = 0;
			tmpstr = resolve_addr(abox->str, PF_INET, d_resp_cur);
			if (tmpstr) {
				d_resp_cur = d_resp_start;
				while (d_resp_cur) {
					safe_strncpy(ifarg_cur->cmdstr,
					    abox->str);
					tmpstr = strstr(d_resp_cur->str, " ");
					if (tmpstr != NULL &&
					    (strlen(tmpstr) > 0)) {
						tmpstr++;
						x = 0;
						while (x < 15 &&
						    tmpstr[x] != ' ' &&
						    x < strlen(tmpstr)) {
							ifarg_cur
							    ->p_v4nmask[x] =
							    tmpstr[x];
							x++;
						}
					}

					x = 0;
					while (x < 18 &&
					    d_resp_cur->str[x] != ' ') {
						ifarg_cur->p_v4addr[x] =
						    d_resp_cur->str[x];
						x++;
					}
					ifarg_cur->type = IFT_V4;
					if (d_resp_cur->next)
						ifarg_cur = new_if(ifarg_cur);
					d_resp_cur = d_resp_cur->next;
				}
			} else {
				safe_strncpy(ifarg_cur->p_v4addr, abox->str);
				safe_strncpy(ifarg_cur->cmdstr, abox->str);
				ifarg_cur->type = IFT_V4;
			}

			free_dnsresp(d_resp_start);
		}

		if (abox->type == AT_V6 && !abox->resolv) {
			safe_strncpy(ifarg_cur->p_v6addr, abox->str);
			safe_strncpy(ifarg_cur->cmdstr, abox->str);

			mk_ipv6addr(&ifarg_cur->v6ad, ifarg_cur->p_v6addr);
			ifarg_cur->type = IFT_V6;
		}

		if (abox->type == AT_V6 && abox->resolv) {
			d_resp_start = d_resp_cur = (struct dnsresp *)malloc(
			    sizeof(struct dnsresp));
			d_resp_start->next = NULL;
			safe_bzero(d_resp_start->str);
			d_resp_start->type = 0;
			tmpstr = resolve_addr(abox->str, PF_INET6, d_resp_cur);
			if (tmpstr) {
				d_resp_cur = d_resp_start;
				while (d_resp_cur) {
					safe_strncpy(ifarg_cur->cmdstr,
					    abox->str);
					safe_strncpy(ifarg_cur->p_v6addr,
					    d_resp_cur->str);
					ifarg_cur->type = IFT_V6;

					mk_ipv6addr(&ifarg_cur->v6ad,
					    ifarg_cur->p_v6addr);

					if (d_resp_cur->next)
						ifarg_cur = new_if(ifarg_cur);
					d_resp_cur = d_resp_cur->next;
				}
			} else {
				safe_strncpy(ifarg_cur->cmdstr, abox->str);
				safe_strncpy(ifarg_cur->p_v6addr, abox->str);
				ifarg_cur->type = IFT_V6;

				mk_ipv6addr(&ifarg_cur->v6ad,
				    ifarg_cur->p_v6addr);
			}

			free_dnsresp(d_resp_start);
		}

		if (abox->type == AT_INT) {
			if_cur = if_start;
			if_found = 0;
			while (if_cur) {
				if (!strcmp(abox->str, if_cur->name)) {
					if (if_found) {
						ifarg_old = ifarg_cur;
						ifarg_cur = new_if(ifarg_cur);
					}
					memcpy((struct if_info *)ifarg_cur,
					    (struct if_info *)if_cur,
					    sizeof(struct if_info));
					ifarg_cur->type = IFT_INTV4;
					safe_strncpy(ifarg_cur->cmdstr,
					    abox->str);
					if_found = 1;
				}
				if_cur = if_cur->next;
			}
			if (!if_found) {
				//				strncpy
				//(ifarg_cur->name, abox->str, IFNAMSIZ);
				//				safe_strncpy
				//(ifarg_cur->cmdstr, abox->str);
				//				safe_snprintf(ifarg_cur->errorstr,
				//"Unable to retrieve interface information");
				//				ifarg_cur->type =
				//IFT_INTV4;
				exit(1);
			}
		}

		if (abox->type == AT_UNKWN && !abox->resolv) {
			if_cur = if_start;
			if_found = 0;
			while (if_cur) {
				if (!strcmp(abox->str, if_cur->name)) {
					if (if_found) {
						ifarg_old = ifarg_cur;
						ifarg_cur = new_if(ifarg_cur);
					}
					memcpy((struct if_info *)ifarg_cur,
					    (struct if_info *)if_cur,
					    sizeof(struct if_info));
					ifarg_cur->type = IFT_INTV4;
					safe_strncpy(ifarg_cur->cmdstr,
					    abox->str);
					if_found = 1;
				}
				if_cur = if_cur->next;
			}
			if (!if_found) {
				//				strncpy
				//(ifarg_cur->name, abox->str, IFNAMSIZ);
				//				safe_strncpy
				//(ifarg_cur->cmdstr, abox->str);
				//				safe_snprintf(ifarg_cur->errorstr,
				//"Unable to retrieve interface information");
				//				ifarg_cur->type =
				//IFT_INTV4;
				exit(1);
			}
		}

		if (abox->type == AT_UNKWN && abox->resolv) {
			d_resp_start = d_resp_cur = (struct dnsresp *)malloc(
			    sizeof(struct dnsresp));
			d_resp_start->next = NULL;
			safe_bzero(d_resp_start->str);
			d_resp_start->type = 0;
			tmpstr = resolve_addr(abox->str, PF_UNSPEC, d_resp_cur);
			if (tmpstr) {
				d_resp_cur = d_resp_start;
				while (d_resp_cur) {
					safe_strncpy(ifarg_cur->cmdstr,
					    abox->str);
					if (d_resp_cur->type == AF_INET6) {
						safe_strncpy(
						    ifarg_cur->p_v6addr,
						    d_resp_cur->str);
						ifarg_cur->type = IFT_V6;

						mk_ipv6addr(&ifarg_cur->v6ad,
						    ifarg_cur->p_v6addr);
					}
					if (d_resp_cur->type == AF_INET) {
						tmpstr = strstr(d_resp_cur->str,
						    " ");
						if (tmpstr != NULL &&
						    (strlen(tmpstr) > 0)) {
							tmpstr++;
							x = 0;
							while (x < 15 &&
							    tmpstr[x] != ' ' &&
							    x < strlen(
								    tmpstr)) {
								ifarg_cur
								    ->p_v4nmask
									[x] =
								    tmpstr[x];
								x++;
							}
						}

						x = 0;
						while (x < 18 &&
						    d_resp_cur->str[x] != ' ') {
							ifarg_cur->p_v4addr[x] =
							    d_resp_cur->str[x];
							x++;
						}
						ifarg_cur->type = IFT_V4;
					}
					if (d_resp_cur->next)
						ifarg_cur = new_if(ifarg_cur);
					d_resp_cur = d_resp_cur->next;
				}
				free_dnsresp(d_resp_start);
			} else {
				if_cur = if_start;
				if_found = 0;
				while (if_cur) {
					if (!strcmp(abox->str, if_cur->name)) {
						if (if_found) {
							ifarg_old = ifarg_cur;
							ifarg_cur = new_if(
							    ifarg_cur);
						}
						memcpy((struct if_info *)
							   ifarg_cur,
						    (struct if_info *)if_cur,
						    sizeof(struct if_info));
						ifarg_cur->type = IFT_INTV4;
						safe_strncpy(ifarg_cur->cmdstr,
						    abox->str);
						if_found = 1;
					}
					if_cur = if_cur->next;
				}
				if (!if_found) {
					safe_strncpy(ifarg_cur->cmdstr,
					    abox->str);
					safe_snprintf(ifarg_cur->errorstr,
					    "Unparsable argument.");
					ifarg_cur->type = IFT_UNKWN;
				}
			}
		}

		abox = abox->next;
		ifarg_old = ifarg_cur;
		ifarg_cur = new_if(ifarg_cur);
	}

	ifarg_old->next = NULL;
	free(ifarg_cur);
	ifarg_cur = NULL;

	if (ifarg_start == ifarg_cur) {
		free(ifarg_start);
		return NULL;
	}

	return ifarg_start;
}

int
main(int argc, char *argv[])
{
	int x, y, z, m, v4args, v6args, iffound, argcount, first_err;
	struct if_info *if_start, *if_cur;
	struct if_info *ifarg_start, *ifarg_cur, *ifarg_old;
	int ch, parse_stdin, index;
	struct misc_args m_argv4, m_argv6;
	int split_errv4, split_errv6;
	char expaddr[128];
	char oldcmdstr[128];
	struct argbox *abox_start, *abox_cur, *abox_tmp;
	char *stdinarg[3];
#ifdef HAVE_GETOPT_LONG
	static struct option l_o[] = { { "all", no_argument, 0, 'a' },
		{ "cidr-bitmap", no_argument, 0, 'b' },
		{ "classful-addr", no_argument, 0, 'c' },
		{ "help", no_argument, 0, 'h' },
		{ "cidr-addr", no_argument, 0, 'i' },
		{ "subnets", required_argument, 0, 'n' },
		{ "v4split", required_argument, 0, 's' },
		{ "v6-standard", no_argument, 0, 't' },
		{ "version", no_argument, 0, 'v' },
		{ "classful-bitmap", no_argument, 0, 'x' },
		{ "addr-ipv4", required_argument, 0, '4' },
		{ "addr-ipv6", required_argument, 0, '6' },
		{ "addr-int", required_argument, 0, 'I' },
		{ "v4inv6", no_argument, 0, 'e' },
		{ "v6split", required_argument, 0, 'S' },
		{ "v6rev", no_argument, 0, 'r' },
		{ "split-verbose", no_argument, 0, 'u' },
		{ "resolve", no_argument, 0, 'd' },
		{ "wildcard", no_argument, 0, 'w' }, { 0, 0, 0, 0 } };
#endif
	if (argc < 2) {
		print_short_help();
		return 0;
	}

	parse_stdin = 0;
	v4args = 0;
	v6args = 0;
	m_argv4.splitmask = 0;
	m_argv4.numnets = 0;
	m_argv6.splitmask = 0;
	m_argv6.numnets = 0;
	split_errv4 = 0;
	split_errv6 = 0;
	first_err = 1;
	ifarg_start = NULL;
	ifarg_old = NULL;
	resolve = 0;

	/*
	 * abox == argument box == a box that holds (commandline) arguments :)
	 * This is the structure we use to store all user input parsed into
	 * (hopefully) managable chunks.
	 * This excludes most of the -[a-z] flags, they're generally handled by
	 * v[4,6]args.
	 */
	abox_start = abox_cur = (struct argbox *)malloc(sizeof(struct argbox));
	safe_bzero(abox_cur->str);
	abox_cur->type = 0;
	abox_cur->resolv = 0;
	abox_cur->next = NULL;

	/*
	 * v[4,6]args holds flags based on commandline arguments for what we
	 * want to output.
	 */
#ifdef HAVE_GETOPT_LONG
	while ((ch = getopt_long(argc, argv, "abcdehHiI:n:rs:S:tuvVwx4:6:", l_o,
		    NULL)) != -1) {
#else
	while ((ch = getopt(argc, argv, "abcdehHiI:n:rs:S:tuvVwx4:6:")) != -1) {
#endif
		switch (ch) {
		case 'a':
			v4args = v4args | CF_INFO | CF_BITMAP | CIDR_INFO |
			    CIDR_BITMAP | NET_INFO;
			v6args = v6args | V6_INFO | V4INV6 | V6REV;
			break;
		case 'b':
			v4args = v4args | CIDR_BITMAP;
			break;
		case 'c':
			v4args = v4args | CF_INFO;
			break;
		case 'd':
			resolve = 1;
#if (!defined(HAVE_GETHOSTBYNAME2) && !defined(HAVE_GETADDRINFO)) || \
    !defined(HAVE_INET_NTOP)
			printf(
			    "-[INFO : IPv6 address resolution will fail due to lack of OS support]\n");
#endif
			break;
		case 'e':
			v6args = v6args | V4INV6;
			break;
		case 'h':
		case 'H':
			print_help();
			return 0;
		case 'i':
			v4args = v4args | CIDR_INFO;
			break;
		case 'n':
			v4args = v4args | NET_INFO;
			m_argv4.numnets = atoi(optarg);
			break;
		case 'r':
			v6args = v6args | V6REV;
			break;
		case 's':
			y = getsplitnumv4(optarg, &m_argv4.splitmask);
			if (!y) {
				v4args = v4args | V4SPLIT;
			} else {
				printf(
				    "-[ERR : Invalid IPv4 splitmask, unable to split]\n");
				split_errv4 = 1;
			}
			break;
		case 'S':
			y = getsplitnumv6(optarg, &m_argv6.v6splitmask,
			    &m_argv6.v6splitnum);
			if (!y) {
				v6args = v6args | V6SPLIT;
			} else {
				printf(
				    "-[ERR : Invalid IPv6 splitmask, unable to split]\n");
				split_errv6 = 1;
			}
			break;
		case 't':
			v6args = v6args | V6_INFO;
			break;
		case 'u':
			v4args = v4args | V4VERBSPLIT;
			v6args = v6args | V6VERBSPLIT;
			break;
		case 'v':
		case 'V':
			print_version();
			return 0;
		case 'w':
			v4args = v4args | C_WILDCARD;
			break;
		case 'x':
			v4args = v4args | CF_BITMAP;
			break;
		case '?':
			printf("Try '%s -h' for more information.\n", NAME);
			return 0;
		case '4':
			safe_strncpy(abox_cur->str, optarg);
			abox_cur->type = AT_V4;
			abox_cur->resolv = 1;
			if (validate_netmask(optarg) == 2)
				abox_cur->resolv = 0;
			if (validate_v4addr(optarg) == 1)
				abox_cur->resolv = 0;
			abox_cur = new_arg(abox_cur);

			break;
		case '6':
			safe_strncpy(abox_cur->str, optarg);
			abox_cur->type = AT_V6;
			abox_cur->resolv = 1;
			if (validate_v6addr(expaddr) == 1)
				abox_cur->resolv = 0;
			abox_cur = new_arg(abox_cur);

			break;
		case 'I':
			safe_strncpy(abox_cur->str, optarg);
			abox_cur->type = AT_INT;
			abox_cur->resolv = 0;
			abox_cur = new_arg(abox_cur);

			break;
		default:
			print_short_help();
			return 0;
		}
	}

	if (!v4args && !v6args && (split_errv4 || split_errv6)) {
		printf("-[ERR : No valid commands recieved]\n");
		free_boxargs(abox_start);
		return -1;
	}
	if (split_errv4 || split_errv6)
		printf("\n");

	argcount = optind;
	/*
	 * Our default v4 and v6 options, hopefully what's mostly used.
	 */
	if (!v4args)
		v4args = CIDR_INFO;
	if (!v6args)
		v6args = V6_INFO;
	if (m_argv4.numnets < 1)
		m_argv4.numnets = -1;

	if (argv[argcount]) {
		if (argv[argcount][0] == '-' && argv[argcount][1] == '\0')
			parse_stdin = 1;
	} else {
		if (abox_start->str[0] == '\0') {
			print_short_help();
			free_boxargs(abox_start);
			return 0;
		}
	}

	/*
	 * Populate our argumentbox.
	 * (Ie., see what's on the commandline).
	 */
	if (!parse_stdin && argv[argcount]) {
		abox_cur = get_boxargs(argc, argv, argcount, abox_cur);

		abox_tmp = abox_start;
		while (abox_tmp->next != abox_cur) {
			abox_tmp = abox_tmp->next;
		}
		abox_tmp->next = NULL;
		free(abox_cur);
		abox_cur = NULL;
	}

	abox_tmp = abox_start;
	if (!resolve) {
		while (abox_tmp) {
			abox_tmp->resolv = 0;
			abox_tmp = abox_tmp->next;
		}
	}

#if 0
	show_abox (abox_start);
#endif

	/*
	 * This will try to gather information about the network interfaces
	 * present on the local machine.
	 */
	if_start = NULL;
	if_cur = NULL;
	if (!(if_cur = if_start = get_if_ext())) {
		//		printf
		//		    ("-[INFO : Unable to retrieve interface
		//information]\n"); 		printf
		//		    ("-[INFO : Will only parse none interface
		//arguments]\n\n");
	}

	if (!parse_stdin)
		ifarg_cur = ifarg_start = parse_abox(abox_start, if_start);

	if (!ifarg_start && !parse_stdin) {
		//		printf ("-[FATAL : No valid commandline arguments
		//found]\n\n");
		return -1;
	}

	iffound = 0;
	index = 0;
	ifarg_cur = ifarg_start;
	safe_bzero(oldcmdstr);
	while (ifarg_cur && !parse_stdin) {
		if (strlen(ifarg_cur->cmdstr) > 0) {
			if (!strcmp(ifarg_cur->cmdstr, oldcmdstr))
				index++;
			else
				index = 0;
		} else {
			index = 0;
		}
		iffound += out_cmdline(ifarg_cur, v4args, m_argv4, v6args,
		    m_argv6, 0, index);
		safe_strncpy(oldcmdstr, ifarg_cur->cmdstr);
		ifarg_cur = ifarg_cur->next;
	}

	z = 0;
	y = 1;
	while (parse_stdin && y > -1) {
		stdinarg[0] = (char *)malloc(128);
		stdinarg[1] = (char *)malloc(128);
		stdinarg[2] = NULL;
		for (x = 0; x < 2; x++)
			bzero((char *)stdinarg[x], 128);
		y = get_stdin(stdinarg);
		if (y > 0) {
			m = 2;
			if (stdinarg[1][0] == '\0') {
				free(stdinarg[1]);
				stdinarg[1] = NULL;
				m = 1;
			}
			abox_cur = get_boxargs(m, stdinarg, 0, abox_cur);
			abox_tmp = abox_start;
			while (abox_tmp->next != abox_cur &&
			    abox_tmp != abox_cur) {
				abox_tmp = abox_tmp->next;
			}
			abox_tmp->next = NULL;
			free(abox_cur);
			abox_cur = NULL;

			abox_tmp = abox_start;
			if (!resolve) {
				while (abox_tmp) {
					abox_tmp->resolv = 0;
					abox_tmp = abox_tmp->next;
				}
			}

			ifarg_cur = ifarg_start = parse_abox(abox_start,
			    if_start);

			if (ifarg_start) {
				iffound = 0;
				index = 0;
				ifarg_cur = ifarg_start;
				safe_bzero(oldcmdstr);
				while (ifarg_cur) {
					if (strlen(ifarg_cur->cmdstr) > 0) {
						if (!strcmp(ifarg_cur->cmdstr,
							oldcmdstr))
							index++;
						else
							index = 0;
					} else {
						index = 0;
					}
					iffound += out_cmdline(ifarg_cur,
					    v4args, m_argv4, v6args, m_argv6, 0,
					    index);
					safe_strncpy(oldcmdstr,
					    ifarg_cur->cmdstr);
					ifarg_cur = ifarg_cur->next;
				}
			}

			z = 1;
			free_if(ifarg_start);
			free_boxargs(abox_start);
			abox_start = abox_cur = (struct argbox *)malloc(
			    sizeof(struct argbox));
			safe_bzero(abox_cur->str);
			abox_cur->type = 0;
			abox_cur->resolv = 0;
			abox_cur->next = NULL;
		}
		for (x = 0; x < 2; x++) {
			if (stdinarg[x]) {
				free(stdinarg[x]);
				stdinarg[x] = NULL;
			}
		}
		if (y == -1)
			printf("\n-[ERR : Problem parsing stdin]\n\n");
	}
	if (parse_stdin) {
		free(stdinarg[0]);
		free(stdinarg[1]);
	}

	if (!z && parse_stdin)
		printf("-[FATAL : No arguments found on stdin]\n\n");

	if (!parse_stdin)
		free_if(ifarg_start);
	free_if(if_start);

	return iffound;
}
