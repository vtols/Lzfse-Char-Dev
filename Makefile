# From "Linux Device Drivers" 3rd edition 

# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq ($(KERNELRELEASE),)
	obj-m := lzfse_cdev.o

# Otherwise we were called directly from the command
# line; invoke the kernel build system.
else
	PWD := $(shell pwd)
	KERNELDIR ?= $(PWD)/../linux

# ARCH=um is additional flag to build for User Mode Linux
# CFLAGS=... disable PIE, some gcc binaries have PIE enabled by deafault
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) ARCH=um CFLAGS='-no-pie -fno-pie -fno-PIE' modules

endif
