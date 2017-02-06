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

# CFLAGS=... disable PIE, some gcc binaries have PIE enabled by deafault
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) CFLAGS='-no-pie -fno-pie -fno-PIE' modules

endif
