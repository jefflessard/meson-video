#ifndef _MESON_FORMATS_H
#define _MESON_FORMATS_H

#include <linux/videodev2.h>

#define MAX_NUM_PLANES VIDEO_MAX_PLANES

enum mesont_formats {
	FORMAT_NV12,
	FORMAT_YUV420M,
	FORMAT_H264,
	FORMAT_HEVC,
	MAX_FORMATS
};

struct meson_format {
	const u32 pixelformat;
	const char *description;
	const u32 num_planes;
	const u8 plane_size_denums[MAX_NUM_PLANES];
	const u8 plane_line_denums[MAX_NUM_PLANES];
	const u32 flags;
};

const struct meson_format nv12 = {
	.description="NV12 raw pixel format",
	.pixelformat = V4L2_PIX_FMT_NV12,
	.num_planes = 2,
	.plane_size_denums = {1, 2},
	.plane_line_denums = {1, 1},
};

const struct meson_format yuv420m = {
	.description="YUV420M raw pixel format",
	.pixelformat = V4L2_PIX_FMT_YUV420M,
	.num_planes = 3,
	.plane_size_denums = {1, 4, 4},
	.plane_line_denums = {1, 2, 2},
};

const struct meson_format h264 = {
	.description="H.264 AVC compressed format",
	.pixelformat = V4L2_PIX_FMT_H264,
	.num_planes = 1,
	.plane_size_denums = {2},
	.plane_line_denums = {0},
	.flags = V4L2_FMT_FLAG_COMPRESSED | V4L2_FMT_FLAG_DYN_RESOLUTION,
};

const struct meson_format hevc = {
	.description="H.265 HEVC compressed format",
	.pixelformat = V4L2_PIX_FMT_HEVC,
	.num_planes = 1,
	.plane_size_denums = {4},
	.plane_line_denums = {0},
	.flags = V4L2_FMT_FLAG_COMPRESSED | V4L2_FMT_FLAG_DYN_RESOLUTION,
};

#endif
