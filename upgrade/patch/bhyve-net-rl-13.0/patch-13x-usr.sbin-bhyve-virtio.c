--- virtio.c.orig	2019-06-13 12:18:25.432816000 +0300
+++ virtio.c	2019-07-09 12:05:06.902182000 +0300
@@ -41,6 +41,7 @@
 #include <pthread_np.h>
 
 #include "bhyverun.h"
+#include "iov.h"
 #include "pci_emul.h"
 #include "virtio.h"
 
@@ -381,6 +382,61 @@
 	return (-1);
 }
 
+int	vq_getbufs_mrgrx(struct vqueue_info *vq, struct iovec *iov,
+		int n_iov, int len, int *u_cnt)
+{
+	uint16_t idx;
+	uint16_t uidx, mask;
+	int i, iov_len;
+	int bufs, last_avail_saved, n;
+	int total_len;
+	volatile struct virtio_used *vue;
+
+	i = 0;
+	bufs = 0;
+	total_len = 0;
+	mask = vq->vq_qsize - 1;
+	uidx = vq->vq_used->vu_idx;
+
+	/*
+	 * vq_getchain() increment the last avail index.
+	 * Save it to restore if there are no enough buffers to store packet.
+	 */
+	last_avail_saved = vq->vq_last_avail;
+	while (1) {
+		n = vq_getchain(vq, &idx, &iov[i], n_iov - i, NULL);
+
+		if (n <= 0) {
+			/* Restore the last avail index. */
+			vq->vq_last_avail = last_avail_saved;
+			*u_cnt = 0;
+			return (n);
+		}
+
+		iov_len = count_iov(&iov[i], n);
+		i += n;
+		total_len += iov_len;
+
+		vue = &vq->vq_used->vu_ring[uidx++ & mask];
+		vue->vu_idx = idx;
+
+		if (total_len < len) {
+			vue->vu_tlen = iov_len;
+			bufs++;
+		} else {
+			vue->vu_tlen = iov_len - (total_len - len);
+			bufs++;
+			break;
+		}
+
+	};
+
+	*u_cnt = bufs;
+
+	return i;
+
+}
+
 /*
  * Return the currently-first request chain back to the available queue.
  *
@@ -431,6 +487,23 @@
 	 * This is necessary on ISAs with memory ordering less strict than x86
 	 * (and even on x86 to act as a compiler barrier).
 	 */
+	atomic_thread_fence_rel();
+	vuh->vu_idx = uidx;
+}
+
+/*
+ * Return specified merged rx buffers to the guest, setting its I/O length.
+ */
+void
+vq_relbufs_mrgrx(struct vqueue_info *vq, int nbufs)
+{
+	uint16_t uidx;
+	volatile struct vring_used *vuh;
+
+	vuh = vq->vq_used;
+
+	uidx = vuh->vu_idx + nbufs;
+
 	atomic_thread_fence_rel();
 	vuh->vu_idx = uidx;
 }
