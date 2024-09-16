#include <linux/clk.h>
#include <linux/types.h>

#include "meson-formats.h"
#include "meson-codecs.h"
#include "meson-platforms.h"
#include "meson-vcodec.h"

#include "clk/gxbb.h"

int meson_platform_register_clks(struct meson_vcodec_core *core) {
	struct device *dev = core->dev;
	struct regmap *map = core->regmaps[BUS_HHI];
	const struct meson_eeclkc_data *data = core->platform_specs->clks;
	int ret, i;

	for (i = 0; i < data->regmap_clk_num; i++)
		data->regmap_clks[i]->map = map;

	for (i = 0; i < data->hw_clks.num; i++) {
		/* array might be sparse */
		if (!data->hw_clks.hws[i])
			continue;

		ret = devm_clk_hw_register(dev, data->hw_clks.hws[i]);
		if (ret) {
			dev_err(dev, "Clock registration failed\n");
			return ret;
		}
	}

	return devm_of_clk_add_hw_provider(dev, meson_clk_hw_get, (void *)&data->hw_clks);

}

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
	.clks = &gxbb_clkc_data,
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
