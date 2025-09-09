--- pci_xhci.c.orig	2025-08-27 22:58:14.687029000 +0300
+++ pci_xhci.c	2025-08-27 22:31:08.280046000 +0300
@@ -406,7 +406,7 @@
 				 * XHCI 4.19.3 USB2 RxDetect->Polling,
 				 *             USB3 Polling->U0
 				 */
-				if (dev->hci.hci_usbver == 2)
+				if (dev->hci.hci_usbver <= 2)
 					port->portsc |=
 					    XHCI_PS_PLS_SET(UPS_PORT_LS_POLL);
 				else
@@ -625,6 +625,7 @@
 static void
 pci_xhci_assert_interrupt(struct pci_xhci_softc *sc)
 {
+	DPRINTF(("%s", __func__));
 
 	sc->rtsregs.intrreg.erdp |= XHCI_ERDP_LO_BUSY;
 	sc->rtsregs.intrreg.iman |= XHCI_IMAN_INTR_PEND;
@@ -1152,8 +1153,9 @@
 	uint32_t	type;
 
 	epid = XHCI_TRB_3_EP_GET(trb->dwTrb3);
+	type = XHCI_TRB_3_TYPE_GET(trb->dwTrb3);
 
-	DPRINTF(("pci_xhci: reset ep %u: slot %u", epid, slot));
+	DPRINTF(("pci_xhci: reset ep %u: slot %u: type %u", epid, slot, type));
 
 	cmderr = pci_xhci_validate_slot(slot);
 	if (cmderr != XHCI_TRB_ERROR_SUCCESS)
@@ -1162,8 +1164,6 @@
 	dev = XHCI_SLOTDEV_PTR(sc, slot);
 	assert(dev != NULL);
 
-	type = XHCI_TRB_3_TYPE_GET(trb->dwTrb3);
-
 	if (type == XHCI_TRB_TYPE_STOP_EP &&
 	    (trb->dwTrb3 & XHCI_TRB_3_SUSP_EP_BIT) != 0) {
 		/* XXX suspend endpoint for 10ms */
@@ -1176,8 +1176,6 @@
 	}
 
 	devep = &dev->eps[epid];
-	if (devep->ep_xfer != NULL)
-		USB_DATA_XFER_RESET(devep->ep_xfer);
 
 	dev_ctx = dev->dev_ctx;
 	assert(dev_ctx != NULL);
@@ -1199,6 +1197,9 @@
 		cmderr = XHCI_TRB_ERROR_ENDP_NOT_ON;
 		goto done;
 	}
+
+	if (devep->ep_xfer != NULL)
+		USB_DATA_XFER_RESET(devep->ep_xfer);
 
 done:
 	return (cmderr);
@@ -1610,10 +1611,12 @@
 
 		if (!xfer->data[i].processed) {
 			xfer->head = i;
+			err = XHCI_TRB_ERROR_INVALID;
 			break;
 		}
 
 		xfer->ndata--;
+		xfer->head = (xfer->head + 1) % USB_MAX_XFER_BLOCKS;
 		edtla += xfer->data[i].bdone;
 
 		trb->dwTrb3 = (trb->dwTrb3 & ~0x1) | (xfer->data[i].ccs);
@@ -1621,6 +1624,12 @@
 		pci_xhci_update_ep_ring(sc, dev, devep, ep_ctx,
 		    xfer->data[i].streamid, xfer->data[i].trbnext,
 		    xfer->data[i].ccs);
+		if (xfer->data[i].blen > 0)
+			err = XHCI_TRB_ERROR_SHORT_PKT;
+		if (xfer->data[i].status == USB_NEXT_DATA) {
+			i = (i + 1) % USB_MAX_XFER_BLOCKS;
+			continue;
+		}
 
 		/* Only interrupt if IOC or short packet */
 		if (!(trb->dwTrb3 & XHCI_TRB_3_IOC_BIT) &&
@@ -1730,13 +1739,11 @@
 		} else {
 			err = pci_xhci_xfer_complete(sc, xfer, slot, epid,
 			                             &do_intr);
-			if (err == XHCI_TRB_ERROR_SUCCESS && do_intr) {
-				pci_xhci_assert_interrupt(sc);
+			if (err == XHCI_TRB_ERROR_SUCCESS) {
+				if (do_intr)
+					pci_xhci_assert_interrupt(sc);
+				USB_DATA_XFER_RESET(xfer);
 			}
-
-
-			/* XXX should not do it if error? */
-			USB_DATA_XFER_RESET(xfer);
 		}
 	}
 
@@ -1756,6 +1763,7 @@
 	struct xhci_trb *setup_trb;
 	struct usb_data_xfer *xfer;
 	struct usb_data_xfer_block *xfer_block;
+	struct usb_data_xfer_block *prev_xfer_block = NULL;
 	uint64_t	val;
 	uint32_t	trbflags;
 	int		do_intr, err;
@@ -1798,6 +1806,8 @@
 			xfer_block = usb_data_xfer_append(xfer, NULL, 0,
 			                                  (void *)addr, ccs);
 			xfer_block->processed = 1;
