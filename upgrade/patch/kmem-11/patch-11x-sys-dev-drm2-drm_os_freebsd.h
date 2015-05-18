--- drm_os_freebsd.h.bak	2015-03-20 05:09:41.000000000 +0300
+++ drm_os_freebsd.h	2015-03-20 05:11:11.000000000 +0300
@@ -85,7 +85,7 @@
 #define	HZ			hz
 #define	DRM_HZ			hz
 #define	DRM_CURRENTPID		curthread->td_proc->p_pid
-#define	DRM_SUSER(p)		(priv_check(p, PRIV_DRIVER) == 0)
+#define	DRM_SUSER(p)		(priv_check(p, PRIV_DRI_DRIVER) == 0)
 #define	udelay(usecs)		DELAY(usecs)
 #define	mdelay(msecs)		do { int loops = (msecs);		\
 				  while (loops--) DELAY(1000);		\
