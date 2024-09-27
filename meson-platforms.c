#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/types.h>

#include "meson-formats.h"
#include "meson-codecs.h"
#include "meson-platforms.h"
#include "meson-vcodec.h"

#include "clk/gxbb.h"


/* BEGIN taken from meson-ee-pwrc.c */

/* AO Offsets */
#define GX_AO_RTI_GEN_PWR_SLEEP0	(0x3a << 2)
	#define DOS_HEVC _D1_PWR_OFF	BIT(7)
	#define DOS_HEVC_PWR_OFF		BIT(6)
	#define DOS_VDEC2_D1_PWR_OFF	BIT(5)
	#define DOS_VDEC2_PWR_OFF		BIT(4)
	#define DOS_VDEC1_D1_PWR_OFF	BIT(3)
	#define DOS_VDEC1_PWR_OFF		BIT(2)
	#define DOS_HCODEC_D1_PWR_OFF	BIT(1)
	#define DOS_HCODEC_PWR_OFF		BIT(0)

#define GX_AO_RTI_GEN_PWR_ISO0		(0x3b << 2)
	#define DOS_HEVC_OUT_EN			BIT(11)
	#define DOS_HEVC_IN_EN		    BIT(10)
	#define DOS_VDEC2_ISO_OUT_EN	BIT( 9)	
	#define DOS_VDEC2_ISO_IN_EN		BIT( 8)	
	#define DOS_VDEC1_ISO_OUT_EN	BIT( 7)	
	#define DOS_VDEC1_ISO_IN_EN		BIT( 6)	
	#define DOS_HCODEC_ISO_OUT_EN	BIT( 5)
	#define DOS_HCODEC_ISO_IN_EN	BIT( 4)	
	#define GPU_ISO_OUT_EN			BIT( 3)
	#define GPU_ISO_IN_EN			BIT( 2)

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
	int ret;

	ret = regmap_update_bits(core->regmaps[BUS_AO],
			core->platform_specs->pwrc[index].sleep_reg,
			core->platform_specs->pwrc[index].sleep_mask, 0);
	if (ret < 0)
		return ret;

	ret = regmap_update_bits(core->regmaps[BUS_AO],
			core->platform_specs->pwrc[index].iso_reg,
			core->platform_specs->pwrc[index].iso_mask, 0);
	if (ret < 0)
		return ret;

	return 0;
}

int meson_vcodec_pwrc_off(struct meson_vcodec_core *core, enum meson_vcodec_pwrc index) {
	int ret;

	ret = regmap_update_bits(core->regmaps[BUS_AO],
			core->platform_specs->pwrc[index].sleep_reg,
			core->platform_specs->pwrc[index].sleep_mask,
			core->platform_specs->pwrc[index].sleep_mask);
	if (ret < 0)
		return ret;

	ret = regmap_update_bits(core->regmaps[BUS_AO],
			core->platform_specs->pwrc[index].iso_reg,
			core->platform_specs->pwrc[index].iso_mask,
			core->platform_specs->pwrc[index].iso_mask);
	if (ret < 0)
		return ret;

	return 0;
}


/* platform functions */

int meson_platform_register_clks(struct meson_vcodec_core *core) {
	struct device *dev = core->dev;
	struct regmap *map = core->regmaps[BUS_HHI];
	const struct meson_eeclkc_data *data = core->platform_specs->clks;
	int ret, i;

	if (!data) {
		return 0;
	}

	for (i = 0; i < data->regmap_clk_num; i++)
		data->regmap_clks[i]->map = map;

	for (i = 0; i < data->hw_clks.num; i++) {
		/* array might be sparse */
		if (!data->hw_clks.hws[i])
			continue;

		dev_dbg(dev,"Registering clock %s\n",  data->hw_clks.hws[i]->init->name);
		ret = devm_clk_hw_register(dev, data->hw_clks.hws[i]);
		if (ret) {
			dev_err(dev, "Clock registration failed\n");
			return ret;
		}
	}

	return 0;
}


/* platform specific specs */

static struct clk_regmap gxbb_vdec_hcodec_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VDEC_CLK_CNTL,
		.mask = 0x3,
		.shift = 25,
		.flags = CLK_MUX_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vdec_hcodec_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char*[]) {
			"fclk_div4",
			"fclk_div3",
			"fclk_div5",
			"fclk_div7",
		},
		.num_parents = 4,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap gxbb_vdec_hcodec_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VDEC_CLK_CNTL,
		.shift = 16,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vdec_hcodec_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&gxbb_vdec_hcodec_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap gxbb_vdec_hcodec = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VDEC_CLK_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdec_hcodec",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&gxbb_vdec_hcodec_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_hw *gxbb_hw_clks[] = {
	&gxbb_vdec_hcodec_sel.hw,
	&gxbb_vdec_hcodec_div.hw,
	&gxbb_vdec_hcodec.hw,
};

static struct clk_regmap *const gxbb_clk_regmaps[] = {
	&gxbb_vdec_hcodec_sel,
	&gxbb_vdec_hcodec_div,
	&gxbb_vdec_hcodec,
};

static const struct meson_eeclkc_data gxbb_clkc_data = {
	.regmap_clks = gxbb_clk_regmaps,
	.regmap_clk_num = ARRAY_SIZE(gxbb_clk_regmaps),
	.hw_clks = {
		.hws = gxbb_hw_clks,
		.num = ARRAY_SIZE(gxbb_hw_clks),
	},
};

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
	DECODER(h264,  3840, 2160, nv12)
	DECODER(vp9,   3840, 2160, nv12)
	DECODER(hevc,  3840, 2160, nv12)

	ENCODER(h264,  1920, 1280, nv12, yuv420)
	// ENCODER(hevc, 1920, 1280, nv12, yuv420)
	
	// TRANSCODER(h264, nv12, 1920, 1280, mpeg1, mpeg2, h264, vp9, hevc)
	// TRANSCODER(hevc, nv12, 1920, 1280, mpeg1, mpeg2, h264, vp9, hevc)
};

const struct meson_platform_specs gxl_platform_specs = {
	.platform_id = AM_MESON_CPU_MAJOR_ID_GXL,
	.clks = &gxbb_clkc_data,
	.hwclks = {
		[CLK_HCODEC] = &gxbb_vdec_hcodec.hw,
	},
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