+			if (trb->dwTrb3 & XHCI_TRB_3_TC_BIT)
+				xfer_block->ccs = ccs;
 			break;
 
 		case XHCI_TRB_TYPE_SETUP_STAGE:
@@ -1817,8 +1827,8 @@
 			       sizeof(struct usb_device_request));
 
 			xfer_block = usb_data_xfer_append(xfer, NULL, 0,
-			                                  (void *)addr, ccs);
-			xfer_block->processed = 1;
+			    (void *)addr, ccs);
+			xfer_block->status = USB_NO_DATA;
 			break;
 
 		case XHCI_TRB_TYPE_NORMAL:
@@ -1836,6 +1846,14 @@
 			     (void *)(trbflags & XHCI_TRB_3_IDT_BIT ?
 			         &trb->qwTrb0 : XHCI_GADDR(sc, trb->qwTrb0)),
 			     trb->dwTrb2 & 0x1FFFF, (void *)addr, ccs);
+			if (xfer_block == NULL) {
+				err = USB_ERR_STALLED;
+				break;
+			}
+			xfer_block->status = trbflags & XHCI_TRB_3_CHAIN_BIT ?
+			    USB_NEXT_DATA :
+			    USB_LAST_DATA;
+			prev_xfer_block = xfer_block;
 			break;
 
 		case XHCI_TRB_TYPE_STATUS_STAGE:
@@ -1855,6 +1873,11 @@
 			if ((epid > 1) && (trbflags & XHCI_TRB_3_IOC_BIT)) {
 				xfer_block->processed = 1;
 			}
+			if (prev_xfer_block != NULL &&
+			    prev_xfer_block->status == USB_NEXT_DATA) {
+				prev_xfer_block->status = USB_LAST_DATA;
+				prev_xfer_block = NULL;
+			}
 			break;
 
 		default:
@@ -1873,17 +1896,22 @@
 			xfer_block->trbnext = addr;
 			xfer_block->streamid = streamid;
 		}
+
+		if (trbflags & XHCI_TRB_3_BEI_BIT)
+			continue;
 
 		if (!setup_trb && !(trbflags & XHCI_TRB_3_CHAIN_BIT) &&
 		    XHCI_TRB_3_TYPE_GET(trbflags) != XHCI_TRB_TYPE_LINK) {
 			break;
 		}
+
+		if (XHCI_TRB_3_TYPE_GET(trbflags) == XHCI_TRB_TYPE_EVENT_DATA)
+			continue;
 
 		/* handle current batch that requires interrupt on complete */
 		if (trbflags & XHCI_TRB_3_IOC_BIT) {
 			DPRINTF(("pci_xhci: trb IOC bit set"));
-			if (epid == 1)
-				do_retry = 1;
+			do_retry = 1;
 			break;
 		}
 	}
