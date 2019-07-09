--- pci_virtio_net.c.orig	2019-06-24 10:51:37.273427000 +0300
+++ pci_virtio_net.c	2019-07-09 12:04:08.950892000 +0300
@@ -159,11 +159,13 @@
 	pthread_mutex_t	rx_mtx;
 	int		rx_vhdrlen;
 	int		rx_merge;	/* merged rx bufs in use */
+	struct token_bucket rx_tb;
 
 	pthread_t 	tx_tid;
 	pthread_mutex_t	tx_mtx;
 	pthread_cond_t	tx_cond;
 	int		tx_in_progress;
+	struct token_bucket tx_tb;
 
 	void (*pci_vtnet_rx)(struct pci_vtnet_softc *sc);
 	void (*pci_vtnet_tx)(struct pci_vtnet_softc *sc, struct iovec *iov,
@@ -344,6 +346,8 @@
 			return;
 		}
 
+		token_bucket_rate_limit(&sc->rx_tb, len);
+
 		/*
 		 * The only valid field in the rx packet header is the
 		 * number of buffers if merged rx bufs were negotiated.
@@ -368,85 +372,106 @@
 }
 
 static __inline int
-pci_vtnet_netmap_writev(struct nm_desc *nmd, struct iovec *iov, int iovcnt)
+pci_vtnet_netmap_writev(struct nm_desc *nmd, struct iovec *iov, int iovcnt, int iovsize)
 {
-	int r, i;
-	int len = 0;
+	char *buf;
+	int i;
+	int frag_size;
+	int iov_off;
+	int len;
+	int nm_off;
+	int nm_buf_size;
 
-	for (r = nmd->cur_tx_ring; ; ) {
-		struct netmap_ring *ring = NETMAP_TXRING(nmd->nifp, r);
-		uint32_t cur, idx;
-		char *buf;
+	struct netmap_ring *ring = NETMAP_TXRING(nmd->nifp, nmd->cur_tx_ring);
 
-		if (nm_ring_empty(ring)) {
-			r++;
-			if (r > nmd->last_tx_ring)
-				r = nmd->first_tx_ring;
-			if (r == nmd->cur_tx_ring)
-				break;
-			continue;
+	if ((nm_ring_space(ring) * ring->nr_buf_size) < iovsize) {
+		/*
+		 * No more avail space in TX ring, try to flush it.
+		 */
+		ioctl(nmd->fd, NIOCTXSYNC, NULL);
+		return (0);
+	}
+
+	i = ring->cur;
+	buf = NETMAP_BUF(ring, ring->slot[i].buf_idx);
+	iov_off = 0;
+	len = iovsize;
+	nm_buf_size = ring->nr_buf_size;
+	nm_off = 0;
+
+	while (iovsize) {
+
+		if (unlikely(iov_off == iov->iov_len)) {
+			iov++;
+			iov_off = 0;
 		}
-		cur = ring->cur;
-		idx = ring->slot[cur].buf_idx;
-		buf = NETMAP_BUF(ring, idx);
 
-		for (i = 0; i < iovcnt; i++) {
-			if (len + iov[i].iov_len > 2048)
-				break;
-			memcpy(&buf[len], iov[i].iov_base, iov[i].iov_len);
-			len += iov[i].iov_len;
+		if (unlikely(nm_off == nm_buf_size)) {
+			ring->slot[i].flags = NS_MOREFRAG;
+			i = nm_ring_next(ring, i);
+			buf = NETMAP_BUF(ring, ring->slot[i].buf_idx);
+			nm_off = 0;
 		}
-		ring->slot[cur].len = len;
-		ring->head = ring->cur = nm_ring_next(ring, cur);
-		nmd->cur_tx_ring = r;
-		ioctl(nmd->fd, NIOCTXSYNC, NULL);
-		break;
+
+		frag_size = MIN(nm_buf_size - nm_off, iov->iov_len - iov_off);
+		memcpy(buf + nm_off, iov->iov_base + iov_off, frag_size);
+
+		iovsize -= frag_size;
+		iov_off += frag_size;
+		nm_off += frag_size;
+
+		ring->slot[i].len = nm_off;
 	}
 
+	/* The last slot must not have NS_MOREFRAG set. */
+	ring->slot[i].flags &= ~NS_MOREFRAG;
+	ring->head = ring->cur = nm_ring_next(ring, i);
+	ioctl(nmd->fd, NIOCTXSYNC, NULL);
+
 	return (len);
 }
 
 static __inline int
