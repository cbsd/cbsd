--- vmm.h.bak	2018-03-16 17:40:37.873890000 +0300
+++ vmm.h	2018-03-16 17:51:38.786938000 +0300
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
