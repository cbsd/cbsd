-- -if_vxlan.c.orig 2020 - 09 - 18 19 : 40 : 29.163326000 +
    0000 ++ +if_vxlan.c 2020 - 09 - 18 19 : 51 : 00.255747000 + 0000 @ @-2759,
    7 + 2759, 10 @ @vso = xvso;
offset += sizeof(struct udphdr);

- if (m->m_pkthdr.len <
    offset + sizeof(struct vxlan_header)) + /*
				    +	* Drop if the mbuf len is not enough to
				    store inner Ethernet frame.
				    +	*/
    +if (m->m_pkthdr.len <
	(offset + sizeof(struct vxlan_header) + ETHER_HDR_LEN))goto out;

if (__predict_false(m->m_len < offset + sizeof(struct vxlan_header))) {
	@ @-2799, 7 + 2802, 7 @ @ struct ifnet *ifp;
	struct mbuf *m;
	struct ether_header *eh;
	-int error;
	+int error = 0;

	sc = vxlan_socket_lookup_softc(vso, vni);
	if (sc == NULL)
		@ @-2847, 7 + 2850, 7 @ @m->m_pkthdr.csum_data = 0;
}

- error = netisr_dispatch(NETISR_ETHER, m);
+ (*ifp->if_input)(ifp, m);
*m0 = NULL;

out:
