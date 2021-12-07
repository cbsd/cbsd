--- pci_fbuf.c.orig	2021-11-22 11:29:36.212615000 +0300
+++ pci_fbuf.c	2021-12-07 10:43:12.005093000 +0300
@@ -390,7 +390,7 @@
 	/* initialize config space */
 	pci_set_cfgdata16(pi, PCIR_DEVICE, 0x40FB);
 	pci_set_cfgdata16(pi, PCIR_VENDOR, 0xFB5D);
-	pci_set_cfgdata8(pi, PCIR_CLASS, PCIC_DISPLAY);
+	pci_set_cfgdata8(pi, PCIR_CLASS, PCIS_DISPLAY_OTHER);
 	pci_set_cfgdata8(pi, PCIR_SUBCLASS, PCIS_DISPLAY_VGA);
 
 	sc->fb_base = vm_create_devmem(
