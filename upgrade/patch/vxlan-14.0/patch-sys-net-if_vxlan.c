--- if_vxlan.c.orig	2021-04-01 10:36:57.568756000 +0300
+++ if_vxlan.c	2021-04-02 10:35:25.147957000 +0300
@@ -2769,7 +2769,10 @@
 	vso = xvso;
 	offset += sizeof(struct udphdr);
 
-	if (m->m_pkthdr.len < offset + sizeof(struct vxlan_header))
+	/*
+	* Drop if the mbuf len is not enough to store inner Ethernet frame.
+	*/
+	if (m->m_pkthdr.len < (offset + sizeof(struct vxlan_header) + ETHER_HDR_LEN))
 		goto out;
 
 	if (__predict_false(m->m_len < offset + sizeof(struct vxlan_header))) {
@@ -2810,7 +2813,7 @@
 	struct ifnet *ifp;
 	struct mbuf *m;
 	struct ether_header *eh;
-	int error;
+	int error = 0;
 
 	sc = vxlan_socket_lookup_softc(vso, vni);
 	if (sc == NULL)
@@ -2857,7 +2860,7 @@
 		m->m_pkthdr.csum_data = 0;
 	}
 
-	error = netisr_dispatch(NETISR_ETHER, m);
+	(*ifp->if_input)(ifp, m);
 	*m0 = NULL;
 
 out:
