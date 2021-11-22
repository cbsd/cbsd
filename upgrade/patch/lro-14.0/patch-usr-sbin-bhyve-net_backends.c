--- net_backends.c.orig	2021-11-22 11:04:15.641182000 +0300
+++ net_backends.c	2021-11-22 11:19:41.959818000 +0300
@@ -670,8 +670,7 @@
 netmap_get_cap(struct net_backend *be)
 {
 
-	return (netmap_has_vnet_hdr_len(be, VNET_HDR_LEN) ?
-	    NETMAP_FEATURES : 0);
+	return 0;
 }
 
 static int
