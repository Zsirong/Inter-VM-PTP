#ivshmem_makefile
ifneq ($(KERNELRELEASE),)    #注意ifneq后空格
obj-m := ivshmem.o
else
KDIR := /lib/modules/$(shell uname -r)/build  #改地址
all:
	make -C $(KDIR) M=$(shell pwd) modules    #注意tab
clean:
	rm -f *.ko *.o *.mod.o *.mod.c *.symvers
endif

