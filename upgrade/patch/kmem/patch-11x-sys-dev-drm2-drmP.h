--- drmP.h.bak	2014-03-12 02:33:41.000000000 +0400
+++ drmP.h	2014-03-12 02:34:16.000000000 +0400
@@ -252,7 +252,7 @@
 
 #define PAGE_ALIGN(addr) round_page(addr)
 /* DRM_SUSER returns true if the user is superuser */
-#define DRM_SUSER(p)		(priv_check(p, PRIV_DRIVER) == 0)
+#define DRM_SUSER(p)		(priv_check(p, PRIV_DRI_DRIVER) == 0)
 #define DRM_AGP_FIND_DEVICE()	agp_find_device()
 #define DRM_MTRR_WC		MDF_WRITECOMBINE
 #define jiffies			ticks
