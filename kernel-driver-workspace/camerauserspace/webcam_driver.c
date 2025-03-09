#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>

#define DRIVER_NAME "simple_webcam"
#define ISO_PACKET_COUNT 3             // Number of isochronous packets
#define ISO_PACKET_SIZE 3 * 1020       // Size of each isochronous packet (3x1020 bytes)
#define WEBCAM_VENDOR_ID 0x04F2        // Replace with your USB Vendor ID
#define WEBCAM_PRODUCT_ID 0xB5AE       // Replace with your USB Product ID

// USB Device ID Table
static struct usb_device_id webcam_id_table[] = {
    { USB_DEVICE(WEBCAM_VENDOR_ID, WEBCAM_PRODUCT_ID) },
    {}
};
MODULE_DEVICE_TABLE(usb, webcam_id_table);

// Webcam device structure
struct webcam_device {
    struct usb_device *udev;
    struct urb *iso_urb;
    unsigned char *iso_buffer;
};

// Completion handler for isochronous transfer
static void webcam_iso_complete(struct urb *urb)
{
    int i;

    if (urb->status) {
        pr_err("%s: URB error status: %d\n", DRIVER_NAME, urb->status);
        return;
    }

    // Process each frame from the isochronous transfer
    for (i = 0; i < ISO_PACKET_COUNT; i++) {
        pr_info("%s: Frame %d: %*ph\n", DRIVER_NAME, i,
                urb->iso_frame_desc[i].actual_length,
                urb->transfer_buffer + urb->iso_frame_desc[i].offset);
    }

    // Resubmit the URB to keep the stream alive
    if (usb_submit_urb(urb, GFP_ATOMIC)) {
        pr_err("%s: Failed to resubmit URB\n", DRIVER_NAME);
    }
}

// Probe function (called when the device is connected)
static int webcam_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    struct usb_device *udev = interface_to_usbdev(interface);
    struct webcam_device *webcam;
    int pipe, ret, i;

    // Print which interface we are binding to
    pr_info("%s: Probing interface %d\n", DRIVER_NAME, interface->cur_altsetting->desc.bInterfaceNumber);

    // Ensure we are working with interface 1
    if (interface->cur_altsetting->desc.bInterfaceNumber != 1) {
        pr_err("%s: Skipping interface %d, only interface 1 is supported\n",
               DRIVER_NAME, interface->cur_altsetting->desc.bInterfaceNumber);
        return -ENODEV;
    }

    // Detach uvcvideo or other kernel drivers
    usb_set_intfdata(interface, NULL);
    usb_driver_claim_interface(&webcam_probe, interface, NULL);

    // Set alternate setting to 11 (or your required alternate setting)
    ret = usb_set_interface(udev, 1, 11);
    if (ret) {
        pr_err("%s: Failed to set alternate setting 11, error: %d\n", DRIVER_NAME, ret);
        return ret;
    }

    // Allocate webcam device structure
    webcam = kzalloc(sizeof(struct webcam_device), GFP_KERNEL);
    if (!webcam) {
        pr_err("%s: Failed to allocate memory for webcam_device\n", DRIVER_NAME);
        return -ENOMEM;
    }

    webcam->udev = udev;

    // Allocate memory for isochronous buffer
    webcam->iso_buffer = kzalloc(ISO_PACKET_COUNT * ISO_PACKET_SIZE, GFP_KERNEL);
    if (!webcam->iso_buffer) {
        pr_err("%s: Failed to allocate isochronous buffer\n", DRIVER_NAME);
        kfree(webcam);
        return -ENOMEM;
    }

    // Allocate the URB
    webcam->iso_urb = usb_alloc_urb(ISO_PACKET_COUNT, GFP_KERNEL);
    if (!webcam->iso_urb) {
        pr_err("%s: Failed to allocate URB\n", DRIVER_NAME);
        kfree(webcam->iso_buffer);
        kfree(webcam);
        return -ENOMEM;
    }

    // Isochronous endpoint for data reception
    pipe = usb_rcvisocpipe(udev, 0x81); // Endpoint 0x81 for isochronous IN

    usb_fill_int_urb(webcam->iso_urb, udev, pipe, webcam->iso_buffer,
                     ISO_PACKET_COUNT * ISO_PACKET_SIZE, webcam_iso_complete,
                     webcam, 1);

    // Set up isochronous frame descriptors
    for (i = 0; i < ISO_PACKET_COUNT; i++) {
        webcam->iso_urb->iso_frame_desc[i].offset = i * ISO_PACKET_SIZE;
        webcam->iso_urb->iso_frame_desc[i].length = ISO_PACKET_SIZE;
    }

    // Submit the URB to start streaming
    ret = usb_submit_urb(webcam->iso_urb, GFP_KERNEL);
    if (ret) {
        pr_err("%s: Failed to submit the URB, error: %d\n", DRIVER_NAME, ret);
        usb_free_urb(webcam->iso_urb);
        kfree(webcam->iso_buffer);
        kfree(webcam);
        return ret;
    }

    // Save the webcam device pointer in the interface data
    usb_set_intfdata(interface, webcam);

    pr_info("%s: USB webcam driver successfully initialized for interface 1, alternate setting 11\n", DRIVER_NAME);
    return 0;
}

// Disconnect function (called when device is disconnected)
static void webcam_disconnect(struct usb_interface *interface)
{
    struct webcam_device *webcam = usb_get_intfdata(interface);

    if (webcam) {
        usb_kill_urb(webcam->iso_urb);
        usb_free_urb(webcam->iso_urb);
        kfree(webcam->iso_buffer);
        kfree(webcam);
        usb_set_intfdata(interface, NULL);
    }

    pr_info("%s: USB webcam driver disconnected\n", DRIVER_NAME);
}

// USB driver structure
static struct usb_driver webcam_driver = {
    .name = DRIVER_NAME,
    .id_table = webcam_id_table,
    .probe = webcam_probe,
    .disconnect = webcam_disconnect,
};

// Module initialization
static int __init webcam_init(void)
{
    pr_info("%s: Initializing USB webcam driver\n", DRIVER_NAME);
    return usb_register(&webcam_driver);
}

// Module cleanup
static void __exit webcam_exit(void)
{
    pr_info("%s: Exiting USB webcam driver\n", DRIVER_NAME);
    usb_deregister(&webcam_driver);
}

module_init(webcam_init);
module_exit(webcam_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rehan Farooq");
MODULE_DESCRIPTION("Simple USB Webcam Driver for Interface 1, Alternate Setting 11");
