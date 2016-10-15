--- drm_os_freebsd.h-orig	2016-08-05 01:16:21.615319000 +0300
+++ drm_os_freebsd.h	2016-10-14 22:47:47.288952000 +0300
@@ -112,7 +112,7 @@
 #define	HZ			hz
 #define	DRM_HZ			hz
 #define	DRM_CURRENTPID		curthread->td_proc->p_pid
-#define	DRM_SUSER(p)		(priv_check(p, PRIV_DRIVER) == 0)
+#define	DRM_SUSER(p)		(priv_check(p, PRIV_DRI_DRIVER) == 0)
 #define	udelay(usecs)		DELAY(usecs)
 #define	mdelay(msecs)		do { int loops = (msecs);		\
 				  while (loops--) DELAY(1000);		\
