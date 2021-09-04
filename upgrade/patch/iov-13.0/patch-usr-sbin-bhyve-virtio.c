--- virtio.c.orig	2021-07-11 08:17:17.632398000 +0000
+++ virtio.c	2021-09-01 13:58:48.855298000 +0000
@@ -227,7 +227,7 @@
 	else
 		reqp->writable++;
 }
-#define	VQ_MAX_DESCRIPTORS	512	/* see below */
+#define	VQ_MAX_DESCRIPTORS	1024	/* see below */
 
 /*
  * Examine the chain of descriptors starting at the "next one" to
