--- virtio.h-orig	2018-12-14 16:47:26.349634000 +0300
+++ virtio.h	2018-12-14 17:19:17.916836000 +0300
@@ -214,6 +214,7 @@
 #define	VIRTIO_DEV_CONSOLE	0x1003
 #define	VIRTIO_DEV_RANDOM	0x1005
 #define	VIRTIO_DEV_SCSI		0x1008
+#define	VIRTIO_DEV_9P		0x1009
 
 /*
  * PCI config space constants.
