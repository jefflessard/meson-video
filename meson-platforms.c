#include <linux/clk.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/types.h>

#include "meson-formats.h"
#include "meson-codecs.h"
#include "meson-platforms.h"
#include "meson-vcodec.h"

#include "regs/hhi_regs.h"
#include "regs/ao_regs.h"
#include "regs/dos_regs.h"

/* AO_RTI_GEN_PWR_SLEEP0 */
#define DOS_WAVE420L_D1_PWR_OFF		BIT(25)
#define DOS_WAVE420L_PWR_OFF		BIT(24)
#define DOS_WAVE420L_PWR		(DOS_WAVE420L_D1_PWR_OFF | DOS_WAVE420L_PWR_OFF)
#define DOS_HEVC_D1_PWR_OFF		BIT(7)
#define DOS_HEVC_PWR_OFF		BIT(6)
#define DOS_HEVC_PWR			(DOS_HEVC_D1_PWR_OFF | DOS_HEVC_PWR_OFF)
#define DOS_VDEC2_D1_PWR_OFF		BIT(5)
#define DOS_VDEC2_PWR_OFF		BIT(4)
#define DOS_VDEC2_PWR			(DOS_VDEC2_D1_PWR_OFF | DOS_VDEC2_PWR_OFF)
#define DOS_VDEC1_D1_PWR_OFF		BIT(3)
#define DOS_VDEC1_PWR_OFF		BIT(2)
#define DOS_VDEC1_PWR			(DOS_VDEC1_D1_PWR_OFF | DOS_VDEC1_PWR_OFF)
#define DOS_HCODEC_D1_PWR_OFF		BIT(1)
#define DOS_HCODEC_PWR_OFF		BIT(0)
#define DOS_HCODEC_PWR			(DOS_HCODEC_D1_PWR_OFF | DOS_HCODEC_PWR_OFF)

/* AO_RTI_GEN_PWR_ISO0 */
#define DOS_WAVE420L_ISO_OUT_EN		BIT(13)
#define DOS_WAVE420L_ISO_IN_EN		BIT(12)
#define DOS_WAVE420L_ISO		(DOS_WAVE420L_ISO_OUT_EN | DOS_WAVE420L_ISO_IN_EN)
#define DOS_HEVC_ISO_OUT_EN		BIT(11)
#define DOS_HEVC_ISO_IN_EN		BIT(10)
#define DOS_HEVC_ISO			(DOS_HEVC_ISO_OUT_EN | DOS_HEVC_ISO_IN_EN)
#define DOS_VDEC2_ISO_OUT_EN		BIT( 9)	
#define DOS_VDEC2_ISO_IN_EN		BIT( 8)	
#define DOS_VDEC2_ISO			(DOS_VDEC2_ISO_OUT_EN | DOS_VDEC2_ISO_IN_EN)
#define DOS_VDEC1_ISO_OUT_EN		BIT( 7)	
#define DOS_VDEC1_ISO_IN_EN		BIT( 6)	
#define DOS_VDEC1_ISO			(DOS_VDEC1_ISO_OUT_EN | DOS_VDEC1_ISO_IN_EN)
#define DOS_HCODEC_ISO_OUT_EN		BIT( 5)
#define DOS_HCODEC_ISO_IN_EN		BIT( 4)	
#define DOS_HCODEC_ISO			(DOS_HCODEC_ISO_OUT_EN | DOS_HCODEC_ISO_IN_EN)
#define GPU_ISO_OUT_EN			BIT( 3)
#define GPU_ISO_IN_EN			BIT( 2)
#define GPU_ISO				(GPU_ISO_OUT_EN | GPU_ISO_IN_EN)

/* DOS_MEM_PD_VDEC */
#define DOS_MEM_PD_VDEC_PRE_ARB		GENMASK(17,16)
#define DOS_MEM_PD_VDEC_PIC_DC		GENMASK(15,14)
#define DOS_MEM_PD_VDEC_PSCALE		GENMASK(13,12)
#define DOS_MEM_PD_VDEC_MCRCC		GENMASK(11,10)
#define DOS_MEM_PD_VDEC_DBLK		GENMASK( 9, 8)
#define DOS_MEM_PD_VDEC_MC		GENMASK( 7, 6)
#define DOS_MEM_PD_VDEC_IQIDCT		GENMASK( 5, 4)
#define DOS_MEM_PD_VDEC_VLD		GENMASK( 3, 2)
#define DOS_MEM_PD_VDEC_VCPU		GENMASK( 1, 0)
#define DOS_MEM_PD_VDEC_ALL		GENMASK(31, 0)

