# obj-m += vdec/
obj-m += meson_vcodec.o
meson_vcodec-objs += \
	meson-formats.o \
	meson-codecs.o \
	meson-vdec-adapter.o \
	vdec/codec_mpeg12.o \
	vdec/codec_h264.o \
	vdec/codec_hevc_common.o \
	vdec/codec_vp9.o \
	vdec/codec_hevc.o \
	vdec/esparser.o \
	vdec/vdec_helpers.o \
	vdec/vdec_1.o \
	vdec/vdec_hevc.o \
	amlogic.o \
	amlvenc_h264.o \
	amlvenc_h264_info.o \
	encoder_h264.o \
	meson-platforms.o \
	meson-vcodec.o

# Path to the kernel source tree
KDIR ?= /lib/modules/$(shell uname -r)/build

SRC_FILES := $(patsubst %.o, %.c, $(meson_vcodec-objs))

# Make targets

all: modules

debug: CCFLAGS += -g -DDEBUG
debug: modules

modules:
	$(MAKE) EXTRA_CFLAGS="$(CCFLAGS)" EXTRA_LDFLAGS="$(LDFLAGS)" -C $(KDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install

# Preprocess target
preprocess: $(patsubst %.c, %.i, $(SRC_FILES))

%.i: %.c
	$(MAKE) -C $(KDIR) M=$(PWD) EXTRA_CFLAGS="$(CCFLAGS) -E" $@

probe: modules_install
	rmmod meson_vcodec || true
	dmesg -c > /dev/null
	modprobe meson_vcodec

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

extractor:
	$(CC) firmware-extractor.c -o firmware-extractor -lz

.PHONY: modules modules_install probe clean
