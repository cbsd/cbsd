--- drmP.h-orig	2015-08-09 15:03:18.529851000 +0300
+++ drmP.h	2015-08-09 15:03:57.391919000 +0300
@@ -228,7 +228,7 @@
 #define PAGE_ALIGN(addr) round_page(addr)
 /* DRM_SUSER returns true if the user is superuser */
 #if __FreeBSD_version >= 700000
-#define DRM_SUSER(p)		(priv_check(p, PRIV_DRIVER) == 0)
+#define DRM_SUSER(p)		(priv_check(p, PRIV_DRI_DRIVER) == 0)
 #else
 #define DRM_SUSER(p)		(suser(p) == 0)
 #endif