/* DOS_MEM_PD_HCODEC */
#define DOS_MEM_PD_HCODEC_PRE_ARB	GENMASK(17,16)
#define DOS_MEM_PD_HCODEC_PIC_DC	GENMASK(15,14)
#define DOS_MEM_PD_HCODEC_MFDIN		GENMASK(13,12)
#define DOS_MEM_PD_HCODEC_MCRCC		GENMASK(11,10)
#define DOS_MEM_PD_HCODEC_DBLK		GENMASK( 9, 8)
#define DOS_MEM_PD_HCODEC_MC		GENMASK( 7, 6)
#define DOS_MEM_PD_HCODEC_QDCT		GENMASK( 5, 4)
#define DOS_MEM_PD_HCODEC_VLC		GENMASK( 3, 2)
#define DOS_MEM_PD_HCODEC_VCPU		GENMASK( 1, 0)
#define DOS_MEM_PD_HCODEC_ALL		GENMASK(31, 0)

/* DOS_MEM_PD_HEVC */
#define DOS_MEM_PD_HEVC_DDR		GENMASK(15,14)
#define DOS_MEM_PD_HEVC_SAO		GENMASK(13,12)
#define DOS_MEM_PD_HEVC_DBLK		GENMASK(11,10)
#define DOS_MEM_PD_HEVC_IPP		GENMASK( 9, 8)
#define DOS_MEM_PD_HEVC_MPRED		GENMASK( 7, 6)
#define DOS_MEM_PD_HEVC_IQIT		GENMASK( 5, 4)
#define DOS_MEM_PD_HEVC_PARSER		GENMASK( 3, 2)
#define DOS_MEM_PD_HEVC_VCPU		GENMASK( 1, 0)
#define DOS_MEM_PD_HEVC_ALL		GENMASK(31, 0)

/* DOS_MEM_PD_WAVE420L */
#define DOS_MEM_PD_WAVE420L_VCORE	GENMASK(31, 2)
#define DOS_MEM_PD_WAVE420L_VCPU	GENMASK( 1, 0)
#define DOS_MEM_PD_WAVE420L_ALL		GENMASK(31, 0)

/* DOS_GCLK_EN0 */
#define DOS_GCLK_EN0_VDEC1		GENMASK( 9, 0)
#define DOS_GCLK_EN0_HCODEC		GENMASK(26,12)

/* DOS_GCLK_EN1 */
#define DOS_GCLK_EN1_VDEC2		GENMASK( 9, 0)

/* DOS_GCLK_EN3 */
#define DOS_GCLK_EN3_HEVC		GENMASK(31, 0)

struct meson_dos_pwrc_domain {
	char *name;
	unsigned int sleep_reg;
	unsigned int sleep_mask;
	unsigned int iso_reg;
	unsigned int iso_mask;
	unsigned int gate_reg;
	unsigned int gate_mask;
	unsigned int mem_reg;
	unsigned int mem_mask;
};

#define DOS_PDG(__name, __sleep_mask, __iso_mask, __mem_reg, __mem_mask, __gate_reg, __gate_mask) \
{ \
	.name = __name, \
	.sleep_reg = AO_RTI_GEN_PWR_SLEEP0, \
	.sleep_mask = __sleep_mask, \
	.iso_reg = AO_RTI_GEN_PWR_ISO0, \
	.iso_mask = __iso_mask, \
	.gate_reg = __gate_reg, \
	.gate_mask = __gate_mask, \
	.mem_reg = __mem_reg, \
	.mem_mask = __mem_mask, \
}

#define DOS_PD(__name, __sleep_mask, __iso_mask, __mem_reg, __mem_mask) \
	DOS_PDG(__name, __sleep_mask, __iso_mask, __mem_reg, __mem_mask, 0, 0)

