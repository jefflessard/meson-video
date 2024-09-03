#ifndef _MESON_FORMATS_H
#define _MESON_FORMATS_H

#include <linux/videodev2.h>

#define MAX_NUM_PLANES 4

struct meson_format {
	const u32 pixelformat;
	const char *description;
	const u32 num_planes;
	const u8 plane_size_denums[MAX_NUM_PLANES];
};

const struct meson_format nv12 = {
	.description="NV12 raw pixel format",
	.pixelformat = V4L2_PIX_FMT_NV12,
	.num_planes = 2,
	.plane_size_denums = {1, 2},
};

const struct meson_format yuv420m = {
	.description="YUV420M raw pixel format",
	.pixelformat = V4L2_PIX_FMT_YUV420M,
	.num_planes = 3,
	.plane_size_denums = {1, 2, 4},
};

const struct meson_format h264 = {
	.description="H.264 AVC compressed format",
	.pixelformat = V4L2_PIX_FMT_H264,
	.num_planes = 1,
	.plane_size_denums = {1},
};

const struct meson_format hevc = {
	.description="H.265 HEVC compressed format",
	.pixelformat = V4L2_PIX_FMT_HEVC,
	.num_planes = 1,
	.plane_size_denums = {1},
};

#endif
