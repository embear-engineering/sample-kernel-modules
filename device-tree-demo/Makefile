# A Makefile to compile the device-tree-demo-driver.c file into a kernel module
obj-m += device-tree-demo-driver.o

# The current directory is passed to sub-makes as argument
PWD := $(shell pwd)

# The KDIR is the path to the kernel source
KDIR ?= /lib/modules/$(shell uname -r)/build

OVERLAY = device-tree-demo-overlay.dts

# The default target is to build the kernel module
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# The clean target is to remove all the generated files
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	@rm -f *.pre

# The install target is to install the kernel module
install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install

# Use gcc to preprocess the device tree file and then use dtc to compile it to a binary
dtb:
	$(CROSS_COMPILE)gcc -E -nostdinc -I $(KDIR)/include/ -I $(KDIR)/arch/arm/boot/dts/ -undef -D__DTS__  -x assembler-with-cpp $(OVERLAY) -o $(OVERLAY).pre
	dtc -@ -i $(KDIR)/include -I dts -O dtb -o $(OVERLAY:%.dts=%.dtbo) $(OVERLAY).pre
