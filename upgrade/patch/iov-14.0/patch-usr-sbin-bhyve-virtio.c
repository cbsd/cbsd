--- virtio.c.orig	2021-11-22 11:04:15.645452000 +0300
+++ virtio.c	2021-11-22 11:17:27.592870000 +0300
@@ -227,7 +227,7 @@
 	else
 		reqp->writable++;
 }
-#define	VQ_MAX_DESCRIPTORS	512	/* see below */
+#define	VQ_MAX_DESCRIPTORS	1024	/* see below */
 
 /*
  * Examine the chain of descriptors starting at the "next one" to