/* Helper macros for iteration */

#define EXPAND_ARGS(...) __VA_ARGS__
#define ARGS_EXPANDER(FN, ...) FN(__VA_ARGS__)
#define EXPAND_FN(FN, ARGS, NARG) ARGS_EXPANDER(FN, EXPAND_ARGS ARGS, NARG)

#define FOREACH_0(FN, ARGS, ...) 
#define FOREACH_1(FN, ARGS, E, ...)  EXPAND_FN(FN, ARGS, E) 
#define FOREACH_2(FN, ARGS, E, ...)  EXPAND_FN(FN, ARGS, E) FOREACH_1(FN, ARGS, __VA_ARGS__)
#define FOREACH_3(FN, ARGS, E, ...)  EXPAND_FN(FN, ARGS, E) FOREACH_2(FN, ARGS, __VA_ARGS__)
#define FOREACH_4(FN, ARGS, E, ...)  EXPAND_FN(FN, ARGS, E) FOREACH_3(FN, ARGS, __VA_ARGS__)
#define FOREACH_5(FN, ARGS, E, ...)  EXPAND_FN(FN, ARGS, E) FOREACH_4(FN, ARGS, __VA_ARGS__)
#define FOREACH_6(FN, ARGS, E, ...)  EXPAND_FN(FN, ARGS, E) FOREACH_5(FN, ARGS, __VA_ARGS__)
#define FOREACH_7(FN, ARGS, E, ...)  EXPAND_FN(FN, ARGS, E) FOREACH_6(FN, ARGS, __VA_ARGS__)
#define FOREACH_8(FN, ARGS, E, ...)  EXPAND_FN(FN, ARGS, E) FOREACH_7(FN, ARGS, __VA_ARGS__)
#define FOREACH_9(FN, ARGS, E, ...)  EXPAND_FN(FN, ARGS, E) FOREACH_8(FN, ARGS, __VA_ARGS__)

#define FOREACH__(FN, ARGS, NARGS, ...) FOREACH_##NARGS(FN, ARGS, __VA_ARGS__) 
#define FOREACH_(FN, ARGS, NARGS, ...) FOREACH__(FN, ARGS, NARGS, __VA_ARGS__)
#define FOREACH(FN, ARGS, ...) FOREACH_(FN, ARGS, COUNT_ARGS(__VA_ARGS__), __VA_ARGS__)


/* Helper codec formats macros */

#define ENCODER(__name, __max_width, __max_height, ...) \
	FOREACH(ENCODER_ITEM, (__name, __max_width, __max_height), __VA_ARGS__)

#define ENCODER_ITEM(__name, __max_width, __max_height, __format) \
	{ \
		.src_fmt = &__format, \
		.dst_fmt = &__name, \
		.int_fmt = NULL, \
		.decoder = NULL, \
		.encoder = &__name##_encoder, \
		.max_width = __max_width, \
		.max_height = __max_height, \
	},

#define DECODER(__name, __max_width, __max_height, ...) \
	FOREACH(DECODER_ITEM,(__name, __max_width, __max_height), __VA_ARGS__)

#define DECODER_ITEM(__name, __max_width, __max_height, __format) \
	{ \
		.src_fmt = &__name, \
		.dst_fmt = &__format, \
		.int_fmt = NULL, \
		.decoder = &__name##_decoder, \
		.encoder = NULL, \
		.max_width = __max_width, \
		.max_height = __max_height, \
	},

#define TRANSCODER(__encoder, __format, __max_width, __max_height, ...) \
	FOREACH(TRANSCODER_ITEM,(__encoder, __format, __max_width, __max_height), __VA_ARGS__)

#define TRANSCODER_ITEM(__encoder, __format, __max_width, __max_height, __decoder) \
	{ \
		.src_fmt = &__decoder, \
		.dst_fmt = &__encoder, \
		.int_fmt = &__format, \
		.decoder = &__decoder##_decoder, \
		.encoder = &__encoder##_encoder, \
		.max_width = __max_width, \
		.max_height = __max_height, \
	},


/* helper functions */

