--- priv.h-orig	2015-08-09 15:05:48.885396000 +0300
+++ priv.h	2015-08-09 15:06:35.939834000 +0300
@@ -501,11 +501,12 @@
  */
 #define	PRIV_KMEM_READ		680	/* Open mem/kmem for reading. */
 #define	PRIV_KMEM_WRITE		681	/* Open mem/kmem for writing. */
+#define	PRIV_DRI_DRIVER		682
 
 /*
  * Track end of privilege list.
  */
-#define	_PRIV_HIGHEST		682
+#define	_PRIV_HIGHEST		683
 
 /*
  * Validate that a named privilege is known by the privilege system.  Invalid
