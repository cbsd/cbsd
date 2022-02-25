-- -drm_os_freebsd.h - orig 2017 - 03 - 28 19 : 54 : 29.557612000 +
    0300 ++ +drm_os_freebsd.h 2017 - 07 - 04 00 : 59 : 00.115952000 +
    0300 @ @-111,
    7 + 111,
    7 @ @
#define HZ hz
#define DRM_HZ hz
#define DRM_CURRENTPID curthread->td_proc->p_pid
    -#define DRM_SUSER(p)(priv_check(p, PRIV_DRIVER) == 0) +
    #define DRM_SUSER(p)(priv_check(p, PRIV_DRI_DRIVER) == 0)
#define udelay(usecs) DELAY(usecs)
#define mdelay(msecs)                \
	do {                         \
		int loops = (msecs); \
		while (loops--)      \
			DELAY(1000);\
