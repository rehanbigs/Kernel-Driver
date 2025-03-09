#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/keyboard.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple keyboard driver as an LKM");
MODULE_VERSION("1.0");

// Callback function for key press events
static int key_notifier_callback(struct notifier_block *nb, unsigned long action, void *data) {
    struct keyboard_notifier_param *param = data;

    // Check for key press or release
    if (action == KBD_KEYCODE) {
        printk(KERN_INFO "Key %s: %d\n", param->down ? "pressed" : "released", param->value);
    }
    return NOTIFY_OK;
}

// Notifier block structure
static struct notifier_block keyboard_notifier = {
    .notifier_call = key_notifier_callback,
};

// Module initialization function
static int __init keyboard_driver_init(void) {
    printk(KERN_INFO "Keyboard driver loaded.\n");
    register_keyboard_notifier(&keyboard_notifier); // Register the notifier
    return 0;
}

// Module cleanup function
static void __exit keyboard_driver_exit(void) {
    unregister_keyboard_notifier(&keyboard_notifier); // Unregister the notifier
    printk(KERN_INFO "Keyboard driver unloaded.\n");
}

module_init(keyboard_driver_init);
module_exit(keyboard_driver_exit);
