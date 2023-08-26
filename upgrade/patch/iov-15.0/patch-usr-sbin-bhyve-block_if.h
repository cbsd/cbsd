--- block_if.h.orig	2022-06-01 12:19:18.214744000 +0300
+++ block_if.h	2022-06-01 12:20:12.163589000 +0300
@@ -50,7 +50,7 @@
  * a single request.  BLOCKIF_RING_MAX is the maxmimum number of
  * pending requests that can be queued.
  */
-#define	BLOCKIF_IOV_MAX		128	/* not practical to be IOV_MAX */
+#define	BLOCKIF_IOV_MAX		512	/* not practical to be IOV_MAX */
 #define	BLOCKIF_RING_MAX	128
 
 struct blockif_req {