int meson_vcodec_reset(struct meson_vcodec_core *core, enum meson_vcodec_reset index) {
	int ret;

	ret = reset_control_reset(core->resets[index]);
	if (ret < 0)
		return ret;

	return 0;
}

void meson_vcodec_clk_unprepare(struct meson_vcodec_core *core, enum meson_vcodec_clk index) {
	clk_disable_unprepare(core->clks[index]);
}


int meson_vcodec_clk_prepare(struct meson_vcodec_core *core, enum meson_vcodec_clk index, u64 rate) {
	int ret;

	if (rate) {
		ret = clk_set_rate(core->clks[index], rate);
		if (ret < 0)
			return ret;
	}

	ret = clk_prepare_enable(core->clks[index]);
	if (ret < 0)
		return ret;

	return 0;
}

int meson_vcodec_pwrc_on(struct meson_vcodec_core *core, enum meson_vcodec_pwrc index) {
	const struct meson_dos_pwrc_domain *pd;
	int ret;

	pd = &core->platform_specs->pwrc[index];

	/* Clear sleep state */
	if (pd->sleep_reg) {
		ret = regmap_clear_bits(core->regmaps[BUS_AO],
					AO_REMAP(pd->sleep_reg),
					pd->sleep_mask);
		if (ret < 0)
			return ret;
	}

	/* Enable DOS gates */
	if (pd->gate_reg) {
		ret = regmap_set_bits(core->regmaps[BUS_DOS],
				      VREG_REMAP(pd->gate_reg),
				      pd->gate_mask);
		if (ret < 0)
			return ret;
	}

	/* Power up memories */
	if (pd->mem_reg) {
		ret = regmap_clear_bits(core->regmaps[BUS_DOS],
					VREG_REMAP(pd->mem_reg),
					pd->mem_mask);
		if (ret < 0)
			return ret;
	}

	/* Remove isolation */
	if (pd->iso_reg) {
		ret = regmap_clear_bits(core->regmaps[BUS_AO],
					AO_REMAP(pd->iso_reg),
					pd->iso_mask);
		if (ret < 0)
			return ret;
	}

	return 0;
}

