#include <linux/videodev2.h>

#include "meson-formats.h"


const struct meson_format nv12 = {
	.description="NV12 raw pixel format",
	.pixelformat = V4L2_FMT(NV12M),
	.bits_per_px = 8,
	.num_planes = 2,
	.uvplane_bppx = 8,
	.uvplane_bppy = 4,
	.align_width = 32,
	.align_height = 16,
};

const struct meson_format yuv420 = {
	.description="YUV420 raw pixel format",
	.pixelformat = V4L2_FMT(YUV420M),
	.bits_per_px = 8,
	.num_planes = 3,
	.uvplane_bppx = 4,
	.uvplane_bppy = 4,
	.align_width = 32,
	.align_height = 16,
};

const struct meson_format mpeg1 = {
	.description="MPEG1 byte stream",
	.pixelformat = V4L2_FMT(MPEG1),
	.flags = V4L2_FMT_FLAG_COMPRESSED,
	.bits_per_px = 16,
	.num_planes = 1,
	.align_width = 32,
	.align_height = 16,
};

const struct meson_format mpeg2 = {
	.description="MPEG2 H.262 byte stream",
	.pixelformat = V4L2_FMT(MPEG2),
	.flags = V4L2_FMT_FLAG_COMPRESSED,
	.bits_per_px = 8,
	.num_planes = 1,
	.align_width = 32,
	.align_height = 16,
};

const struct meson_format h264 = {
	.description="H.264 AVC byte stream",
	.pixelformat = V4L2_FMT(H264),
	.flags = V4L2_FMT_FLAG_COMPRESSED | V4L2_FMT_FLAG_DYN_RESOLUTION,
	.bits_per_px = 8,
	.num_planes = 1,
	.align_width = 32,
	.align_height = 16,
};

const struct meson_format vp9 = {
	.description="VP9 byte stream",
	.pixelformat = V4L2_FMT(VP9),
	.flags = V4L2_FMT_FLAG_COMPRESSED | V4L2_FMT_FLAG_DYN_RESOLUTION,
	.bits_per_px = 4,
	.num_planes = 1,
	.align_width = 32,
	.align_height = 16,
};

const struct meson_format hevc = {
	.description="H.265 HEVC byte stream",
	.pixelformat = V4L2_FMT(HEVC),
	.flags = V4L2_FMT_FLAG_COMPRESSED | V4L2_FMT_FLAG_DYN_RESOLUTION,
	.bits_per_px = 4,
	.num_planes = 1,
	.align_width = 32,
	.align_height = 16,
};
