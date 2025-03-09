#include <linux/module.h>
#include <linux/usb.h>

// USB Device ID Table
static struct usb_device_id webcam_id_table[] = {
    { USB_DEVICE(0x04F2, 0xB5AE) }, // Replace with your Vendor and Product ID
    {} // Terminates the list
};
MODULE_DEVICE_TABLE(usb, webcam_id_table);

// Probe Function (Called when device is connected)
static int webcam_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    pr_info("USB Webcam (%04X:%04X) plugged in\n", id->idVendor, id->idProduct);
    return 0; // Return 0 to indicate success
}

// Disconnect Function (Called when device is disconnected)
static void webcam_disconnect(struct usb_interface *interface)
{
    pr_info("USB Webcam disconnected\n");
}

// USB Driver Structure
static struct usb_driver webcam_driver = {
    .name = "basic_webcam_driver",
    .id_table = webcam_id_table,
    .probe = webcam_probe,
    .disconnect = webcam_disconnect,
};

// Module Initialization
static int __init webcam_init(void)
{
    printk("Initializing USB Webcam Driver\n");
    return usb_register(&webcam_driver); // Register the USB driver
}

// Module Exit
static void __exit webcam_exit(void)
{
    pr_info("Exiting USB Webcam Driver\n");
    usb_deregister(&webcam_driver); // Deregister the USB driver
}

module_init(webcam_init);
module_exit(webcam_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Basic USB Webcam Driver");