int meson_vcodec_pwrc_off(struct meson_vcodec_core *core, enum meson_vcodec_pwrc index) {
	const struct meson_dos_pwrc_domain *pd;
	int ret;

	pd = &core->platform_specs->pwrc[index];

	/* Enable isolation */
	if (pd->iso_reg) {
		ret = regmap_set_bits(core->regmaps[BUS_AO],
				      AO_REMAP(pd->iso_reg),
				      pd->iso_mask);
		if (ret < 0)
			return ret;
	}

	/* Power down memories */
	if (pd->mem_reg) {
		ret = regmap_set_bits(core->regmaps[BUS_DOS],
				      VREG_REMAP(pd->mem_reg),
				      pd->mem_mask);
		if (ret < 0)
			return ret;
	}

	/* Disable DOS gates */
	if (pd->gate_reg) {
		ret = regmap_clear_bits(core->regmaps[BUS_DOS],
					VREG_REMAP(pd->gate_reg),
					pd->gate_mask);
		if (ret < 0)
			return ret;
	}

	/* Set sleep state */
	if (pd->sleep_reg) {
		ret = regmap_set_bits(core->regmaps[BUS_AO],
				      AO_REMAP(pd->sleep_reg),
				      pd->sleep_mask);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/* platform specific specs */

static struct meson_dos_pwrc_domain gx_pwrc_domains[] = {
	[PWRC_VDEC1] = DOS_PDG("VDEC1",
		DOS_VDEC1_PWR, DOS_VDEC1_ISO,
		DOS_MEM_PD_VDEC, DOS_MEM_PD_VDEC_ALL,
		DOS_GCLK_EN0, DOS_GCLK_EN0_VDEC1),
	[PWRC_HEVC] = DOS_PDG("HEVC",
		DOS_HEVC_PWR, DOS_HEVC_ISO,
		DOS_MEM_PD_HEVC, DOS_MEM_PD_HEVC_ALL,
		DOS_GCLK_EN3, DOS_GCLK_EN3_HEVC),
	[PWRC_HCODEC] = DOS_PDG("HCODEC",
		DOS_HCODEC_PWR, DOS_HCODEC_ISO,
		DOS_MEM_PD_HCODEC, DOS_MEM_PD_HCODEC_ALL,
		DOS_GCLK_EN0, DOS_GCLK_EN0_HCODEC),
};

#if 0
static struct meson_dos_pwrc_domain gxm_pwrc_domains[] = {
	[PWRC_VDEC1] = DOS_PDG("VDEC1",
		DOS_VDEC1_PWR, DOS_VDEC1_ISO,
		DOS_MEM_PD_VDEC, DOS_MEM_PD_VDEC_ALL,
		DOS_GCLK_EN0, DOS_GCLK_EN0_VDEC1),
	[PWRC_HEVC] = DOS_PDG("HEVC",
		DOS_HEVC_PWR, DOS_HEVC_ISO,
		DOS_MEM_PD_HEVC, DOS_MEM_PD_HEVC_ALL,
		DOS_GCLK_EN3, DOS_GCLK_EN3_HEVC),
	[PWRC_HCODEC] = DOS_PDG("HCODEC",
		DOS_HCODEC_PWR, DOS_HCODEC_ISO,
		DOS_MEM_PD_HCODEC, DOS_MEM_PD_HCODEC_ALL,
		DOS_GCLK_EN0, DOS_GCLK_EN0_HCODEC),
	[PWRC_WAVE420L] = DOS_PD("WAVE420L",
		DOS_WAVE420L_PWR, DOS_WAVE420L_ISO,
		DOS_MEM_PD_WAVE420L, DOS_MEM_PD_WAVE420L_ALL),
};

static struct meson_ee_pwrc_domain_desc sm1_pwrc_domains[] = {
	[PWRC_VDEC1] = DOS_PDG("VDEC1",
		BIT(1), BIT(1),
		DOS_MEM_PD_VDEC, DOS_MEM_PD_VDEC_ALL,
		DOS_GCLK_EN0, DOS_GCLK_EN0_VDEC1),
	[PWRC_HEVC] = DOS_PDG("HEVC",
		BIT(2), BIT(2),
		DOS_MEM_PD_HEVC, DOS_MEM_PD_HEVC_ALL,
		DOS_GCLK_EN3, DOS_GCLK_EN3_HEVC),
	[PWRC_HCODEC] = DOS_PDG("HCODEC",
		BIT(0), BIT(0),
		DOS_MEM_PD_HCODEC, DOS_MEM_PD_HCODEC_ALL,
		DOS_GCLK_EN0, DOS_GCLK_EN0_HCODEC),
	[PWRC_WAVE420L] = DOS_PD("WAVE420L",
		BIT(3), BIT(3),
		DOS_MEM_PD_WAVE420L, DOS_MEM_PD_WAVE420L_ALL),
};
#endif

static const struct meson_codec_formats gxl_codecs[] = {
	DECODER(mpeg1, 1920, 1080, nv12)
	DECODER(mpeg2, 1920, 1080, nv12)
	DECODER(h264,  3840, 2160, nv12, nv21)
	DECODER(vp9,   3840, 2160, nv12)
	DECODER(hevc,  3840, 2160, nv12)

	ENCODER(h264,  1920, 1280, nv12, yuv420)
	
	// TRANSCODER(h264, nv12, 1920, 1280, mpeg1, mpeg2, h264, vp9, hevc)
	// TRANSCODER(hevc, nv12, 1920, 1280, mpeg1, mpeg2, h264, vp9, hevc)
};

const struct meson_platform_specs gxl_platform_specs = {
	.platform_id = AM_MESON_CPU_MAJOR_ID_GXL,
	.pwrc = gx_pwrc_domains,
	.codecs = gxl_codecs,
	.num_codecs = ARRAY_SIZE(gxl_codecs),
	.firmwares = {
		[MPEG1_DECODER] = "meson/vdec/gxl_mpeg12_multi.bin",
		[MPEG2_DECODER] = "meson/vdec/gxl_mpeg12_multi.bin",
		[H264_DECODER] = "meson/vdec/gxl_h264_multi.bin",
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
