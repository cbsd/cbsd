--- usb_emul.c.orig	2025-08-27 22:41:29.291336000 +0300
+++ usb_emul.c	2025-08-27 22:31:08.292758000 +0300
@@ -69,6 +69,7 @@
 	xb->ccs = ccs;
 	xb->processed = 0;
 	xb->bdone = 0;
+	xb->status = USB_NO_DATA;
 	xfer->ndata++;
 	xfer->tail = (xfer->tail + 1) % USB_MAX_XFER_BLOCKS;
 	return (xb);
