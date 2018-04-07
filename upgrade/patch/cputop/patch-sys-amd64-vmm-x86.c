--- x86.c.orig	2018-03-30 16:11:58.628101000 +0300
+++ x86.c	2018-03-30 18:19:26.021724000 +0300
@@ -60,16 +60,15 @@
 SYSCTL_ULONG(_hw_vmm, OID_AUTO, bhyve_xcpuids, CTLFLAG_RW, &bhyve_xcpuids, 0,
     "Number of times an unknown cpuid leaf was accessed");
 
-/*
- * The default CPU topology is a single thread per package.
- */
-static u_int threads_per_core = 1;
+#if __FreeBSD_version < 1200058	/* Remove after 11 EOL helps MFCing */
+extern u_int threads_per_core;
 SYSCTL_UINT(_hw_vmm_topology, OID_AUTO, threads_per_core, CTLFLAG_RDTUN,
     &threads_per_core, 0, NULL);
 
-static u_int cores_per_package = 1;
+extern u_int cores_per_package;
 SYSCTL_UINT(_hw_vmm_topology, OID_AUTO, cores_per_package, CTLFLAG_RDTUN,
     &cores_per_package, 0, NULL);
+#endif
 
 static int cpuid_leaf_b = 1;
 SYSCTL_INT(_hw_vmm_topology, OID_AUTO, cpuid_leaf_b, CTLFLAG_RDTUN,
@@ -95,6 +94,7 @@
 	int error, enable_invpcid, level, width, x2apic_id;
 	unsigned int func, regs[4], logical_cpus;
 	enum x2apic_state x2apic_state;
+	uint16_t cores, maxcpus, sockets, threads;
 
 	VCPU_CTR2(vm, vcpu_id, "cpuid %#x,%#x", *eax, *ecx);
 
@@ -142,11 +142,12 @@
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
+				    &maxcpus);
+				logical_cpus = threads *
+				    cores;
 				regs[2] = logical_cpus - 1;
 			}
 			break;
@@ -305,7 +306,9 @@
 			 */
 			regs[3] |= (CPUID_MCA | CPUID_MCE | CPUID_MTRR);
 
-			logical_cpus = threads_per_core * cores_per_package;
+			vm_get_topology(vm, &sockets, &cores, &threads,
+			    &maxcpus);
+			logical_cpus = threads * cores;
 			regs[1] &= ~CPUID_HTT_CORES;
 			regs[1] |= (logical_cpus & 0xff) << 16;
 			regs[3] |= CPUID_HTT;
@@ -315,8 +318,10 @@
 			cpuid_count(*eax, *ecx, regs);
 
 			if (regs[0] || regs[1] || regs[2] || regs[3]) {
+				vm_get_topology(vm, &sockets, &cores, &threads,
+				    &maxcpus);
 				regs[0] &= 0x3ff;
-				regs[0] |= (cores_per_package - 1) << 26;
+				regs[0] |= (cores - 1) << 26;
 				/*
 				 * Cache topology:
 				 * - L1 and L2 are shared only by the logical
@@ -324,10 +329,10 @@
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
@@ -389,16 +394,18 @@
 			/*
 			 * Processor topology enumeration
 			 */
+			vm_get_topology(vm, &sockets, &cores, &threads,
+			    &maxcpus);
 			if (*ecx == 0) {
-				logical_cpus = threads_per_core;
+				logical_cpus = threads;
 				width = log2(logical_cpus);
 				level = CPUID_TYPE_SMT;
 				x2apic_id = vcpu_id;
 			}
 
 			if (*ecx == 1) {
-				logical_cpus = threads_per_core *
-				    cores_per_package;
+				logical_cpus = threads *
+				    cores;
 				width = log2(logical_cpus);
 				level = CPUID_TYPE_CORE;
 				x2apic_id = vcpu_id;
