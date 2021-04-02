--- net_backends.c.orig	2021-04-01 10:37:00.509421000 +0300
+++ net_backends.c	2021-04-02 10:37:27.292961000 +0300
@@ -619,8 +619,7 @@
 netmap_get_cap(struct net_backend *be)
 {
 
-	return (netmap_has_vnet_hdr_len(be, VNET_HDR_LEN) ?
-	    NETMAP_FEATURES : 0);
+	return 0;
 }
 
 static int
