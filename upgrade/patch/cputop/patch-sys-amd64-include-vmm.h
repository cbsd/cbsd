--- vmm.h.orig	2018-03-30 16:11:58.341559000 +0300
+++ vmm.h	2018-03-30 18:19:26.019621000 +0300
@@ -181,6 +181,10 @@
 void vm_destroy(struct vm *vm);
 int vm_reinit(struct vm *vm);
 const char *vm_name(struct vm *vm);
+void vm_get_topology(struct vm *vm, uint16_t *sockets, uint16_t *cores,
+    uint16_t *threads, uint16_t *maxcpus);
+int vm_set_topology(struct vm *vm, uint16_t sockets, uint16_t cores,
+    uint16_t threads, uint16_t maxcpus);
 
 /*
  * APIs that modify the guest memory map require all vcpus to be frozen.
