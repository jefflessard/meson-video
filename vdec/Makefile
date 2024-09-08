# SPDX-License-Identifier: GPL-2.0
# Makefile for Amlogic meson video decoder driver

# CONFIG_VIDEO_MESON_VDEC = m

meson-vdec-objs = esparser.o vdec.o vdec_helpers.o vdec_platform.o
meson-vdec-objs += vdec_1.o vdec_hevc.o
meson-vdec-objs += codec_mpeg12.o codec_h264.o codec_hevc_common.o codec_vp9.o codec_hevc.o

obj-$(CONFIG_VIDEO_MESON_VDEC) += meson-vdec.o


KDIR ?= /lib/modules/$(shell uname -r)/build

# Make targets

all: modules

debug: CCFLAGS += -g -DDEBUG
debug: modules

modules:
	$(MAKE) EXTRA_CFLAGS="$(CCFLAGS)" EXTRA_LDFLAGS="$(LDFLAGS)" -C $(KDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install

probe: modules_install
	rmmod meson_vdec || true
	dmesg -c > /dev/null
	modprobe meson_vdec

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

.PHONY: modules modules_install probe clean
