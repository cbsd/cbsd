--- virtio.c.orig	2022-06-01 12:19:19.036506000 +0300
+++ virtio.c	2022-06-01 12:20:51.277854000 +0300
@@ -227,7 +227,7 @@
 	else
 		reqp->writable++;
 }
-#define	VQ_MAX_DESCRIPTORS	512	/* see below */
+#define	VQ_MAX_DESCRIPTORS	1024	/* see below */
 
 /*
  * Examine the chain of descriptors starting at the "next one" to
