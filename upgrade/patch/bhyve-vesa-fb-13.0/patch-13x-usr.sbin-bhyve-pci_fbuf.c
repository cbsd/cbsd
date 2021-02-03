--- pci_fbuf.c.orig	2021-01-23 00:09:48.409575000 +0300
+++ pci_fbuf.c	2021-01-23 00:09:24.614356000 +0300
@@ -368,7 +368,7 @@
 	/* initialize config space */
 	pci_set_cfgdata16(pi, PCIR_DEVICE, 0x40FB);
 	pci_set_cfgdata16(pi, PCIR_VENDOR, 0xFB5D);
-	pci_set_cfgdata8(pi, PCIR_CLASS, PCIC_DISPLAY);
+	pci_set_cfgdata8(pi, PCIR_CLASS, PCIS_DISPLAY_OTHER);
 	pci_set_cfgdata8(pi, PCIR_SUBCLASS, PCIS_DISPLAY_VGA);
 
 	error = pci_emul_alloc_bar(pi, 0, PCIBAR_MEM32, DMEMSZ);
