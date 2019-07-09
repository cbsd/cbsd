--- virtio.h.orig	2019-06-13 12:18:25.433485000 +0300
+++ virtio.h	2019-07-09 12:09:23.611186000 +0300
@@ -479,8 +479,11 @@
 
 int	vq_getchain(struct vqueue_info *vq, uint16_t *pidx,
 		    struct iovec *iov, int n_iov, uint16_t *flags);
+int	vq_getbufs_mrgrx(struct vqueue_info *vq, struct iovec *iov,
+			int n_iov, int len, int *u_cnt);
 void	vq_retchain(struct vqueue_info *vq);
 void	vq_relchain(struct vqueue_info *vq, uint16_t idx, uint32_t iolen);
+void	vq_relbufs_mrgrx(struct vqueue_info *vq, int nbufs);
 void	vq_endchains(struct vqueue_info *vq, int used_all_avail);
 
 uint64_t vi_pci_read(struct vmctx *ctx, int vcpu, struct pci_devinst *pi,