@@ -2586,6 +2614,7 @@
 	port = XHCI_PORTREG_PTR(sc, portn);
 	dev = XHCI_DEVINST_PTR(sc, portn);
 	if (dev) {
+		dev->dev_ue->ue_reset(dev->dev_sc);
 		port->portsc &= ~(XHCI_PS_PLS_MASK | XHCI_PS_PR | XHCI_PS_PRC);
 		port->portsc |= XHCI_PS_PED |
 		    XHCI_PS_SPEED_SET(dev->hci.hci_speed);
@@ -2620,7 +2649,7 @@
 		port->portsc = XHCI_PS_CCS |		/* connected */
 		               XHCI_PS_PP;		/* port power */
 
-		if (dev->hci.hci_usbver == 2) {
+		if (dev->hci.hci_usbver <= 2) {
 			port->portsc |= XHCI_PS_PLS_SET(UPS_PORT_LS_POLL) |
 			    XHCI_PS_SPEED_SET(dev->hci.hci_speed);
 		} else {
@@ -2636,6 +2665,34 @@
 	}
 }
 
+static void
+pci_xhci_deinit_port(struct pci_xhci_softc *sc, int portn)
+{
+	struct pci_xhci_portregs *port;
+	struct pci_xhci_dev_emu *dev;
+
+	port = XHCI_PORTREG_PTR(sc, portn);
+	dev = XHCI_DEVINST_PTR(sc, portn);
+	if (dev) {
+		port->portsc &= ~(XHCI_PS_CCS | /* connected */
+		    XHCI_PS_PP);		/* port power */
+
+		if (dev->hci.hci_usbver <= 2) {
+			port->portsc &= ~(XHCI_PS_PLS_SET(UPS_PORT_LS_POLL) |
+			    XHCI_PS_SPEED_SET(dev->hci.hci_speed));
+		} else {
+			port->portsc &= ~(XHCI_PS_PLS_SET(UPS_PORT_LS_U0) |
+			    XHCI_PS_PED | /* enabled */
+			    XHCI_PS_SPEED_SET(dev->hci.hci_speed));
+		}
+
+		DPRINTF(("Deinit port %d 0x%x", portn, port->portsc));
+	} else {
+		port->portsc = XHCI_PS_PLS_SET(UPS_PORT_LS_RX_DET) | XHCI_PS_PP;
+		DPRINTF(("Deinit empty port %d 0x%x", portn, port->portsc));
+	}
+}
+
 static int
 pci_xhci_dev_intr(struct usb_hci *hci, int epctx)
 {
@@ -2694,7 +2751,7 @@
 
 	DPRINTF(("xhci device interrupt on endpoint %d", epid));
 
-	pci_xhci_device_doorbell(sc, hci->hci_port, epid, 0);
+	pci_xhci_device_doorbell(sc, hci->hci_slot, epid, 0);
 
 done:
 	return (error);
@@ -2704,7 +2761,34 @@
 pci_xhci_dev_event(struct usb_hci *hci, enum hci_usbev evid __unused,
     void *param __unused)
 {
+	struct xhci_trb evtrb;
+	struct pci_xhci_dev_emu *dev = hci->hci_sc;
+	struct pci_xhci_softc *xsc = dev->xsc;
+	struct pci_xhci_portregs *port;
+
+	port = XHCI_PORTREG_PTR(xsc, hci->hci_port);
 	DPRINTF(("xhci device event port %d", hci->hci_port));
+
+	switch (evid) {
+	case USBDEV_ATTACH:
+		pci_xhci_init_port(xsc, hci->hci_port);
+		port->portsc |= XHCI_PS_CSC;
+		pci_xhci_set_evtrb(&evtrb, hci->hci_port,
+		    XHCI_TRB_ERROR_SUCCESS, XHCI_TRB_EVENT_PORT_STS_CHANGE);
+		return (pci_xhci_insert_event(xsc, &evtrb, 1) !=
+		    XHCI_TRB_ERROR_SUCCESS);
+		break;
+	case USBDEV_REMOVE:
+		pci_xhci_deinit_port(xsc, hci->hci_port);
+		port->portsc |= XHCI_PS_CSC;
+		pci_xhci_set_evtrb(&evtrb, hci->hci_port,
+		    XHCI_TRB_ERROR_SUCCESS, XHCI_TRB_EVENT_PORT_STS_CHANGE);
+		return (pci_xhci_insert_event(xsc, &evtrb, 1) !=
+		    XHCI_TRB_ERROR_SUCCESS);
+		break;
+	default:
+		break;
+	}
 	return (0);
 }
 
@@ -2726,7 +2810,7 @@
 {
 	char node_name[16];
 	nvlist_t *slots_nvl, *slot_nvl;
-	char *cp, *opt, *str, *tofree;
+	char *cp, *opt, *str, *tofree, *subopt;
 	int slot;
 
 	if (opts == NULL)
@@ -2746,7 +2830,16 @@
 		snprintf(node_name, sizeof(node_name), "%d", slot);
 		slot++;
 		slot_nvl = create_relative_config_node(slots_nvl, node_name);
-		set_config_value_node(slot_nvl, "device", opt);
+		subopt = strsep(&opt, ".");
+		set_config_value_node(slot_nvl, "device", subopt);
+		subopt = strsep(&opt, ".");
+		if (subopt == NULL)
+			continue;
+		set_config_value_node(slot_nvl, "param1", subopt);
+		subopt = strsep(&opt, ".");
+		if (subopt == NULL)
+			continue;
+		set_config_value_node(slot_nvl, "param2", subopt);
 
 		/*
 		 * NB: Given that we split on commas above, the legacy
@@ -2834,9 +2927,10 @@
 		dev->hci.hci_intr = pci_xhci_dev_intr;
 		dev->hci.hci_event = pci_xhci_dev_event;
 		dev->hci.hci_speed = USB_SPEED_MAX;
+		dev->hci.hci_slot = slot;
 		dev->hci.hci_usbver = -1;
 
-		devsc = ue->ue_probe(&dev->hci, nvl);
+		devsc = ue->ue_probe(&dev->hci, slot_nvl);
 		if (devsc == NULL) {
 			free(dev);
 			goto bad;
@@ -2846,7 +2940,7 @@
 		if (dev->hci.hci_usbver == -1)
 			dev->hci.hci_usbver = ue->ue_usbver;
 
-		if (dev->hci.hci_usbver == 2) {
+		if (dev->hci.hci_usbver <= 2) {
 			if (usb2_port == sc->usb2_port_start +
 			    XHCI_MAX_DEVS / 2) {
 				WPRINTF(("pci_xhci max number of USB 2 devices "
@@ -3246,6 +3340,7 @@
 		SNAPSHOT_VAR_OR_LEAVE(dev->hci.hci_address, meta, ret, done);
 		SNAPSHOT_VAR_OR_LEAVE(dev->hci.hci_port, meta, ret, done);
 		SNAPSHOT_VAR_OR_LEAVE(dev->hci.hci_speed, meta, ret, done);
+		SNAPSHOT_VAR_OR_LEAVE(dev->hci.hci_slot, meta, ret, done);
 		SNAPSHOT_VAR_OR_LEAVE(dev->hci.hci_usbver, meta, ret, done);
 	}
 
