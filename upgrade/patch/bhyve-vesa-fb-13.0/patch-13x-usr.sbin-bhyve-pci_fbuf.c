--- pci_fbuf.c.orig	2019-09-14 18:28:38.419850000 +0300
+++ pci_fbuf.c	2019-09-16 15:06:50.417954000 +0300
@@ -366,7 +366,7 @@
 	/* initialize config space */
 	pci_set_cfgdata16(pi, PCIR_DEVICE, 0x40FB);
 	pci_set_cfgdata16(pi, PCIR_VENDOR, 0xFB5D);
-	pci_set_cfgdata8(pi, PCIR_CLASS, PCIC_DISPLAY);
+	pci_set_cfgdata8(pi, PCIR_CLASS, PCIS_DISPLAY_OTHER);
 	pci_set_cfgdata8(pi, PCIR_SUBCLASS, PCIS_DISPLAY_VGA);
 
 	error = pci_emul_alloc_bar(pi, 0, PCIBAR_MEM32, DMEMSZ);
