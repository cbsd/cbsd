--- net_backends.c.orig	2023-02-24 11:42:56.520925000 +0300
+++ net_backends.c	2023-02-24 11:45:43.243754000 +0300
@@ -671,9 +671,8 @@
 static uint64_t
 netmap_get_cap(struct net_backend *be)
 {
-
-	return (netmap_has_vnet_hdr_len(be, VNET_HDR_LEN) ?
-	    NETMAP_FEATURES : 0);
+	netmap_has_vnet_hdr_len(be, VNET_HDR_LEN);
+	return 0;
 }
 
 static int
