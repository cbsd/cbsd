--- usb_mouse.c.orig	2025-08-27 22:41:29.291565000 +0300
+++ usb_mouse.c	2025-08-27 22:31:08.293586000 +0300
@@ -295,7 +295,7 @@
 }
 
 static void *
-umouse_probe(struct usb_hci *hci, nvlist_t *nvl __unused)
+umouse_probe(struct usb_hci *hci, const nvlist_t *nvl __unused)
 {
 	struct umouse_softc *sc;
 
