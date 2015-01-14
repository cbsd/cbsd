--- drmP.h.bak	2015-01-13 13:16:06.000000000 +0300
+++ drmP.h	2015-01-13 13:17:54.000000000 +0300
@@ -228,7 +228,7 @@
 #define PAGE_ALIGN(addr) round_page(addr)
 /* DRM_SUSER returns true if the user is superuser */
 #if __FreeBSD_version >= 700000
-#define DRM_SUSER(p)		(priv_check(p, PRIV_DRIVER) == 0)
+#define DRM_SUSER(p)		(priv_check(p, PRIV_DRI_DRIVER) == 0)
 #else
 #define DRM_SUSER(p)		(suser(p) == 0)
 #endif
