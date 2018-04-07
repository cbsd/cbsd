--- vmm_dev.h.orig	2018-03-30 16:11:58.342528000 +0300
+++ vmm_dev.h	2018-03-30 18:19:26.020042000 +0300
@@ -225,6 +225,13 @@
 	uint8_t		value;
 };
 
+struct vm_cpu_topology {
+	uint16_t	sockets;
+	uint16_t	cores;
+	uint16_t	threads;
+	uint16_t	maxcpus;
+};
+
 enum {
 	/* general routines */
 	IOCNUM_ABIVERS = 0,
@@ -283,6 +290,10 @@
 	IOCNUM_GET_X2APIC_STATE = 61,
 	IOCNUM_GET_HPET_CAPABILITIES = 62,
 
+	/* CPU Topology */
+	IOCNUM_SET_TOPOLOGY = 63,
+	IOCNUM_GET_TOPOLOGY = 64,
+
 	/* legacy interrupt injection */
 	IOCNUM_ISA_ASSERT_IRQ = 80,
 	IOCNUM_ISA_DEASSERT_IRQ = 81,
@@ -376,6 +387,10 @@
 	_IOWR('v', IOCNUM_GET_X2APIC_STATE, struct vm_x2apic)
 #define	VM_GET_HPET_CAPABILITIES \
 	_IOR('v', IOCNUM_GET_HPET_CAPABILITIES, struct vm_hpet_cap)
+#define VM_SET_TOPOLOGY \
+	_IOW('v', IOCNUM_SET_TOPOLOGY, struct vm_cpu_topology)
+#define VM_GET_TOPOLOGY \
+	_IOR('v', IOCNUM_GET_TOPOLOGY, struct vm_cpu_topology)
 #define	VM_GET_GPA_PMAP \
 	_IOWR('v', IOCNUM_GET_GPA_PMAP, struct vm_gpa_pte)
 #define	VM_GLA2GPA	\
