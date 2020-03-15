/*
 * sipcalc, sub-o.h
 *
 * $Id: sub-o.h,v 1.11 2003/03/19 12:28:15 simius Exp $
 *
 * -
 * Copyright (c) 2003 Simon Ekstrand
 * All rights reserved.
 */

#ifndef SUB_O_H
#define SUB_O_H

/*
 * prototypes
 */
void show_c_wildcard_info_v4 (struct if_info *ifi);
void show_split_networks_v4 (struct if_info *ifi, u_int32_t splitmask, int v4args, struct misc_args m_argv4);
int show_networks_v4 (struct if_info *ifi, int count);
void print_cf_info_v4 (struct if_info *ifi);
void print_cf_bitmap_v4 (struct if_info *ifi);
void print_cidr_info_v4 (struct if_info *ifi);
void print_cidr_bitmap_v4 (struct if_info *ifi);
void print_comp_v6 (struct sip_in6_addr addr);
void print_exp_v4inv6 (struct sip_in6_addr addr);
void print_comp_v4inv6 (struct sip_in6_addr addr);
void print_exp_v6 (struct sip_in6_addr addr);
void print_mixed_v6 (struct sip_in6_addr addr);
void print_rev_v6 (struct if_info *ifi);
void print_v6 (struct if_info *ifi);
void print_v4inv6 (struct if_info *ifi);
int v6plus (struct sip_in6_addr *a, struct sip_in6_addr *b);
void show_split_networks_v6 (struct if_info *ifi, struct sip_in6_addr splitmask, int v6args, struct misc_args m_argv6);
void print_short_help ();
void print_help ();
void print_version ();

#endif				/* SUB_O_H */
