#ifndef _MESON_PLATFORMS_H
#define _MESON_PLATFORMS_H

#include "meson-formats.h"
#include "meson-codecs.h"

enum meson_platform_id {
	MESON_MAJOR_ID_M8B = 0x1B,
	MESON_MAJOR_ID_GXBB = 0x1F,
	MESON_MAJOR_ID_GXTVBB,
	MESON_MAJOR_ID_GXL,
	MESON_MAJOR_ID_GXM,
	MESON_MAJOR_ID_TXL,
	MESON_MAJOR_ID_TXLX,
	MESON_MAJOR_ID_AXG,
	MESON_MAJOR_ID_GXLX,
	MESON_MAJOR_ID_TXHD,
	MESON_MAJOR_ID_G12A,
	MESON_MAJOR_ID_G12B,
	MESON_MAJOR_ID_SM1 = 0x2B,
	MESON_MAJOR_ID_TL1 = 0x2E,
	MESON_MAJOR_ID_TM2,
	MESON_MAJOR_ID_C1,
	MESON_MAJOR_ID_SC2 = 0x32,
	MESON_MAJOR_ID_T5 = 0x34,
	MESON_MAJOR_ID_T5D = 0x35,
	MESON_MAJOR_ID_T7 = 0x36,
	MESON_MAJOR_ID_S4 = 0x37,
	MESON_MAJOR_ID_T3 = 0x38,
	MESON_MAJOR_ID_S4D = 0x3a,
	MESON_MAJOR_ID_T5W = 0x3b,
	MESON_MAJOR_ID_S5 = 0x3e,
	MESON_MAJOR_ID_UNKNOWN,
};

enum meson_vcodec_pwrc {
	PWRC_VDEC1,
	PWRC_HEVC,
	PWRC_HCODEC,
	MAX_PWRC
};

struct meson_ee_pwrc_top_domain;

struct meson_platform_specs {
	const enum meson_platform_id platform_id;
	const struct meson_ee_pwrc_top_domain *pwrc;
	const struct meson_codec_formats *codecs;
	const u32 num_codecs;
};


/* BEGIN taken from meson-ee-pwrc.c */

/* AO Offsets */
#define GX_AO_RTI_GEN_PWR_SLEEP0	(0x3a << 2)
#define GX_AO_RTI_GEN_PWR_ISO0		(0x3b << 2)

struct meson_ee_pwrc_top_domain {
	unsigned int sleep_reg;
	unsigned int sleep_mask;
	unsigned int iso_reg;
	unsigned int iso_mask;
};

#define SM1_EE_PD(__bit)				\
{							\
	.sleep_reg = GX_AO_RTI_GEN_PWR_SLEEP0, 		\
	.sleep_mask = BIT(__bit), 			\
	.iso_reg = GX_AO_RTI_GEN_PWR_ISO0, 		\
	.iso_mask = BIT(__bit), 			\
}

/* END taken from meson-ee-pwrc.c */

#define GX_EE_PD(__sleep_mask, __iso_mask)		\
{							\
	.sleep_reg = GX_AO_RTI_GEN_PWR_SLEEP0, 		\
	.sleep_mask = __sleep_mask, 			\
	.iso_reg = GX_AO_RTI_GEN_PWR_ISO0, 		\
	.iso_mask = __iso_mask, 			\
}

static const struct meson_ee_pwrc_top_domain gx_pwrc[MAX_PWRC] = {
	[PWRC_VDEC1] = GX_EE_PD(
			BIT(2) | BIT(3),
			BIT(6) | BIT(7)),
	[PWRC_HEVC] = GX_EE_PD(
			BIT(6) | BIT(7),
			BIT(6) | BIT(7)),
	[PWRC_HCODEC] = GX_EE_PD(
			BIT(3),
			BIT(4) | BIT(5)),
};

static const struct meson_ee_pwrc_top_domain sm1_pwrc[MAX_PWRC] = {
	[PWRC_VDEC1] = SM1_EE_PD(1),
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
	
	TRANSCODER(h264, nv12, 1920, 1280, mpeg1, mpeg2, h264, vp9, hevc)
	// TRANSCODER(hevc, nv12, 1920, 1280, mpeg1, mpeg2, h264, vp9, hevc)
};

const struct meson_platform_specs gxl_platform_specs = {
	.platform_id = MESON_MAJOR_ID_GXL,
	.pwrc = gx_pwrc,
	.codecs = gxl_codecs,
	.num_codecs = ARRAY_SIZE(gxl_codecs),
};


#endif
