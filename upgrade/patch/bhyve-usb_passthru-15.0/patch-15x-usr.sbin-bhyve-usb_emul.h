--- usb_emul.h.orig	2025-08-27 22:41:29.291406000 +0300
+++ usb_emul.h	2025-08-27 22:31:08.280861000 +0300
@@ -52,7 +52,7 @@
 	int	ue_usbspeed;	/* usb device speed */
 
 	/* instance creation */
-	void	*(*ue_probe)(struct usb_hci *hci, nvlist_t *nvl);
+	void	*(*ue_probe)(struct usb_hci *hci, const nvlist_t *nvl);
 	int	(*ue_init)(void *sc);
 
 	/* handlers */
@@ -76,6 +76,15 @@
 	USBDEV_REMOVE,
 };
 
+/*
+ * USB data status for TRB
+ */
+enum usb_xfer_data_status {
+	USB_NO_DATA,
+	USB_NEXT_DATA,
+	USB_LAST_DATA,
+};
+
 /* usb controller, ie xhci, ehci */
 struct usb_hci {
 	int	(*hci_intr)(struct usb_hci *hci, int epctx);
@@ -87,7 +96,8 @@
 	int	hci_address;
 	int	hci_port;
 	int	hci_speed;
-	int	hci_usbver;
+	int	hci_slot;
+	int	hci_usbver; /* Can be modified by the backend in the probe step */
 };
 
 /*
@@ -105,6 +115,7 @@
 	int	ccs;
 	uint32_t streamid;
 	uint64_t trbnext;		/* next TRB guest address */
+	enum usb_xfer_data_status status;
 };
 
 struct usb_data_xfer {
@@ -113,6 +124,7 @@
 	int	ndata;				/* # of data items */
 	int	head;
 	int	tail;
+	void *tr_softc;
 	pthread_mutex_t mtx;
 };
 
