--- drm_os_freebsd.h-orig	2015-08-09 15:04:46.535215000 +0300
+++ drm_os_freebsd.h	2015-08-09 15:05:16.919116000 +0300
@@ -85,7 +85,7 @@
 #define	HZ			hz
 #define	DRM_HZ			hz
 #define	DRM_CURRENTPID		curthread->td_proc->p_pid
-#define	DRM_SUSER(p)		(priv_check(p, PRIV_DRIVER) == 0)
+#define	DRM_SUSER(p)		(priv_check(p, PRIV_DRI_DRIVER) == 0)
 #define	udelay(usecs)		DELAY(usecs)
 #define	mdelay(msecs)		do { int loops = (msecs);		\
 				  while (loops--) DELAY(1000);		\
