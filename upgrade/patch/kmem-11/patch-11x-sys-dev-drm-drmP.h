--- drmP.h-orig	2015-08-09 16:29:41.059949000 +0300
+++ drmP.h	2015-08-09 16:30:15.362971000 +0300
@@ -221,7 +221,7 @@
 
 #define PAGE_ALIGN(addr) round_page(addr)
 /* DRM_SUSER returns true if the user is superuser */
-#define DRM_SUSER(p)		(priv_check(p, PRIV_DRIVER) == 0)
+#define DRM_SUSER(p)		(priv_check(p, PRIV_DRI_DRIVER) == 0)
 #define DRM_AGP_FIND_DEVICE()	agp_find_device()
 #define DRM_MTRR_WC		MDF_WRITECOMBINE
 #define jiffies			ticks
