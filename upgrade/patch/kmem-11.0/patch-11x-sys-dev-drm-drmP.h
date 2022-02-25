-- -drmP.h - orig 2016 - 07 - 08 10 : 52 : 12.711596000 + 0300 ++ +drmP.h 2016 -
    10 - 14 22 : 43 : 23.685820000 + 0300 @ @-219,
    7 + 219,
    7 @ @

#define PAGE_ALIGN(addr) round_page(addr)
    /* DRM_SUSER returns true if the user is superuser */
    -#define DRM_SUSER(p)(priv_check(p, PRIV_DRIVER) == 0) +
    #define DRM_SUSER(p)(priv_check(p, PRIV_DRI_DRIVER) == 0)
#define DRM_AGP_FIND_DEVICE() agp_find_device()
#define DRM_MTRR_WC MDF_WRITECOMBINE
#define jiffies ticks
