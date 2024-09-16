#include <linux/types.h>

#include "meson-formats.h"
#include "meson-codecs.h"
#include "meson-platforms.h"

static const struct meson_ee_pwrc_top_domain gx_pwrc[MAX_PWRC] = {
	[PWRC_VDEC] = GX_EE_PD(
			BIT(2) | BIT(3),
			BIT(6) | BIT(7)),
	[PWRC_HEVC] = GX_EE_PD(
			BIT(6) | BIT(7),
			BIT(6) | BIT(7)),
	[PWRC_HCODEC] = GX_EE_PD(
			BIT(0) | BIT(1),
			BIT(4) | BIT(5)),
};

static const struct meson_ee_pwrc_top_domain sm1_pwrc[MAX_PWRC] = {
	[PWRC_VDEC] = SM1_EE_PD(1),
	[PWRC_HEVC] = SM1_EE_PD(2),
	[PWRC_HCODEC] = SM1_EE_PD(1),
};

static const struct meson_codec_formats gxl_codecs[] = {
	DECODER(mpeg1, 1920, 1080, nv12, yuv420)
	DECODER(mpeg2, 1920, 1080, nv12, yuv420)
	DECODER(h264, 3840, 2160, nv12)
	DECODER(vp9, 3840, 2160, nv12)
	DECODER(hevc, 3840, 2160, nv12)

	ENCODER(h264, 1920, 1280, nv12, yuv420)
	// ENCODER(hevc, 1920, 1280, nv12, yuv420)
	
	// TRANSCODER(h264, nv12, 1920, 1280, mpeg1, mpeg2, h264, vp9, hevc)
	// TRANSCODER(hevc, nv12, 1920, 1280, mpeg1, mpeg2, h264, vp9, hevc)
};

const struct meson_platform_specs gxl_platform_specs = {
	.platform_id = AM_MESON_CPU_MAJOR_ID_GXL,
	.pwrc = gx_pwrc,
	.codecs = gxl_codecs,
	.num_codecs = ARRAY_SIZE(gxl_codecs),
	.firmwares = {
		[MPEG1_DECODER] = "meson/vdec/gxl_mpeg12.bin",
		[MPEG2_DECODER] = "meson/vdec/gxl_mpeg12.bin",
		[H264_DECODER] = "meson/vdec/gxl_h264.bin",
		[VP9_DECODER] = "meson/vdec/gxl_vp9.bin",
		[HEVC_DECODER] = "meson/vdec/gxl_hevc.bin",
		[H264_ENCODER] = "meson/vdec/gxl_h264_enc.bin",
		//[HEVC_ENCODER] = NULL,
	},
};

MODULE_FIRMWARE("meson/vdec/gxl_mpeg12.bin");
MODULE_FIRMWARE("meson/vdec/gxl_h264.bin");
MODULE_FIRMWARE("meson/vdec/gxl_vp9.bin");
MODULE_FIRMWARE("meson/vdec/gxl_hevc.bin");
MODULE_FIRMWARE("meson/vdec/gxl_h264_enc.bin");
