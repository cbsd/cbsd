--- virtio.h-orig	2018-10-24 01:01:05.886139000 +0300
+++ virtio.h	2018-11-30 15:28:36.126011000 +0300
@@ -214,6 +214,7 @@
 #define	VIRTIO_DEV_CONSOLE	0x1003
 #define	VIRTIO_DEV_RANDOM	0x1005
 #define	VIRTIO_DEV_SCSI		0x1008
+#define	VIRTIO_DEV_9P		0x1009
 
 /*
  * PCI config space constants.
