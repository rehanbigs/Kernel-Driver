# SPDX-License-Identifier: GPL-2.0

# Module name
obj-m := uvc_driver.o

# Kernel build directory
KDIR := /usr/src/linux-headers-$(shell uname -r)


# Target: Build the kernel module
all:
	make -C $(KDIR) M=$(PWD) modules

# Target: Clean up generated files
clean:
	make -C $(KDIR) M=$(PWD) clean

