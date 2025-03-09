#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/input.h>

#define KBD_IRQ             1
#define KBD_DATA_REG        0x60
#define KBD_SCANCODE_MASK   0x7f
#define KBD_STATUS_MASK     0x80

// 0x11 for w
#define KBD_W_SCANCODE      0x0e   /* backspace */
// #define KBD_A_SCANCODE      0x48

static struct input_dev *keyboard_inp;

static irqreturn_t isr_for_keyboard(int irq, void *dev_id)
{
    char code_scan;

    code_scan = inb(KBD_DATA_REG);

    pr_info("Scan Code: %x %s\n", code_scan & KBD_SCANCODE_MASK, code_scan & KBD_STATUS_MASK ? "Released" : "Pressed");

    if ((code_scan & KBD_SCANCODE_MASK) == KBD_W_SCANCODE) 
    {
        if (!(code_scan & KBD_STATUS_MASK)) 
        {  
            pr_info("Remapping happening...........\n\n\n");

            input_report_key(keyboard_inp, KEY_A, 1);
            input_sync(keyboard_inp);
        } 
        else 
        {
            input_report_key(keyboard_inp, KEY_A, 0);
            input_sync(keyboard_inp);
        }

        return IRQ_HANDLED;
    }

    return IRQ_NONE;
}

static int __init init_for_keyboard(void)
{
    int ret;

    keyboard_inp = input_allocate_device();
    if (!keyboard_inp) {
        pr_err("Failed to allocate input device\n");
        return -ENOMEM;
    }

    keyboard_inp->name = "Keyboard Remapper";
    keyboard_inp->phys = "kbd/input0";
    keyboard_inp->id.bustype = BUS_USB;

    set_bit(EV_KEY, keyboard_inp->evbit);
    set_bit(KEY_A, keyboard_inp->keybit);

    ret = input_register_device(keyboard_inp);
    if (ret) {
        pr_err("Failed to register input device\n");
        input_free_device(keyboard_inp);
        return ret;
    }

    ret = request_irq(KBD_IRQ, isr_for_keyboard, IRQF_SHARED, "kbd2", (void *)isr_for_keyboard);
    if (ret) {
        pr_err("Failed to request IRQ\n");
        input_unregister_device(keyboard_inp);
        input_free_device(keyboard_inp);
        return ret;
    }

    pr_info("Keyboard IRQ handler module loaded and remapping\n");
    return 0;
}

static void __exit exit_for_keyboard(void)
{
    free_irq(KBD_IRQ, (void *)isr_for_keyboard);
    input_unregister_device(keyboard_inp);
    pr_info("Keyboard IRQ handler module unloaded\n");
}

module_init(init_for_keyboard);
module_exit(exit_for_keyboard);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ali raza");
MODULE_DESCRIPTION("key changer");