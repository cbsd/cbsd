--- priv.h-orig	2015-08-09 13:05:35.776763000 +0300
+++ priv.h	2015-08-09 13:06:37.812274000 +0300
@@ -502,11 +502,12 @@
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
