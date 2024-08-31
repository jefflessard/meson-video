obj-m += vdec/
# obj-m += vpu.o

# Path to the kernel source tree
KDIR ?= /lib/modules/$(shell uname -r)/build


# Make targets

all: modules

debug: CCFLAGS += -g -DDEBUG
debug: module

modules:
	$(MAKE) EXTRA_CFLAGS="$(CCFLAGS)" -C $(KDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod -a

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

extractor:
	$(CC) firmware-extractor.c -o firmware-extractor -lz

.PHONY: modules modules_install clean
