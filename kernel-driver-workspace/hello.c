#include <linux/module.h>
#include<linux/init.h>
#include<linux/kernel.h>

static int __init my_init(void)
{
	printk(KERN_INFO "Hello-hello,kernel!!!\n");
	return 0;
}


static void __exit my_exit(void)
{

printk("hello,Goodbye kernel!\n");
}
module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");



