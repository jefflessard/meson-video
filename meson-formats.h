#ifndef _MESON_FORMATS_H
#define _MESON_FORMATS_H

#include <linux/videodev2.h>

#define MAX_NUM_PLANES VIDEO_MAX_PLANES

enum meson_formats {
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
	const u32 num_planes;
	const u8 plane_size_denums[MAX_NUM_PLANES];
	const u8 plane_line_denums[MAX_NUM_PLANES];
	const u32 flags;
};

const struct meson_format nv12 = {
	.description="NV12 raw pixel format",
	.pixelformat = V4L2_PIX_FMT_NV12M,
	.num_planes = 2,
	.plane_size_denums = {1, 2},
	.plane_line_denums = {1, 1},
};

const struct meson_format yuv420 = {
	.description="YUV420 raw pixel format",
	.pixelformat = V4L2_PIX_FMT_YUV420M,
	.num_planes = 3,
	.plane_size_denums = {1, 4, 4},
	.plane_line_denums = {1, 2, 2},
};

const struct meson_format mpeg1 = {
	.description="MPEG1 byte stream",
	.pixelformat = V4L2_PIX_FMT_MPEG1,
	.num_planes = 1,
	.plane_size_denums = {1},
	.plane_line_denums = {0},
	.flags = V4L2_FMT_FLAG_COMPRESSED,
};

const struct meson_format mpeg2 = {
	.description="MPEG2 H.262 byte stream",
	.pixelformat = V4L2_PIX_FMT_MPEG2,
	.num_planes = 1,
	.plane_size_denums = {2},
	.plane_line_denums = {0},
	.flags = V4L2_FMT_FLAG_COMPRESSED,
};

const struct meson_format h264 = {
	.description="H.264 AVC byte stream",
	.pixelformat = V4L2_PIX_FMT_H264,
	.num_planes = 1,
	.plane_size_denums = {4},
	.plane_line_denums = {0},
	.flags = V4L2_FMT_FLAG_COMPRESSED | V4L2_FMT_FLAG_DYN_RESOLUTION,
};

const struct meson_format vp9 = {
	.description="VP9 byte stream",
	.pixelformat = V4L2_PIX_FMT_VP9,
	.num_planes = 1,
	.plane_size_denums = {8},
	.plane_line_denums = {0},
	.flags = V4L2_FMT_FLAG_COMPRESSED | V4L2_FMT_FLAG_DYN_RESOLUTION,
};

const struct meson_format hevc = {
	.description="H.265 HEVC byte stream",
	.pixelformat = V4L2_PIX_FMT_HEVC,
	.num_planes = 1,
	.plane_size_denums = {8},
	.plane_line_denums = {0},
	.flags = V4L2_FMT_FLAG_COMPRESSED | V4L2_FMT_FLAG_DYN_RESOLUTION,
};

#endif
