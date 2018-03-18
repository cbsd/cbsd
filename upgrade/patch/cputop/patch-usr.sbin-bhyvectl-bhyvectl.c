--- bhyvectl.c.bak	2018-03-16 17:41:09.326261000 +0300
+++ bhyvectl.c	2018-03-16 18:31:37.994047000 +0300
@@ -191,7 +191,8 @@
 	"       [--get-msr-bitmap]\n"
 	"       [--get-msr-bitmap-address]\n"
 	"       [--get-guest-sysenter]\n"
-	"       [--get-exit-reason]\n",
+	"       [--get-exit-reason]\n"
+	"       [--get-cpu-topology]\n",
 	progname);
 
 	if (cpu_intel) {
@@ -285,6 +286,7 @@
 enum x2apic_state x2apic_state;
 static int unassign_pptdev, bus, slot, func;
 static int run;
+static int get_cpu_topology;
 
 /*
  * VMCB specific.
@@ -1456,6 +1458,7 @@
 		{ "get-active-cpus", 	NO_ARG,	&get_active_cpus, 	1 },
 		{ "get-suspended-cpus", NO_ARG,	&get_suspended_cpus, 	1 },
 		{ "get-intinfo", 	NO_ARG,	&get_intinfo,		1 },
+		{ "get-cpu-topology",	NO_ARG, &get_cpu_topology,	1 },
 	};
 
 	const struct option intel_opts[] = {
@@ -2310,6 +2313,14 @@
 				printf("%-40s\t%ld\n", desc, stats[i]);
 			}
 		}
+	}
+
+	if (!error && (get_cpu_topology || get_all)) {
+		uint16_t sockets, cores, threads, maxcpus;
+
+		vm_get_topology(ctx, &sockets, &cores, &threads, &maxcpus);
+		printf("cpu_topology:\tsockets=%hu, cores=%hu, threads=%hu, "
+			"maxcpus=%hu\n", sockets, cores, threads, maxcpus);
 	}
 
 	if (!error && run) {
