#ifndef _MESON_FORMATS_H
#define _MESON_FORMATS_H

#include <linux/types.h>
#include <linux/videodev2.h>

#define MAX_NUM_PLANES VIDEO_MAX_PLANES

enum meson_formats: u8 {
	FORMAT_NV12,
	FORMAT_YUV420,
	FORMAT_MPEG1,
	FORMAT_MPEG2,
	FORMAT_H264,
	FORMAT_VP9,
	FORMAT_HEVC,
	MAX_FORMATS
};

struct meson_format {
	const u32 pixelformat;
	const char *description;
	const u8 num_planes;
	const u8 bits_per_px[MAX_NUM_PLANES];
	const u8 align_bits;
	const u32 flags;
};

extern const struct meson_format nv12;
extern const struct meson_format yuv420;
extern const struct meson_format mpeg1;
extern const struct meson_format mpeg2;
extern const struct meson_format h264;
extern const struct meson_format vp9;
extern const struct meson_format hevc;

#endif
