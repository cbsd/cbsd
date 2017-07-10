--- priv.h-orig	2017-03-28 19:54:43.656502000 +0300
+++ priv.h	2017-07-04 01:12:48.018674000 +0300
@@ -504,11 +504,12 @@
  */
 #define	PRIV_KMEM_READ		680	/* Open mem/kmem for reading. */
 #define	PRIV_KMEM_WRITE		681	/* Open mem/kmem for writing. */
+#define	PRIV_DRI_DRIVER		682	/* Open dri for writing. */
 
 /*
  * Track end of privilege list.
  */
-#define	_PRIV_HIGHEST		682
+#define	_PRIV_HIGHEST		683
 
 /*
  * Validate that a named privilege is known by the privilege system.  Invalid
