--- usb_passthru.c.orig	2025-08-27 22:31:08.293726000 +0300
+++ usb_passthru.c	2025-08-27 22:31:08.294950000 +0300
@@ -0,0 +1,872 @@
+#include <sys/cdefs.h>
+#include <sys/queue.h>
+#include <sys/time.h>
+
+#include <dev/usb/usb.h>
+#include <dev/usb/usbdi.h>
+
+#include <assert.h>
+#include <libusb.h>
+#include <pthread.h>
+#include <stdlib.h>
+#include <string.h>
+
+#include "debug.h"
+
+#include "config.h"
+#include "usb_emul.h"
+
+static int usb_passthru_debug = 0;
+static bool libusb_is_init = false;
+#define	DPRINTF(params) if (usb_passthru_debug) PRINTLN params
+#define WPRINTF(params) PRINTLN params
+
+#define OUT		0
+#define IN		1
+
+struct usb_passthru_libusb_xfer;
+
+struct usb_passthru_softc {
+	struct usb_hci *hci; /* hci structure for issue xhci interrupt */
+	pthread_mutex_t mtx; /* mutex for locking */
+	unsigned long vid;   /* product vid */
+	unsigned long pid;   /* product pid */
+	struct libusb_device_handle *handle; /* libusb handler */
+	libusb_hotplug_callback_handle cb;   /* callback handler */
+	int *active_altsettings; /* alternative settings for record total number
+				    of interfaces */
+	int num_interfaces;	 /* number of interfaces */
+	uint8_t *endpoint_types[2]; /* type of endpoints[in, out] */
+	uint8_t ep_cnts[2]; /* number of endpoints for each type[in,out] */
+	LIST_ENTRY(usb_passthru_softc) next;
+};
+
+struct usb_passthru_libusb_xfer {
+	struct libusb_transfer *lusb_xfer;
+	struct usb_data_xfer *usb_xfer;
+	struct usb_passthru_softc *sc;
+	uint8_t *buffer;
+	bool in;
+	int epid;
+	int size;
+};
+
+static LIST_HEAD(, usb_passthru_softc) devices;
+
+static int usb_passthru_remove(void *scarg);
+static pthread_t libusb_thread = NULL;
+static int event_thread_exit = false;
+
+static int
+libusb_error_to_usb_error(enum libusb_error error)
+{
+	switch (error) {
+	case LIBUSB_SUCCESS:
+		return (USB_ERR_NORMAL_COMPLETION);
+	case LIBUSB_ERROR_IO:
+		return (USB_ERR_IOERROR);
+	case LIBUSB_ERROR_INVALID_PARAM:
+		return (USB_ERR_INVAL);
+	case LIBUSB_ERROR_ACCESS:
+		return (USB_ERR_INVAL);
+	case LIBUSB_ERROR_NO_DEVICE:
+		return (USB_ERR_NOT_CONFIGURED);
+	case LIBUSB_ERROR_NOT_FOUND:
+		return (USB_ERR_INVAL);
+	case LIBUSB_ERROR_BUSY:
+		return (USB_ERR_IN_USE);
+	case LIBUSB_ERROR_TIMEOUT:
+		return (USB_ERR_TIMEOUT);
+	case LIBUSB_ERROR_OVERFLOW:
+		return (USB_ERR_BAD_BUFSIZE);
+	case LIBUSB_ERROR_PIPE:
+		return (USB_ERR_NO_PIPE);
+	case LIBUSB_ERROR_INTERRUPTED:
+		return (USB_ERR_INTERRUPTED);
+	case LIBUSB_ERROR_NO_MEM:
+		return (USB_ERR_NOMEM);
+	case LIBUSB_ERROR_NOT_SUPPORTED:
+		return (USB_ERR_INVAL);
+	case LIBUSB_ERROR_OTHER:
+		return (USB_ERR_INVAL);
+	}
+}
+
+static void *
+libusb_pull_thread(void *arg __unused)
+{
+	struct timeval tv;
+	int err;
+
+	DPRINTF(("start libusb pullthread"));
+	while (!event_thread_exit) {
+		tv.tv_sec = 1;
+		tv.tv_usec = 0;
+		err = libusb_handle_events_timeout(NULL, &tv);
+		if (err && err != LIBUSB_ERROR_TIMEOUT)
+			break;
+	}
+
+	DPRINTF(("stop libusb pullthread"));
+	return (NULL);
+}
+
+static void
+usb_passthru_calculate_xfer_ptr(struct usb_data_xfer *xfer, int *head,
+    int *tail, int *size)
+{
+	int cur, i;
+	int first = -1, length = 0;
+	struct usb_data_xfer_block *blk;
+
+	for (i = 0, cur = xfer->head; i < xfer->ndata;
+	    ++i, cur = (cur + 1) % USB_MAX_XFER_BLOCKS) {
+		blk = &xfer->data[cur];
+		if (blk->processed)
+			continue;
+		switch (blk->status) {
+		case USB_NEXT_DATA:
+		case USB_LAST_DATA:
+			first = first == -1 ? cur : first;
+			length += blk->blen;
+			break;
+		case USB_NO_DATA:
+			blk->processed = 1;
+			continue;
+		}
+
+		if (blk->status == USB_LAST_DATA)
+			break;
+	}
+	*head = first;
+	*tail = xfer->tail;
+	*size = length;
+}
+
+static void
+usb_passthru_data_calculate_num_isos(struct usb_data_xfer *xfer, int *head,
+    int *nframe, int *len)
+{
+	int i, cur, first = -1, nf = 0, length = 0;
+
+	for (i = 0, cur = xfer->head; i < xfer->ndata;
+	    i = (i + 1) % USB_MAX_XFER_BLOCKS) {
+		if (xfer->data[i].processed)
+			continue;
+		switch (xfer->data[i].status) {
+		case USB_LAST_DATA:
+			++nf;
+			if (first == -1)
+				first = i;
+		/* Fallthrough */
+		case USB_NEXT_DATA:
+			length += xfer->data[i].blen;
+			break;
+		default:
+			break;
+		}
+	}
+
+	*nframe = nf;
+	*head = first;
+	*len = length;
+}
+
+static struct usb_passthru_libusb_xfer *
+usb_passthru_xfer_alloc(struct usb_passthru_softc *sc, int in,
+    struct usb_data_xfer *usb_xfer, int size, int ep, int iso)
+{
+	struct usb_passthru_libusb_xfer *xfer = calloc(1,
+	    sizeof(struct usb_passthru_libusb_xfer));
+	xfer->lusb_xfer = libusb_alloc_transfer(iso);
+	xfer->usb_xfer = usb_xfer;
+	xfer->buffer = calloc(1, size);
+	xfer->size = size;
+	xfer->in = in;
+	xfer->sc = sc;
+	xfer->epid = ep;
+	return (xfer);
+}
+
+static void
+usb_passthru_xfer_free(struct usb_passthru_libusb_xfer *xfer)
+{
+	free(xfer);
+}
+
+static int
+usb_passthru_parse_ep_types(struct usb_passthru_softc *sc)
+{
+	struct libusb_config_descriptor *desc;
+	struct libusb_device *dev;
+	struct libusb_device_handle *handle;
+	struct libusb_interface_descriptor *inf_desc;
+	struct libusb_endpoint_descriptor *ep_desc;
+	int res, intf, ep, addr, dir;
+
+	handle = sc->handle;
+	dev = libusb_get_device(handle);
+	res = libusb_get_active_config_descriptor(dev, &desc);
+	if (res != LIBUSB_SUCCESS)
+		return (libusb_error_to_usb_error(res));
+	assert(desc->bNumInterfaces == sc->num_interfaces);
+
+	free(sc->endpoint_types[0]);
+	free(sc->endpoint_types[1]);
+	sc->ep_cnts[0] = sc->ep_cnts[1] = 0;
+	for (intf = 0; intf < desc->bNumInterfaces; ++intf) {
+		inf_desc = &desc->interface[intf]
+				.altsetting[sc->active_altsettings[intf]];
+		for (ep = 0; ep < inf_desc->bNumEndpoints; ++ep) {
+			addr = inf_desc->endpoint[ep].bEndpointAddress;
+			dir = !!(addr & LIBUSB_ENDPOINT_DIR_MASK);
+			addr = addr & LIBUSB_ENDPOINT_ADDRESS_MASK;
+			sc->ep_cnts[dir] = (addr > sc->ep_cnts[dir] ?
+				addr :
+				sc->ep_cnts[dir]);
+		}
+	}
+
+	sc->endpoint_types[OUT] = calloc(sc->ep_cnts[OUT] + 1, sizeof(uint8_t));
+	sc->endpoint_types[IN] = calloc(sc->ep_cnts[IN] + 1, sizeof(uint8_t));
+	for (intf = 0; intf < desc->bNumInterfaces; ++intf) {
+		inf_desc = &desc->interface[intf]
+				.altsetting[sc->active_altsettings[intf]];
+		for (ep = 0; ep < inf_desc->bNumEndpoints; ++ep) {
+			ep_desc = &inf_desc->endpoint[ep];
+			addr = ep_desc->bEndpointAddress;
+			sc->endpoint_types[!!(addr & LIBUSB_ENDPOINT_DIR_MASK)]
+					  [addr &
+					      LIBUSB_ENDPOINT_ADDRESS_MASK] =
+			    ep_desc->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK;
+		}
+	}
+
+	libusb_free_config_descriptor(desc);
+	return (0);
+}
+
+static int
+usb_passthru_guest_attach_device(struct usb_passthru_softc *sc)
+{
+	struct libusb_config_descriptor *desc;
+	struct libusb_device *dev;
+	struct libusb_device_handle *handle;
+	int intf, res;
+
+	handle = sc->handle;
+	dev = libusb_get_device(handle);
+	res = libusb_get_active_config_descriptor(dev, &desc);
+	if (res != LIBUSB_SUCCESS)
+		return (libusb_error_to_usb_error(res));
+
+	free(sc->active_altsettings);
+	sc->num_interfaces = desc->bNumInterfaces;
+	sc->active_altsettings = calloc(sc->num_interfaces, sizeof(int));
+
+	for (intf = 0; intf < desc->bNumInterfaces; ++intf) {
+		res = libusb_detach_kernel_driver(handle, intf);
+		if (res != LIBUSB_SUCCESS)
+			break;
+		res = libusb_claim_interface(handle, intf);
+		if (res != LIBUSB_SUCCESS)
+			break;
+	}
+
+	libusb_free_config_descriptor(desc);
+	usb_passthru_parse_ep_types(sc);
+	return (res);
+}
+
+static int
+usb_passthru_guest_detach_device_on_host(struct usb_passthru_softc *sc)
+{
+	struct libusb_config_descriptor *desc;
+	struct libusb_device *dev;
+	struct libusb_device_handle *handle;
+	int intf, res;
+
+	handle = sc->handle;
+	if (handle == NULL)
+		return (libusb_error_to_usb_error(LIBUSB_SUCCESS));
+	dev = libusb_get_device(handle);
+
+	res = libusb_get_active_config_descriptor(dev, &desc);
+	if (res != LIBUSB_SUCCESS)
+		return (libusb_error_to_usb_error(res));
+
+	for (intf = 0; intf < desc->bNumInterfaces; ++intf) {
+		res = libusb_release_interface(handle, intf);
+		if (res != LIBUSB_SUCCESS)
+			break;
+		res = libusb_attach_kernel_driver(handle, intf);
+		if (res != LIBUSB_SUCCESS)
+			break;
+	}
+
+	libusb_free_config_descriptor(desc);
+	free(sc->active_altsettings);
+	free(sc->endpoint_types[0]);
+	free(sc->endpoint_types[1]);
+	sc->endpoint_types[0] = NULL;
+	sc->endpoint_types[1] = NULL;
+	sc->active_altsettings = NULL;
+
+	return (libusb_error_to_usb_error(res));
+}
+
+static int
+usb_passthru_guest_detach_device(struct usb_passthru_softc *sc __unused)
+{
+	return (0);
+}
+
+static int
+usb_passthru_hotplug_callback(struct libusb_context *ctx __unused,
+    struct libusb_device *dev, libusb_hotplug_event event, void *user_data)
+{
+	struct usb_passthru_softc *sc = user_data;
+	int err = 0;
+
+	DPRINTF(("%s: enter hotplug handler event: %d", __func__, event));
+
+	pthread_mutex_lock(&sc->mtx);
+
+	if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
+		assert(sc->hci->hci_event(sc->hci, USBDEV_REMOVE, sc) == 0);
+		libusb_close(sc->handle);
+		sc->handle = NULL;
+	} else if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
+		if (sc->handle != NULL)
+			goto done;
+		err = libusb_open(dev, &sc->handle);
+		if (err)
+			goto done;
+		usb_passthru_guest_attach_device(sc);
+		assert(sc->hci->hci_event(sc->hci, USBDEV_ATTACH, sc) == 0);
+	}
+
+done:
+	pthread_mutex_unlock(&sc->mtx);
+
+	return (err);
+}
+
+static void
+usb_passthru_data_callback(struct libusb_transfer *lusb_xfer)
+{
+	struct usb_passthru_libusb_xfer *up_xfer = lusb_xfer->user_data;
+	struct usb_data_xfer *uxfer = up_xfer->usb_xfer;
+	struct usb_passthru_softc *sc = up_xfer->sc;
+	struct usb_hci *hci = sc->hci;
+	int act_len = lusb_xfer->actual_length;
+	int head, tail, size, cur_len, cur, offset, nframe;
+	char *buffer;
+	int cur_iso = 0, i;
+
+	/*
+	 * uxfer == NULL means the guest is cancelled by guest
+	 * like machine shutdown.
+	 */
+	if (lusb_xfer->status == LIBUSB_TRANSFER_CANCELLED && uxfer == NULL) {
+		usb_passthru_xfer_free(up_xfer);
+		return;
+	}
+
+	USB_DATA_XFER_LOCK(uxfer);
+	if (lusb_xfer->type == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS)
+		usb_passthru_data_calculate_num_isos(uxfer, &head, &nframe,
+		    &size);
+	else
+		usb_passthru_calculate_xfer_ptr(uxfer, &head, &tail, &size);
+
+	if (lusb_xfer->status == LIBUSB_TRANSFER_STALL ||
+	    lusb_xfer->status == LIBUSB_TRANSFER_NO_DEVICE ||
+	    lusb_xfer->status == LIBUSB_TRANSFER_CANCELLED) {
+		USB_DATA_SET_ERRCODE(&uxfer->data[head], USB_STALL);
+		goto done;
+	}
+
+	DPRINTF(("%s: act_len: %d blen:%d in:%d epid:%d status: %d", __func__,
+	    act_len, size, up_xfer->in, up_xfer->epid, lusb_xfer->status));
+
+	pthread_mutex_lock(&up_xfer->sc->mtx);
+
+	if (up_xfer->in) {
+		if (lusb_xfer->type == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS)
+			act_len = lusb_xfer->iso_packet_desc[0].actual_length;
+
+		if (act_len > size)
+			act_len = size;
+
+		for (cur = head, offset = 0, i = 0; i < uxfer->ndata;
+		    cur = (cur + 1) % USB_MAX_XFER_BLOCKS, ++i) {
+			cur_len = uxfer->data[cur].blen;
+			if (cur_len > act_len) {
+				cur_len = act_len;
+				USB_DATA_SET_ERRCODE(&uxfer->data[cur],
+				    USB_SHORT);
+			}
+			if (lusb_xfer->type != LIBUSB_TRANSFER_TYPE_ISOCHRONOUS)
+				buffer = &lusb_xfer->buffer[offset];
+			else
+				buffer = libusb_get_iso_packet_buffer_simple(
+					     lusb_xfer, cur_iso) +
+				    offset;
+			if (cur_len)
+				memcpy(uxfer->data[cur].buf, buffer, cur_len);
+			act_len -= cur_len;
+			offset += cur_len;
+			if (act_len <= 0 &&
+			    lusb_xfer->type ==
+				LIBUSB_TRANSFER_TYPE_ISOCHRONOUS) {
+				++cur_iso;
+				act_len = lusb_xfer->iso_packet_desc[cur_iso]
+					      .actual_length;
+				offset = 0;
+			}
+			uxfer->data[cur].blen -= cur_len;
+			uxfer->data[cur].bdone = cur_len;
+			uxfer->data[cur].processed = 1;
+		}
+	} else {
+		for (cur = head; uxfer->data[cur].buf;
+		    cur = (cur + 1) % USB_MAX_XFER_BLOCKS)
+			uxfer->data[cur].processed = 1;
+
+		USB_DATA_SET_ERRCODE(&uxfer->data[head],
+		    lusb_xfer->status == LIBUSB_TRANSFER_COMPLETED ?
+			USB_ERR_NORMAL_COMPLETION :
+			USB_ERR_IOERROR);
+	}
+	pthread_mutex_unlock(&sc->mtx);
+
+done:
+	uxfer->tr_softc = NULL;
+	USB_DATA_XFER_UNLOCK(up_xfer->usb_xfer);
+	hci->hci_intr(hci, up_xfer->epid);
+	usb_passthru_xfer_free(up_xfer);
+}
+
+static void
+usb_passthru_exit(void)
+{
+	struct usb_passthru_softc *sc, *tmp;
+
+	LIST_FOREACH_SAFE(sc, &devices, next, tmp) {
+		LIST_REMOVE(sc, next);
+		usb_passthru_remove(sc);
+	}
+
+	event_thread_exit = true;
+}
+
+static void *
+usb_passthru_probe(struct usb_hci *hci, const nvlist_t *nvl)
+{
+	struct usb_passthru_softc *sc;
+	struct libusb_device_descriptor dev_desc;
+	struct libusb_device *dev;
+	struct libusb_init_option opts[] = {
+		{ .option = LIBUSB_OPTION_CAPSICUMIZE }
+	};
+	const char *param;
+	int error;
+
+	if (!libusb_is_init) {
+		libusb_is_init = true;
+		error = libusb_init_context(NULL, opts,
+		    sizeof(opts) / sizeof(struct libusb_init_option));
+		if (error) {
+			EPRINTLN("failed to capsicumize libusb");
+			return (NULL);
+		}
+		LIST_INIT(&devices);
+	}
+	sc = calloc(1, sizeof(struct usb_passthru_softc));
+
+	param = get_config_value_node(nvl, "param1");
+	if (param == NULL) {
+		free(sc);
+		return (NULL);
+	}
+	sc->vid = strtoul(param, NULL, 16);
+	param = get_config_value_node(nvl, "param2");
+	if (param == NULL) {
+		free(sc);
+		return (NULL);
+	}
+	sc->pid = strtoul(param, NULL, 16);
+
+	sc->hci = hci;
+	sc->handle = libusb_open_device_with_vid_pid(NULL, sc->vid, sc->pid);
+	if (sc->handle == NULL) {
+		EPRINTLN("failed to open usb device with %0lx %0lx", sc->vid,
+		    sc->pid);
+		free(sc);
+		return (NULL);
+	}
+	dev = libusb_get_device(sc->handle);
+	if (libusb_get_device_descriptor(dev, &dev_desc) != LIBUSB_SUCCESS) {
+		EPRINTLN("failed to get usb device descriptor with %0lx %0lx",
+		    sc->vid, sc->pid);
+		free(sc);
+		return (NULL);
+	}
+	if (dev_desc.bcdUSB >= 0x0300) {
+		hci->hci_usbver = 3;
+	} else {
+		hci->hci_usbver = 2;
+	}
+	LIST_INSERT_HEAD(&devices, sc, next);
+
+	return (sc);
+}
+
+static int
+usb_passthru_init(void *scarg)
+{
+	struct usb_passthru_softc *sc = (struct usb_passthru_softc *)scarg;
+	struct usb_hci *hci;
+	enum libusb_speed speed;
+	int error;
+
+	hci = sc->hci;
+	pthread_mutex_init(&sc->mtx, NULL);
+
+	speed = libusb_get_device_speed(libusb_get_device(sc->handle));
+	switch (speed) {
+	case LIBUSB_SPEED_LOW:
+		hci->hci_speed = 2;
+		break;
+	case LIBUSB_SPEED_FULL:
+		hci->hci_speed = 1;
+		break;
+	case LIBUSB_SPEED_HIGH:
+		hci->hci_speed = 3;
+		break;
+	default:
+		break;
+	}
+
+	if ((error = usb_passthru_guest_attach_device(sc)) != LIBUSB_SUCCESS) {
+		EPRINTLN("failed to attach device to guest %s",
+		    libusb_error_name(error));
+		goto failed;
+	}
+
+	error = libusb_hotplug_register_callback(NULL,
+	    LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
+		LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
+	    0, sc->vid, sc->pid, LIBUSB_HOTPLUG_MATCH_ANY,
+	    usb_passthru_hotplug_callback, (void *)sc, &sc->cb);
+	if (error != LIBUSB_SUCCESS) {
+		EPRINTLN("failed to create callback for usb device %s",
+		    libusb_error_name(error));
+		goto failed;
+	}
+
+	if (libusb_thread == NULL) {
+		error = pthread_create(&libusb_thread, NULL, libusb_pull_thread,
+		    NULL);
+		if (error)
+			goto failed;
+	}
+
+	atexit(usb_passthru_exit);
+	return (error);
+
+failed:
+	pthread_mutex_destroy(&sc->mtx);
+	free(sc);
+	return (1);
+}
+
+#define	UREQ(x,y)	((x) | ((y) << 8))
+
+static int
+usb_passthru_request(void *scarg, struct usb_data_xfer *xfer)
+{
+	struct usb_passthru_softc *sc = scarg;
+	struct usb_data_xfer_block *data;
+	int i, cur, err, value, index, len;
+
+	data = NULL;
+	cur = xfer->head;
+	err = USB_ERR_NORMAL_COMPLETION;
+
+	for (i = 0; i < xfer->ndata; i++) {
+		xfer->data[cur].bdone = 0;
+		if (data == NULL && USB_DATA_OK(xfer, i)) {
+			data = &xfer->data[cur];
+		}
+		xfer->data[cur].processed = 1;
+		cur = (cur + 1) % USB_MAX_XFER_BLOCKS;
+	}
+
+	if (data == NULL)
+		goto done;
+
+	if (!xfer->ureq) {
+		DPRINTF(("usb_passthru_request not found: port %d",
+		    sc->hci->hci_port));
+		goto done;
+	}
+
+	value = UGETW(xfer->ureq->wValue);
+	index = UGETW(xfer->ureq->wIndex);
+	len = UGETW(xfer->ureq->wLength);
+	DPRINTF((
+	    "usb_passthru_request: bRequest: %x bmRequestType: %x wValue: %x wIndex: %d wLength: %x",
+	    xfer->ureq->bRequest, xfer->ureq->bmRequestType, value, index,
+	    len));
+
+	pthread_mutex_lock(&sc->mtx);
+	if (sc->handle == NULL) {
+		err = USB_ERR_STALLED;
+		goto done;
+	}
+
+	switch (UREQ(xfer->ureq->bRequest, xfer->ureq->bmRequestType)) {
+	case UREQ(UR_SET_ADDRESS, UT_WRITE_DEVICE):
+		err = 0;
+		goto done;
+	case UREQ(UR_SET_CONFIG, UT_WRITE_DEVICE):
+		err = usb_passthru_guest_detach_device_on_host(sc);
+		if (err)
+			goto done;
+		err = libusb_set_configuration(sc->handle, value & 0xff);
+		if (err)
+			goto done;
+		err = usb_passthru_guest_attach_device(sc);
+		if (err)
+			goto done;
+		goto done;
+	case UREQ(UR_SET_INTERFACE, UT_WRITE_INTERFACE):
+		if (sc->num_interfaces <= index) {
+			err = -1;
+			goto done;
+		}
+		err = libusb_set_interface_alt_setting(sc->handle, index,
+		    value);
+		if (err == LIBUSB_SUCCESS)
+			sc->active_altsettings[index] = value;
+		usb_passthru_parse_ep_types(sc);
+		goto done;
+	case UREQ(UR_CLEAR_FEATURE, UT_WRITE_ENDPOINT):
+		err = libusb_clear_halt(sc->handle, index);
+		goto done;
+	}
+
+	data->blen = len;
+	data->bdone = 0;
+
+	err = libusb_control_transfer(sc->handle, xfer->ureq->bmRequestType,
+	    xfer->ureq->bRequest, UGETW(xfer->ureq->wValue),
+	    UGETW(xfer->ureq->wIndex), data->buf, data->blen, 1000);
+	if (err < 0)
+		goto done;
+	data->blen -= err;
+	data->bdone = err;
+	err = 0;
+done:
+	pthread_mutex_unlock(&sc->mtx);
+	if (xfer->ureq && (xfer->ureq->bmRequestType & UT_WRITE) &&
+	    (err == USB_ERR_NORMAL_COMPLETION) && (data != NULL))
+		data->blen = 0;
+
+	if (data)
+		DPRINTF((
+		    "usb_passthru request error code %d (0=ok), blen %u txlen %u",
+		    err, (data ? data->blen : 0), (data ? data->bdone : 0)));
+
+	return (libusb_error_to_usb_error(err));
+}
+
+static void
+usb_passthru_data_fill_xfer_iso(struct usb_data_xfer *xfer,
+    struct libusb_transfer *lusb_xfer)
+{
+	int i, cur, idx;
+
+	for (i = 0, cur = xfer->head, idx = 0; i < xfer->ndata;
+	    i = (i + 1) % USB_MAX_XFER_BLOCKS) {
+		if (xfer->data[cur].status == USB_LAST_DATA) {
+			lusb_xfer->iso_packet_desc[idx].length +=
+			    xfer->data[cur].blen;
+		} else if (xfer->data[cur].status == USB_NEXT_DATA) {
+			lusb_xfer->iso_packet_desc[idx].length +=
+			    xfer->data[cur].blen;
+			continue;
+		} else {
+			continue;
+		}
+		++idx;
+	}
+}
+
+static int
+usb_passthru_data_handler(void *scarg, struct usb_data_xfer *xfer, int dir,
+    int epctx)
+{
+	struct usb_passthru_softc *sc = scarg;
+	struct usb_passthru_libusb_xfer *up_xfer;
+	int err, cur, len, ep, head, tail, offset, ep_type, nframe;
+
+	assert(sc->ep_cnts[dir] >= epctx);
+
+	ep_type = sc->endpoint_types[dir][epctx];
+
+	err = USB_ERR_NORMAL_COMPLETION;
+	ep = (dir ? LIBUSB_ENDPOINT_IN : 0) | epctx;
+
+	if (ep_type == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS)
+		usb_passthru_data_calculate_num_isos(xfer, &head, &nframe,
+		    &len);
+	else
+		usb_passthru_calculate_xfer_ptr(xfer, &head, &tail, &len);
+
+	if (len == 0)
+		goto done;
+
+	DPRINTF((
+	    "usb_passthru handle data - DIR=%s|EP=%d, blen %d, transfer_type: %d, ndata: %d",
+	    dir ? "IN" : "OUT", epctx, len, sc->endpoint_types[dir][epctx],
+	    xfer->ndata));
+
+	pthread_mutex_lock(&sc->mtx);
+	if (sc->handle == NULL)
+		goto done;
+
+	up_xfer = usb_passthru_xfer_alloc(sc, dir, xfer, len, ep,
+	    ep_type == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS ? nframe : 0);
+
+	if (ep_type == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS)
+		usb_passthru_data_fill_xfer_iso(xfer, up_xfer->lusb_xfer);
+
+	if (!dir) {
+		for (cur = head, offset = 0; offset < len;
+		    cur = (cur + 1) % USB_MAX_XFER_BLOCKS) {
+			memcpy(&up_xfer->buffer[offset], xfer->data[cur].buf,
+			    xfer->data[cur].blen);
+			offset += xfer->data[cur].blen;
+			xfer->data[cur].bdone = xfer->data[cur].blen = 0;
+			xfer->data[cur].blen = 0;
+		}
+	}
+
+	switch (ep_type) {
+	case LIBUSB_TRANSFER_TYPE_BULK:
+		libusb_fill_bulk_transfer(up_xfer->lusb_xfer, sc->handle, ep,
+		    up_xfer->buffer, len, usb_passthru_data_callback, up_xfer,
+		    0);
+		break;
+	case LIBUSB_TRANSFER_TYPE_INTERRUPT:
+		libusb_fill_interrupt_transfer(up_xfer->lusb_xfer, sc->handle,
+		    ep, up_xfer->buffer, len, usb_passthru_data_callback,
+		    up_xfer, 0);
+		break;
+	case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
+		libusb_fill_iso_transfer(up_xfer->lusb_xfer, sc->handle, ep,
+		    up_xfer->buffer, len, nframe, usb_passthru_data_callback,
+		    up_xfer, 0);
+		break;
+	}
+	up_xfer->lusb_xfer->flags = LIBUSB_TRANSFER_FREE_TRANSFER |
+	    LIBUSB_TRANSFER_FREE_BUFFER;
+	err = libusb_submit_transfer(up_xfer->lusb_xfer);
+	if (err)
+		goto done;
+	xfer->tr_softc = up_xfer;
+done:
+	pthread_mutex_unlock(&sc->mtx);
+	return (libusb_error_to_usb_error(err));
+}
+
+static int
+usb_passthru_reset(void *scarg __unused)
+{
+	struct usb_passthru_softc *sc = scarg;
+	int err = 0;
+
+	DPRINTF(("%s", __FUNCTION__));
+
+	if (sc->handle) {
+		err = libusb_reset_device(sc->handle);
+		if (err != LIBUSB_SUCCESS)
+			goto done;
+		err = usb_passthru_guest_attach_device(sc);
+	}
+done:
+	return (libusb_error_to_usb_error(err));
+}
+
+static int
+usb_passthru_remove(void *scarg)
+{
+	struct usb_passthru_softc *sc = scarg;
+	int err;
+
+	pthread_mutex_lock(&sc->mtx);
+	err = usb_passthru_guest_detach_device(sc);
+	if (err)
+		return (err);
+	libusb_hotplug_deregister_callback(NULL, sc->cb);
+	if ((err = usb_passthru_guest_detach_device_on_host(sc)) !=
+	    LIBUSB_SUCCESS) {
+		return (err);
+	}
+	libusb_close(sc->handle);
+	pthread_mutex_unlock(&sc->mtx);
+
+	return (0);
+}
+
+static int
+usb_passthru_stop(void *scarg __unused)
+{
+	return (0);
+}
+
+static int
+usb_passthru_cancel(struct usb_data_xfer *xfer)
+{
+	struct usb_passthru_libusb_xfer *up_xfer;
+	struct usb_passthru_softc *sc;
+	int err;
+
+	up_xfer = xfer->tr_softc;
+	if (up_xfer == NULL) {
+		return (USB_ERR_INVAL);
+	}
+	up_xfer->usb_xfer = NULL;
+
+	DPRINTF(("%s", __FUNCTION__));
+
+	sc = up_xfer->sc;
+	pthread_mutex_lock(&sc->mtx);
+	err = libusb_cancel_transfer(up_xfer->lusb_xfer);
+	xfer->tr_softc = NULL;
+	pthread_mutex_unlock(&sc->mtx);
+
+	return (libusb_error_to_usb_error(err));
+}
+
+static struct usb_devemu ue_passthru = {
+	.ue_emu = "passthru",
+	.ue_usbver = 3,
+	.ue_usbspeed = USB_SPEED_HIGH,
+	.ue_probe = usb_passthru_probe,
+	.ue_init = usb_passthru_init,
+	.ue_request = usb_passthru_request,
+	.ue_data = usb_passthru_data_handler,
+	.ue_reset = usb_passthru_reset,
+	.ue_remove = usb_passthru_remove,
+	.ue_stop = usb_passthru_stop,
+	.ue_cancel = usb_passthru_cancel,
+#ifdef BHYVE_SNAPSHOT
+	.ue_snapshot = usb_passthru_snapshot,
+#endif
+};
+
+USB_EMUL_SET(ue_passthru);
