--- block_if.h.orig	2021-07-11 08:17:17.620403000 +0000
+++ block_if.h	2021-09-01 13:57:29.270808000 +0000
@@ -50,7 +50,7 @@
  * a single request.  BLOCKIF_RING_MAX is the maxmimum number of
  * pending requests that can be queued.
  */
-#define	BLOCKIF_IOV_MAX		128	/* not practical to be IOV_MAX */
+#define	BLOCKIF_IOV_MAX		512	/* not practical to be IOV_MAX */
 #define	BLOCKIF_RING_MAX	128
 
 struct blockif_req {
