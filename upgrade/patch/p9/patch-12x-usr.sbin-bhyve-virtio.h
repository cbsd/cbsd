--- virtio.h-orig	2017-10-07 13:26:01.888283000 +0300
+++ virtio.h	2017-10-07 13:33:30.208065000 +0300
@@ -211,6 +211,7 @@
 #define	VIRTIO_DEV_BLOCK	0x1001
 #define	VIRTIO_DEV_CONSOLE	0x1003
 #define	VIRTIO_DEV_RANDOM	0x1005
+#define	VIRTIO_DEV_9P		0x1009
 
 /*
  * PCI config space constants.
