--- x86.c.orig	2017-11-27 20:31:07.936996000 +0300
+++ x86.c	2018-03-17 20:05:31.651053000 +0300
@@ -63,12 +63,12 @@
 /*
  * The default CPU topology is a single thread per package.
  */
-static u_int threads_per_core = 1;
-SYSCTL_UINT(_hw_vmm_topology, OID_AUTO, threads_per_core, CTLFLAG_RDTUN,
+u_int threads_per_core = 1;
+SYSCTL_UINT(_hw_vmm_topology, OID_AUTO, threads_per_core, CTLFLAG_RW,
     &threads_per_core, 0, NULL);
 
-static u_int cores_per_package = 1;
-SYSCTL_UINT(_hw_vmm_topology, OID_AUTO, cores_per_package, CTLFLAG_RDTUN,
+u_int cores_per_package = 1;
+SYSCTL_UINT(_hw_vmm_topology, OID_AUTO, cores_per_package, CTLFLAG_RW,
     &cores_per_package, 0, NULL);
 
 static int cpuid_leaf_b = 1;
@@ -95,6 +95,7 @@
 	int error, enable_invpcid, level, width, x2apic_id;
 	unsigned int func, regs[4], logical_cpus;
 	enum x2apic_state x2apic_state;
+	uint16_t cores, maxcpus, sockets, threads;
 
 	VCPU_CTR2(vm, vcpu_id, "cpuid %#x,%#x", *eax, *ecx);
 
@@ -142,11 +143,12 @@
 				 *
 				 * However this matches the logical cpus as
 				 * advertised by leaf 0x1 and will work even
-				 * if the 'threads_per_core' tunable is set
-				 * incorrectly on an AMD host.
+				 * if threads is set incorrectly on an AMD host.
 				 */
-				logical_cpus = threads_per_core *
-				    cores_per_package;
+				vm_get_topology(vm, &sockets, &cores, &threads,
+					&maxcpus);
+				logical_cpus = threads *
+					cores;
 				regs[2] = logical_cpus - 1;
 			}
 			break;
@@ -305,7 +307,10 @@
 			 */
 			regs[3] |= (CPUID_MCA | CPUID_MCE | CPUID_MTRR);
 
-			logical_cpus = threads_per_core * cores_per_package;
+			vm_get_topology(vm, &sockets, &cores, &threads,
+				&maxcpus);
+			logical_cpus = threads * cores;
+
 			regs[1] &= ~CPUID_HTT_CORES;
 			regs[1] |= (logical_cpus & 0xff) << 16;
 			regs[3] |= CPUID_HTT;
@@ -315,8 +320,10 @@
 			cpuid_count(*eax, *ecx, regs);
 
 			if (regs[0] || regs[1] || regs[2] || regs[3]) {
+				vm_get_topology(vm, &sockets, &cores, &threads,
+					&maxcpus);
 				regs[0] &= 0x3ff;
-				regs[0] |= (cores_per_package - 1) << 26;
+				regs[0] |= (cores - 1) << 26;
 				/*
 				 * Cache topology:
 				 * - L1 and L2 are shared only by the logical
@@ -324,10 +331,10 @@
 				 * - L3 and above are shared by all logical
 				 *   processors in the package.
 				 */
-				logical_cpus = threads_per_core;
+				logical_cpus = threads;
 				level = (regs[0] >> 5) & 0x7;
 				if (level >= 3)
-					logical_cpus *= cores_per_package;
+					logical_cpus *= cores;
 				regs[0] |= (logical_cpus - 1) << 14;
 			}
 			break;
@@ -390,15 +397,17 @@
 			 * Processor topology enumeration
 			 */
 			if (*ecx == 0) {
-				logical_cpus = threads_per_core;
+				vm_get_topology(vm, &sockets, &cores, &threads,
+					&maxcpus);
+				logical_cpus = threads;
 				width = log2(logical_cpus);
 				level = CPUID_TYPE_SMT;
 				x2apic_id = vcpu_id;
 			}
 
 			if (*ecx == 1) {
-				logical_cpus = threads_per_core *
-				    cores_per_package;
+				logical_cpus = threads *
+					cores;
 				width = log2(logical_cpus);
 				level = CPUID_TYPE_CORE;
 				x2apic_id = vcpu_id;
