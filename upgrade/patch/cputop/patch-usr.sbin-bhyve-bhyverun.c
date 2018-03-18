--- bhyverun.c.bak	2018-03-16 17:41:10.550978000 +0300
+++ bhyverun.c	2018-03-16 18:28:09.112720000 +0300
@@ -93,6 +93,8 @@
 char *vmname;
 
 int guest_ncpus;
+uint16_t cores, maxcpus, sockets, threads;
+
 char *guest_uuid_str;
 
 static int guest_vmexit_on_hlt, guest_vmexit_on_pause;
@@ -137,11 +139,13 @@
 {
 
         fprintf(stderr,
-                "Usage: %s [-abehuwxACHPSWY] [-c vcpus] [-g <gdb port>] [-l <lpc>]\n"
+		"Usage: %s [-abehuwxACHPSWY]\n"
+		"       %*s [-c [cpus=]numcpus[,sockets=n][,cores=n][,threads=n]]\n"
+		"       %*s [-g <gdb port>] [-l <lpc>]\n"
 		"       %*s [-m mem] [-p vcpu:hostcpu] [-s <pci>] [-U uuid] <vm>\n"
 		"       -a: local apic is in xAPIC mode (deprecated)\n"
 		"       -A: create ACPI tables\n"
-		"       -c: # cpus (default 1)\n"
+		"       -c: [cpus=]numcpus[,sockets=n][,cores=n][,threads=n]\n"
 		"       -C: include guest memory in core file\n"
 		"       -e: exit on unhandled I/O access\n"
 		"       -g: gdb port\n"
@@ -159,12 +163,58 @@
 		"       -W: force virtio to use single-vector MSI\n"
 		"       -x: local apic is in x2APIC mode\n"
 		"       -Y: disable MPtable generation\n",
-		progname, (int)strlen(progname), "");
+		progname, (int)strlen(progname), "", (int)strlen(progname), "",
+		(int)strlen(progname), "");
 
 	exit(code);
 }
 
 static int
+topology_parse(const char *opt)
+{
+	char *cp, *str;
+	int i;
+	uint16_t tmp;
+
+	str = strdup(opt);
+	if ((cp = strchr(str, ',')) != NULL) {
+		for (i = 0; str; i++) {
+			if (sscanf(str, "cpus=%hu", &tmp) == 1)
+				guest_ncpus = tmp;
+			else if (sscanf(str, "sockets=%hu", &tmp) == 1)
+				sockets = tmp;
+			else if (sscanf(str, "cores=%hu", &tmp) == 1)
+				cores = tmp;
+			else if (sscanf(str, "threads=%hu", &tmp) == 1)
+				threads = tmp;
+#ifdef notyet  /* Do not expose this until vmm.ko implements it */
+			else if (sscanf(str, "maxcpus=%hu", &tmp) == 1)
+				maxcpus = tmp;
+#endif
+			else
+				return (-1);
+			if (cp == NULL)
+				str = NULL;
+			else {
+				str = ++cp;
+				cp = strchr(str, ',');
+			}
+		}
+	} else {
+		if (sscanf(str, "cpus=%hu", &tmp) == 1)
+			guest_ncpus = tmp;
+		else if (sscanf(str, "%hu", &tmp) == 1)
+			guest_ncpus = tmp;
+		else
+			return (-1);
+	}
+	if (guest_ncpus != sockets * cores * threads)
+		return (-1);
+	else
+		return(0);
+}
+
+static int
 pincpu_parse(const char *opt)
 {
 	int vcpu, pcpu;
@@ -783,6 +833,11 @@
 			exit(1);
 		}
 	}
+	error = vm_set_topology(ctx, sockets, cores, threads, maxcpus);
+	if (error) {
+		perror("vm_set_topology");
+		exit(1);
+	}
 	return (ctx);
 }
 
@@ -801,6 +856,8 @@
 	progname = basename(argv[0]);
 	gdb_port = 0;
 	guest_ncpus = 1;
+	sockets = cores = threads = 1;
+	maxcpus = 0;
 	memsize = 256 * MB;
 	mptgen = 1;
 	rtc_localtime = 1;
@@ -825,7 +882,10 @@
                         }
 			break;
                 case 'c':
-			guest_ncpus = atoi(optarg);
+			if (topology_parse(optarg) !=0) {
+				errx(EX_USAGE, "invalid cpu topology "
+				"'%s'", optarg);
+			}
 			break;
 		case 'C':
 			memflags |= VM_MEM_F_INCORE;
