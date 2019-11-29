obj-m :=monitor.o
KERNELDIR := ~/oreo-msm/out/android-msm-angler-3.10/private/msm-google
PWD :=$(shell pwd)
ARCH=arm64
CROSS_COMPILE=~/oreo-msm/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
LDFLAGS=-L$(KERNEL_DIR)/include/linux/
CFLAGS_MODULE=-fno-pic
all:
	make  ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	rm -rf *.c~
	rm -rf *.h~
	rm -rf *.o
	rm -f modules.order	
