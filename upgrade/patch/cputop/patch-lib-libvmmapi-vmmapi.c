--- vmmapi.c.orig	2018-03-30 16:12:30.502833000 +0300
+++ vmmapi.c	2018-03-30 18:19:26.017249000 +0300
@@ -1475,6 +1475,38 @@
 }
 
 int
+vm_set_topology(struct vmctx *ctx, \
+    uint16_t sockets, uint16_t cores, uint16_t threads, uint16_t maxcpus)
+{
+	struct vm_cpu_topology topology;
+
+	bzero(&topology, sizeof (struct vm_cpu_topology));
+	topology.sockets = sockets;
+	topology.cores = cores;
+	topology.threads = threads;
+	topology.maxcpus = maxcpus;
+	return (ioctl(ctx->fd, VM_SET_TOPOLOGY, &topology));
+}
+
+int
+vm_get_topology(struct vmctx *ctx, \
+    uint16_t *sockets, uint16_t *cores, uint16_t *threads, uint16_t *maxcpus)
+{
+	struct vm_cpu_topology topology;
+	int error;
+
+	bzero(&topology, sizeof (struct vm_cpu_topology));
+	error = ioctl(ctx->fd, VM_GET_TOPOLOGY, &topology);
+	if (error == 0) {
+		*sockets = topology.sockets;
+		*cores = topology.cores;
+		*threads = topology.threads;
+		*maxcpus = topology.maxcpus;
+	}
+	return (error);
+}
+
+int
 vm_get_device_fd(struct vmctx *ctx)
 {
 
@@ -1503,7 +1535,7 @@
 	    VM_GLA2GPA_NOFAULT,
 	    VM_ACTIVATE_CPU, VM_GET_CPUS, VM_SET_INTINFO, VM_GET_INTINFO,
 	    VM_RTC_WRITE, VM_RTC_READ, VM_RTC_SETTIME, VM_RTC_GETTIME,
-	    VM_RESTART_INSTRUCTION };
+	    VM_RESTART_INSTRUCTION, VM_SET_TOPOLOGY, VM_GET_TOPOLOGY };
 
 	if (len == NULL) {
 		cmds = malloc(sizeof(vm_ioctl_cmds));
