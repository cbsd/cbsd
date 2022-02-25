-- -drm_os_freebsd.h - orig 2017 - 06 - 28 15 : 28 : 39.591673000 +
    0300 ++ +drm_os_freebsd.h 2017 - 07 - 04 15 : 14 : 01.974376000 +
    0300 @ @-40,
    7 + 40,
    7 @ @

#define DRM_HZ hz
#define DRM_CURPROC curthread
    -#define DRM_SUSER(p)(priv_check(p, PRIV_DRIVER) == 0) +
    #define DRM_SUSER(p)(priv_check(p, PRIV_DRI_DRIVER) == 0)
#define DRM_UDELAY(n) udelay(n);

#define DRM_WAIT_ON(ret, queue, timeout, condition)\
