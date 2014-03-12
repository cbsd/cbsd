--- priv.h.bak	2014-03-12 02:43:08.000000000 +0400
+++ priv.h	2014-03-12 02:43:54.000000000 +0400
@@ -499,11 +499,12 @@
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
