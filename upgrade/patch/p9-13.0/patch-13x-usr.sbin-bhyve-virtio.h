--- virtio.h.orig	2019-02-21 09:13:46.794634000 +0000
+++ virtio.h	2019-02-24 10:44:57.245495000 +0000
@@ -214,6 +214,7 @@
 #define	VIRTIO_DEV_CONSOLE	0x1003
 #define	VIRTIO_DEV_RANDOM	0x1005
 #define	VIRTIO_DEV_SCSI		0x1008
+#define	VIRTIO_DEV_9P		0x1009
 
 /*
  * PCI config space constants.
