-- -drmP.h - orig 2017 - 04 - 22 11 : 28 : 12.530057000 + 0300 ++ +drmP.h 2017 -
    07 - 04 00 : 57 : 37.513480000 + 0300 @ @-220,
    7 + 220,
    7 @ @

#define PAGE_ALIGN(addr) round_page(addr)
    /* DRM_SUSER returns true if the user is superuser */
    -#define DRM_SUSER(p)(priv_check(p, PRIV_DRIVER) == 0) +
    #define DRM_SUSER(p)(priv_check(p, PRIV_DRI_DRIVER) == 0)
#define DRM_AGP_FIND_DEVICE() agp_find_device()
#define DRM_MTRR_WC MDF_WRITECOMBINE
#define jiffies ticks
