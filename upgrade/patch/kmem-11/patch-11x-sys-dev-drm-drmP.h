--- drmP.h.bak	2014-03-12 02:31:06.000000000 +0400
+++ drmP.h	2014-03-12 02:31:39.000000000 +0400
@@ -228,7 +228,7 @@
 #define PAGE_ALIGN(addr) round_page(addr)
 /* DRM_SUSER returns true if the user is superuser */
 #if __FreeBSD_version >= 700000
-#define DRM_SUSER(p)		(priv_check(p, PRIV_DRIVER) == 0)
+#define DRM_SUSER(p)		(priv_check(p, PRIV_DRI_DRIVER) == 0)
 #else
 #define DRM_SUSER(p)		(suser(p) == 0)
 #endif
