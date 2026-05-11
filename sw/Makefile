ifneq (${KERNELRELEASE},)

# KERNELRELEASE defined: being compiled as part of the kernel
	obj-m := pvz_driver.o

else

# Standalone build: kernel module + the pvz game binary.

	KERNEL_SOURCE := /usr/src/linux-headers-$(shell uname -r)
	PWD := $(shell pwd)

default: module pvz

module:
	${MAKE} -C ${KERNEL_SOURCE} SUBDIRS=${PWD} modules

clean:
	${MAKE} -C ${KERNEL_SOURCE} SUBDIRS=${PWD} clean
	${RM} pvz

# Main game executable
pvz: main.c game.c render.c input.c pvz.h game.h render.h input.h
	${CC} -Wall -O2 -o pvz main.c game.c render.c input.c -lpthread

endif
