--- vmmapi.h.orig	2018-03-30 16:12:30.503096000 +0300
+++ vmmapi.h	2018-03-30 18:19:26.016883000 +0300
@@ -218,6 +218,12 @@
 int	vm_suspended_cpus(struct vmctx *ctx, cpuset_t *cpus);
 int	vm_activate_cpu(struct vmctx *ctx, int vcpu);
 
+/* Cpu topology */
+int	vm_set_topology(struct vmctx *ctx, uint16_t sockets, uint16_t cores, \
+	    uint16_t threads, uint16_t maxcpus);
+int	vm_get_topology(struct vmctx *ctx, uint16_t *sockets, uint16_t *cores, \
+	    uint16_t *threads, uint16_t *maxcpus);
+
 /*
  * FreeBSD specific APIs
  */