-pci_vtnet_netmap_readv(struct nm_desc *nmd, struct iovec *iov, int iovcnt)
+pci_vtnet_netmap_readv(struct nm_desc *nmd, struct iovec *iov, int iovcnt, int iovsize)
 {
-	int len = 0;
-	int i = 0;
-	int r;
+	char *buf;
+	int i;
+	int iov_off;
+	int frag_size;
+	int len;
+	int nm_off;
 
-	for (r = nmd->cur_rx_ring; ; ) {
-		struct netmap_ring *ring = NETMAP_RXRING(nmd->nifp, r);
-		uint32_t cur, idx;
-		char *buf;
-		size_t left;
+	struct netmap_ring *r = NETMAP_RXRING(nmd->nifp, nmd->cur_rx_ring);
 
-		if (nm_ring_empty(ring)) {
-			r++;
-			if (r > nmd->last_rx_ring)
-				r = nmd->first_rx_ring;
-			if (r == nmd->cur_rx_ring)
-				break;
-			continue;
+	i = r->head;
+	buf = NETMAP_BUF(r, r->slot[i].buf_idx);
+	iov_off = 0;
+	nm_off = 0;
+	len = iovsize;
+
+	while (iovsize) {
+
+		if (unlikely(iov_off == iov->iov_len)) {
+			iov++;
+			iov_off = 0;
 		}
-		cur = ring->cur;
-		idx = ring->slot[cur].buf_idx;
-		buf = NETMAP_BUF(ring, idx);
-		left = ring->slot[cur].len;
 
-		for (i = 0; i < iovcnt && left > 0; i++) {
-			if (iov[i].iov_len > left)
-				iov[i].iov_len = left;
-			memcpy(iov[i].iov_base, &buf[len], iov[i].iov_len);
-			len += iov[i].iov_len;
-			left -= iov[i].iov_len;
+		if (unlikely(nm_off == r->slot[i].len)) {
+			i = nm_ring_next(r, i);
+			buf = NETMAP_BUF(r, r->slot[i].buf_idx);
+			nm_off = 0;
 		}
-		ring->head = ring->cur = nm_ring_next(ring, cur);
-		nmd->cur_rx_ring = r;
-		ioctl(nmd->fd, NIOCRXSYNC, NULL);
-		break;
+
+		frag_size = MIN(r->slot[i].len - nm_off, iov->iov_len - iov_off);
+		memcpy(iov->iov_base + iov_off, buf + nm_off, frag_size);
+
+		iovsize -= frag_size;
+		iov_off += frag_size;
+		nm_off += frag_size;
 	}
-	for (; i < iovcnt; i++)
-		iov[i].iov_len = 0;
 
+	r->head = r->cur = nm_ring_next(r, i);
+
 	return (len);
 }
 
@@ -457,32 +482,56 @@
 pci_vtnet_netmap_tx(struct pci_vtnet_softc *sc, struct iovec *iov, int iovcnt,
 		    int len)
 {
-	static char pad[60]; /* all zero bytes */
-
 	if (sc->vsc_nmd == NULL)
 		return;
 
-	/*
-	 * If the length is < 60, pad out to that and add the
-	 * extra zero'd segment to the iov. It is guaranteed that
-	 * there is always an extra iov available by the caller.
-	 */
-	if (len < 60) {
-		iov[iovcnt].iov_base = pad;
-		iov[iovcnt].iov_len = 60 - len;
-		iovcnt++;
+	(void) pci_vtnet_netmap_writev(sc->vsc_nmd, iov, iovcnt, len);
+}
+
+static __inline int
+netmap_next_pkt_len(struct nm_desc *nmd)
+{
+	int i;
+	int len;
+	struct netmap_ring *r = NETMAP_RXRING(nmd->nifp, nmd->cur_rx_ring);
+
+	len = 0;
+
+	if (r->head == r->tail)
+		return 0;
+
+	int num = (r->slot[r->head].flags >> 8) & 0xff;
+
+	if (num > 0) {
+		i = (r->head + num - 1) % 1024;
+		len = 2048 * (num - 1) + r->slot[i].len;
 	}
-	(void) pci_vtnet_netmap_writev(sc->vsc_nmd, iov, iovcnt);
+
+	return (len);
 }
 
+static __inline void
+netmap_drop_pkt(struct nm_desc *nmd)
+{
+	int i;
+	struct netmap_ring *r = NETMAP_RXRING(nmd->nifp, nmd->cur_rx_ring);
+
+	for (i = r->head; i != r->tail; i = nm_ring_next(r, i)) {
+		if (!(r->slot[i].flags & NS_MOREFRAG)) {
+			r->head = r->cur = nm_ring_next(r, i);
+			return;
+		}
+	}
+}
+
 static void
 pci_vtnet_netmap_rx(struct pci_vtnet_softc *sc)
 {
 	struct iovec iov[VTNET_MAXSEGS], *riov;
+	struct virtio_net_rxhdr *vrxh;
 	struct vqueue_info *vq;
-	void *vrx;
-	int len, n;
 	uint16_t idx;
+	int bufs, len, n;
 
 	/*
 	 * Should never be called without a valid netmap descriptor
@@ -497,7 +546,7 @@
 		/*
 		 * Drop the packet and try later.
 		 */
-		(void) nm_nextpkt(sc->vsc_nmd, (void *)dummybuf);
+		netmap_drop_pkt(sc->vsc_nmd);
 		return;
 	}
 
@@ -510,58 +559,69 @@
 		 * Drop the packet and try later.  Interrupt on
 		 * empty, if that's negotiated.
 		 */
-		(void) nm_nextpkt(sc->vsc_nmd, (void *)dummybuf);
+		netmap_drop_pkt(sc->vsc_nmd);
 		vq_endchains(vq, 1);
 		return;
 	}
 
 	do {
-		/*
-		 * Get descriptor chain.
-		 */
-		n = vq_getchain(vq, &idx, iov, VTNET_MAXSEGS, NULL);
-		assert(n >= 1 && n <= VTNET_MAXSEGS);
+		len = netmap_next_pkt_len(sc->vsc_nmd);
 
-		/*
-		 * Get a pointer to the rx header, and use the
-		 * data immediately following it for the packet buffer.
-		 */
-		vrx = iov[0].iov_base;
-		riov = rx_iov_trim(iov, &n, sc->rx_vhdrlen);
-
-		len = pci_vtnet_netmap_readv(sc->vsc_nmd, riov, n);
-
-		if (len == 0) {
+		if (unlikely(len == 0)) {
 			/*
 			 * No more packets, but still some avail ring
 			 * entries.  Interrupt if needed/appropriate.
 			 */
-			vq_retchain(vq);
 			vq_endchains(vq, 0);
 			return;
 		}
 
+		if (sc->rx_merge) {
+			/*
+			 * Get mergable buffers.
+			 */ 
+			n = vq_getbufs_mrgrx(vq, iov, VTNET_MAXSEGS,
+					len + sc->rx_vhdrlen, &bufs);
+		} else {
+			/*
+			 * Get descriptor chain.
+			 */
+			n = vq_getchain(vq, &idx, iov, VTNET_MAXSEGS, NULL);
+		}
+
+		if (n <= 0) {
+			vq_endchains(vq, 0);
+			return;
+		}
+
 		/*
-		 * The only valid field in the rx packet header is the
-		 * number of buffers if merged rx bufs were negotiated.
+		 * Get a pointer to the rx header, and use the
+		 * data immediately following it for the packet buffer.
 		 */
-		memset(vrx, 0, sc->rx_vhdrlen);
+		vrxh = iov[0].iov_base;
+		memset(vrxh, 0, sc->rx_vhdrlen);
 
-		if (sc->rx_merge) {
-			struct virtio_net_rxhdr *vrxh;
+		riov = rx_iov_trim(iov, &n, sc->rx_vhdrlen);
 
-			vrxh = vrx;
-			vrxh->vrh_bufs = 1;
-		}
+		(void)pci_vtnet_netmap_readv(sc->vsc_nmd, riov, n, len);
 
+		token_bucket_rate_limit(&sc->rx_tb, len);
+
 		/*
-		 * Release this chain and handle more chains.
+		 * Release used descriptors.
 		 */
-		vq_relchain(vq, idx, len + sc->rx_vhdrlen);
+		if (sc->rx_merge) {
+			vrxh->vrh_bufs = bufs;
+			vq_relbufs_mrgrx(vq, bufs);
+		} else {
+			vq_relchain(vq, idx, len + sc->rx_vhdrlen);
+		}
+
 	} while (vq_has_descs(vq));
 
 	/* Interrupt if needed, including for NOTIFY_ON_EMPTY. */
 	vq_endchains(vq, 1);
+
 }
 
 static void
@@ -614,6 +674,9 @@
 	}
 
 	DPRINTF(("virtio: packet send, %d bytes, %d segs\n\r", plen, n));
+
+	token_bucket_rate_limit(&sc->tx_tb, plen);
+
 	sc->pci_vtnet_tx(sc, &iov[1], n - 1, plen);
 
 	/* chain is processed, release it and set tlen */
@@ -777,9 +840,12 @@
 {
 	char tname[MAXCOMLEN + 1];
 	struct pci_vtnet_softc *sc;
+	char *cp;
 	char *devname;
-	char *vtopts;
+	char *xopts;
 	int mac_provided;
+	uint64_t tx_rate_limit;
+	uint64_t rx_rate_limit;
 
 	sc = calloc(1, sizeof(struct pci_vtnet_softc));
 
@@ -804,19 +870,37 @@
 	mac_provided = 0;
 	sc->vsc_tapfd = -1;
 	sc->vsc_nmd = NULL;
+
+	rx_rate_limit = 0;
+	tx_rate_limit = 0;
+	memset(&sc->rx_tb, 0, sizeof(struct token_bucket));
+	memset(&sc->tx_tb, 0, sizeof(struct token_bucket));
+
 	if (opts != NULL) {
 		int err;
 
-		devname = vtopts = strdup(opts);
-		(void) strsep(&vtopts, ",");
+		devname = xopts = strdup(opts);
+		while (xopts != NULL) {
+			cp = strsep(&xopts, ",");
 
-		if (vtopts != NULL) {
-			err = net_parsemac(vtopts, sc->vsc_config.mac);
-			if (err != 0) {
+			if (cp == devname)		/* device name */
+				continue;
+			else if (!memcmp(cp, "mac", 3)) {
+				err = net_parsemac(cp, sc->vsc_config.mac);
+				if (err != 0) {
+					free(devname);
+					return (err);
+				}
+				mac_provided = 1;
+			} else if (sscanf(cp, "ratelimit=%ld/%ld", &tx_rate_limit, &rx_rate_limit) == 2)
+				;
+			else if (sscanf(cp, "ratelimit=%ld", &tx_rate_limit) == 1)
+				rx_rate_limit = tx_rate_limit;
+			else {
 				free(devname);
+				fprintf(stderr, "Invalid device option \"%s\"\n", cp);
 				return (err);
 			}
-			mac_provided = 1;
 		}
 
 		if (strncmp(devname, "vale", 4) == 0)
@@ -826,6 +910,9 @@
 			pci_vtnet_tap_setup(sc, devname);
 
 		free(devname);
+
+		sc->tx_tb.rate = tx_rate_limit;
+		sc->rx_tb.rate = rx_rate_limit;
 	}
 
 	if (!mac_provided) {
