-- -net_backends.c.orig 2020 - 11 - 22 13 : 47 : 47.024058000 +
    0300 ++ +net_backends.c 2020 - 11 - 22 13 : 48 : 03.696780000 +
    0300 @ @-649,
    9 + 649,
    7 @ @ static uint64_t
    netmap_get_cap(struct net_backend *be)
{
	- -return (
	    netmap_has_vnet_hdr_len(be, VNET_HDR_LEN) ? -NETMAP_FEATURES : 0);
	+return 0;
}

static int
