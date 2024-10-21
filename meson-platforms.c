#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/types.h>

#include "meson-formats.h"
#include "meson-codecs.h"
#include "meson-platforms.h"
#include "meson-vcodec.h"

#include "regs/hhi_regs.h"
#include "regs/ao_regs.h"
#include "regs/dos_regs.h"

// AO_RTI_GEN_PWR_SLEEP0
	#define DOS_WAVE420L_D1_PWR_OFF	BIT(25)
	#define DOS_WAVE420L_PWR_OFF	BIT(24)
	#define DOS_WAVE420L_PWR		(DOS_WAVE420L_D1_PWR_OFF | DOS_WAVE420L_PWR_OFF)
	#define DOS_HEVC_D1_PWR_OFF		BIT(7)
	#define DOS_HEVC_PWR_OFF		BIT(6)
	#define DOS_HEVC_PWR			(DOS_HEVC_D1_PWR_OFF | DOS_HEVC_PWR_OFF)
	#define DOS_VDEC2_D1_PWR_OFF	BIT(5)
	#define DOS_VDEC2_PWR_OFF		BIT(4)
	#define DOS_VDEC2_PWR			(DOS_VDEC2_D1_PWR_OFF | DOS_VDEC2_PWR_OFF)
	#define DOS_VDEC1_D1_PWR_OFF	BIT(3)
	#define DOS_VDEC1_PWR_OFF		BIT(2)
	#define DOS_VDEC1_PWR			(DOS_VDEC1_D1_PWR_OFF | DOS_VDEC1_PWR_OFF)
	#define DOS_HCODEC_D1_PWR_OFF	BIT(1)
	#define DOS_HCODEC_PWR_OFF		BIT(0)
	#define DOS_HCODEC_PWR			(DOS_HCODEC_D1_PWR_OFF | DOS_HCODEC_PWR_OFF)

// AO_RTI_GEN_PWR_ISO0
	#define DOS_WAVE420L_ISO_OUT_EN	BIT(13)
	#define DOS_WAVE420L_ISO_IN_EN	BIT(12)
	#define DOS_WAVE420L_ISO		(DOS_WAVE420L_ISO_OUT_EN | DOS_WAVE420L_ISO_IN_EN)
	#define DOS_HEVC_ISO_OUT_EN		BIT(11)
	#define DOS_HEVC_ISO_IN_EN		BIT(10)
	#define DOS_HEVC_ISO		    (DOS_HEVC_ISO_OUT_EN | DOS_HEVC_ISO_IN_EN)
	#define DOS_VDEC2_ISO_OUT_EN	BIT( 9)	
	#define DOS_VDEC2_ISO_IN_EN		BIT( 8)	
	#define DOS_VDEC2_ISO		    (DOS_VDEC2_ISO_OUT_EN | DOS_VDEC2_ISO_IN_EN)
	#define DOS_VDEC1_ISO_OUT_EN	BIT( 7)	
	#define DOS_VDEC1_ISO_IN_EN		BIT( 6)	
	#define DOS_VDEC1_ISO		    (DOS_VDEC1_ISO_OUT_EN | DOS_VDEC1_ISO_IN_EN)
	#define DOS_HCODEC_ISO_OUT_EN	BIT( 5)
	#define DOS_HCODEC_ISO_IN_EN	BIT( 4)	
	#define DOS_HCODEC_ISO		    (DOS_HCODEC_ISO_OUT_EN | DOS_HCODEC_ISO_IN_EN)
	#define GPU_ISO_OUT_EN			BIT( 3)
	#define GPU_ISO_IN_EN			BIT( 2)
	#define GPU_ISO				    (GPU_ISO_OUT_EN | GPU_ISO_IN_EN)

// DOS_MEM_PD_VDEC
	#define DOS_MEM_PD_VDEC_PRE_ARB GENMASK(17,16)
	#define DOS_MEM_PD_VDEC_PIC_DC  GENMASK(15,14)
	#define DOS_MEM_PD_VDEC_PSCALE  GENMASK(13,12)
	#define DOS_MEM_PD_VDEC_MCRCC   GENMASK(11,10)
	#define DOS_MEM_PD_VDEC_DBLK    GENMASK( 9, 8)
	#define DOS_MEM_PD_VDEC_MC      GENMASK( 7, 6)
	#define DOS_MEM_PD_VDEC_IQIDCT  GENMASK( 5, 4)
	#define DOS_MEM_PD_VDEC_VLD     GENMASK( 3, 2)
	#define DOS_MEM_PD_VDEC_VCPU    GENMASK( 1, 0)
	#define DOS_MEM_PD_VDEC_ALL     GENMASK(31, 0)

// DOS_MEM_PD_HCODEC
	#define DOS_MEM_PD_HCODEC_PRE_ARB GENMASK(17,16)
	#define DOS_MEM_PD_HCODEC_PIC_DC  GENMASK(15,14)
	#define DOS_MEM_PD_HCODEC_MFDIN   GENMASK(13,12)
	#define DOS_MEM_PD_HCODEC_MCRCC   GENMASK(11,10)
	#define DOS_MEM_PD_HCODEC_DBLK    GENMASK( 9, 8)
	#define DOS_MEM_PD_HCODEC_MC      GENMASK( 7, 6)
	#define DOS_MEM_PD_HCODEC_QDCT    GENMASK( 5, 4)
	#define DOS_MEM_PD_HCODEC_VLC     GENMASK( 3, 2)
	#define DOS_MEM_PD_HCODEC_VCPU    GENMASK( 1, 0)
	#define DOS_MEM_PD_HCODEC_ALL     GENMASK(31, 0)

// DOS_MEM_PD_HEVC
	#define DOS_MEM_PD_HEVC_DDR     GENMASK(15,14)
	#define DOS_MEM_PD_HEVC_SAO     GENMASK(13,12)
	#define DOS_MEM_PD_HEVC_DBLK    GENMASK(11,10)
	#define DOS_MEM_PD_HEVC_IPP     GENMASK( 9, 8)
	#define DOS_MEM_PD_HEVC_MPRED   GENMASK( 7, 6)
	#define DOS_MEM_PD_HEVC_IQIT    GENMASK( 5, 4)
	#define DOS_MEM_PD_HEVC_PARSER  GENMASK( 3, 2)
	#define DOS_MEM_PD_HEVC_VCPU    GENMASK( 1, 0)
	#define DOS_MEM_PD_HEVC_ALL     GENMASK(31, 0)

// DOS_MEM_PD_WAVE420L
	#define DOS_MEM_PD_WAVE420L_VCORE   GENMASK(31, 2)
	#define DOS_MEM_PD_WAVE420L_VCPU    GENMASK( 1, 0)
	#define DOS_MEM_PD_WAVE420L_ALL     GENMASK(31, 0)

// DOS_GCLK_EN0
	#define DOS_GCLK_EN0_VDEC1      GENMASK( 9, 0)
	#define DOS_GCLK_EN0_HCODEC     GENMASK(26,12)

// DOS_GCLK_EN1
	#define DOS_GCLK_EN1_VDEC2      GENMASK( 9, 0)

// DOS_GCLK_EN3
	#define DOS_GCLK_EN3_HEVC       GENMASK(31, 0)

/* DOS clock gating */

struct clk_regmap_dos_gate {
	u32  offset;
	u32  mask;
	u8   flags;
};

static inline struct clk_regmap_dos_gate* clk_get_regmap_dos_gate(struct clk_regmap *clk)
{
	return (struct clk_regmap_dos_gate*)clk->data;
}

static int clk_regmap_dos_gate_endisable(struct clk_hw *hw, bool enable)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct clk_regmap_dos_gate *gate = clk_get_regmap_dos_gate(clk);
	int set = gate->flags & CLK_GATE_SET_TO_DISABLE ? 1 : 0;

	set ^= enable;

	return regmap_update_bits(
			clk->map,
			gate->offset,
			gate->mask,
			set ? gate->mask : 0);
}

static int clk_regmap_dos_gate_enable(struct clk_hw *hw)
{
	return clk_regmap_dos_gate_endisable(hw, true);
}

static void clk_regmap_dos_gate_disable(struct clk_hw *hw)
{
	clk_regmap_dos_gate_endisable(hw, false);
}

static int clk_regmap_dos_gate_is_enabled(struct clk_hw *hw)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct clk_regmap_dos_gate *gate = clk_get_regmap_dos_gate(clk);
	unsigned int val;

	regmap_read(
			clk->map,
			gate->offset,
			&val);
	if (gate->flags & CLK_GATE_SET_TO_DISABLE)
		val ^= gate->mask;

	val &= gate->mask;

	return val ? 1 : 0;
}

const struct clk_ops clk_regmap_dos_gate_ops = {
	.enable = clk_regmap_dos_gate_enable,
	.disable = clk_regmap_dos_gate_disable,
	.is_enabled = clk_regmap_dos_gate_is_enabled,
};
EXPORT_SYMBOL_NS_GPL(clk_regmap_dos_gate_ops, CLK_MESON);

const struct clk_ops clk_regmap_dos_gate_ro_ops = {
	.is_enabled = clk_regmap_dos_gate_is_enabled,
};
EXPORT_SYMBOL_NS_GPL(clk_regmap_dos_gate_ro_ops, CLK_MESON);


/* BEGIN taken from meson-ee-pwrc.c */

struct meson_ee_pwrc_mem_domain {
	unsigned int reg;
	unsigned int mask;
};

struct meson_ee_pwrc_top_domain {
	unsigned int sleep_reg;
	unsigned int sleep_mask;
	unsigned int iso_reg;
	unsigned int iso_mask;
};

struct meson_ee_pwrc_domain_desc {
	char *name;
	struct meson_ee_pwrc_top_domain *top_pd;
	unsigned int mem_pd_count;
	struct meson_ee_pwrc_mem_domain *mem_pd;
};

#define SM1_EE_PD(__bit) \
{ \
	.sleep_reg = AO_RTI_GEN_PWR_SLEEP0, \
	.sleep_mask = BIT(__bit), \
	.iso_reg = AO_RTI_GEN_PWR_ISO0, \
	.iso_mask = BIT(__bit), \
}

/* END taken from meson-ee-pwrc.c */

#define GX_EE_PD(__sleep_mask, __iso_mask) \
{ \
	.sleep_reg = AO_RTI_GEN_PWR_SLEEP0, \
	.sleep_mask = __sleep_mask, \
	.iso_reg = AO_RTI_GEN_PWR_ISO0, \
	.iso_mask = __iso_mask, \
}

#define DOS_PD(__name, __top_pd, __mem_reg, __mem_mask) \
{ \
	.name = __name, \
	.top_pd = &(struct meson_ee_pwrc_top_domain) __top_pd, \
	.mem_pd_count = 1, \
	.mem_pd = \
		&(struct meson_ee_pwrc_mem_domain) { \
			.reg = __mem_reg, \
			.mask = __mem_mask, \
		}, \
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
	const struct meson_ee_pwrc_domain_desc *pd;
	int ret, i;

	pd = &core->platform_specs->pwrc[index];

	if (pd->top_pd) {
		ret = regmap_clear_bits(core->regmaps[BUS_AO],
				AO_REMAP(pd->top_pd->sleep_reg),
				pd->top_pd->sleep_mask);
		if (ret < 0)
			return ret;
	}

	for (i = 0 ; i < pd->mem_pd_count ; ++i) {
		ret = regmap_clear_bits(core->regmaps[BUS_DOS],
				   VREG_REMAP(pd->mem_pd[i].reg),
				   pd->mem_pd[i].mask);
		if (ret < 0)
			return ret;
	}

	if (pd->top_pd) {
		ret = regmap_clear_bits(core->regmaps[BUS_AO],
				AO_REMAP(pd->top_pd->iso_reg),
				pd->top_pd->iso_mask);
		if (ret < 0)
			return ret;
	}

	return 0;
}

int meson_vcodec_pwrc_off(struct meson_vcodec_core *core, enum meson_vcodec_pwrc index) {
	const struct meson_ee_pwrc_domain_desc *pd;
	int ret, i;

	pd = &core->platform_specs->pwrc[index];

	if (pd->top_pd) {
		ret = regmap_set_bits(core->regmaps[BUS_AO],
				AO_REMAP(pd->top_pd->sleep_reg),
				pd->top_pd->sleep_mask);
		if (ret < 0)
			return ret;
	}

	for (i = 0 ; i < pd->mem_pd_count ; ++i) {
		ret = regmap_set_bits(core->regmaps[BUS_DOS],
				   VREG_REMAP(pd->mem_pd[i].reg),
				   pd->mem_pd[i].mask);
		if (ret < 0)
			return ret;
	}

	if (pd->top_pd) {
		ret = regmap_set_bits(core->regmaps[BUS_AO],
				AO_REMAP(pd->top_pd->iso_reg),
				pd->top_pd->iso_mask);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/* platform functions */

int meson_platform_register_clks(struct meson_vcodec_core *core) {
	struct device *dev = core->dev;
	struct regmap *hhi = core->regmaps[BUS_HHI];
	struct regmap *dos = core->regmaps[BUS_DOS];
	const struct meson_eeclkc_data *data = core->platform_specs->clks;
	int ret, i;

	if (!data) {
		return 0;
	}

	for (i = 0; i < data->regmap_clk_num; i++) {
		if (strstr(data->hw_clks.hws[i]->init->name, "dos_gate")) {
  			data->regmap_clks[i]->map = dos;
		} else {
			data->regmap_clks[i]->map = hhi;
		}
	}

	for (i = 0; i < data->hw_clks.num; i++) {
		/* array might be sparse */
		if (!data->hw_clks.hws[i])
			continue;

		dev_dbg(dev,"Registering clock %s (%s)\n",  data->hw_clks.hws[i]->init->name, data->regmap_clks[i]->map == dos ? "DOS" : "HHI");
		ret = devm_clk_hw_register(dev, data->hw_clks.hws[i]);
		if (ret) {
			dev_err(dev, "Clock registration failed\n");
			return ret;
		}
	}

	return 0;
}


/* platform specific specs */

static struct clk_regmap gxbb_dos_gate_vdec1 = {
	.data = &(struct clk_regmap_dos_gate){
		.offset = VREG_REMAP(DOS_GCLK_EN0),
		.mask = DOS_GCLK_EN0_VDEC1,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dos_gate_vdec1",
		.ops = &clk_regmap_dos_gate_ops,
		.parent_names = (const char*[]) {
			"vdec_1",
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_NO_REPARENT,
	},
};

static struct clk_regmap gxbb_dos_gate_hevc = {
	.data = &(struct clk_regmap_dos_gate){
		.offset = VREG_REMAP(DOS_GCLK_EN3),
		.mask = DOS_GCLK_EN3_HEVC,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dos_gate_hevc",
		.ops = &clk_regmap_dos_gate_ops,
		.parent_names = (const char*[]) {
			"vdec_hevc",
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_NO_REPARENT,
	},
};

static struct clk_regmap gxbb_vdec_hcodec_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_REMAP(HHI_VDEC_CLK_CNTL),
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
		.offset = HHI_REMAP(HHI_VDEC_CLK_CNTL),
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
		.offset = HHI_REMAP(HHI_VDEC_CLK_CNTL),
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdec_hcodec",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&gxbb_vdec_hcodec_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap gxbb_dos_gate_hcodec = {
	.data = &(struct clk_regmap_dos_gate){
		.offset = VREG_REMAP(DOS_GCLK_EN0),
		.mask = DOS_GCLK_EN0_HCODEC,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dos_gate_hcodec",
		.ops = &clk_regmap_dos_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&gxbb_vdec_hcodec.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_NO_REPARENT,
	},
};

static struct clk_regmap gxbb_wave420l_0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_REMAP(HHI_WAVE420L_CLK_CNTL),
		.mask = 0x4,
		.shift = 9,
		.flags = CLK_MUX_ROUND_CLOSEST | CLK_MUX_INDEX_ONE,
	},
	.hw.init = &(struct clk_init_data){
		.name = "wave420l_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char*[]) {
			"fclk_div4",
			"fclk_div3",
			"fclk_div5",
			"fclk_div7",
		},
		.num_parents = 4,
		.flags = CLK_SET_RATE_NO_REPARENT,
	},
};

static struct clk_regmap gxbb_wave420l_0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_REMAP(HHI_WAVE420L_CLK_CNTL),
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "wave420l_0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&gxbb_wave420l_0_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap gxbb_wave420l_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_REMAP(HHI_WAVE420L_CLK_CNTL),
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "wave420l_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&gxbb_wave420l_0_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap gxbb_wave420l_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_REMAP(HHI_WAVE420L_CLK_CNTL),
		.mask = 0x4,
		.shift = 25,
		.flags = CLK_MUX_ROUND_CLOSEST | CLK_MUX_INDEX_ONE,
	},
	.hw.init = &(struct clk_init_data){
		.name = "wave420l_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char*[]) {
			"fclk_div4",
			"fclk_div3",
			"fclk_div5",
			"fclk_div7",
		},
		.num_parents = 4,
		.flags = CLK_SET_RATE_NO_REPARENT,
	},
};

static struct clk_regmap gxbb_wave420l_1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_REMAP(HHI_WAVE420L_CLK_CNTL),
		.shift = 16,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "wave420l_1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&gxbb_wave420l_1_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap gxbb_wave420l_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_REMAP(HHI_WAVE420L_CLK_CNTL),
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "wave420l_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&gxbb_wave420l_1_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap gxbb_wave420l = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_REMAP(HHI_WAVE420L_CLK_CNTL),
		.mask = 1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "wave420l",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&gxbb_wave420l_0.hw,
			&gxbb_wave420l_1.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_NO_REPARENT,
	},
};

static struct clk_hw *gxbb_hw_clks[] = {
	&gxbb_dos_gate_vdec1.hw,
	&gxbb_dos_gate_hevc.hw,
	&gxbb_vdec_hcodec_sel.hw,
	&gxbb_vdec_hcodec_div.hw,
	&gxbb_vdec_hcodec.hw,
	&gxbb_dos_gate_hcodec.hw,
	&gxbb_wave420l_0_sel.hw,
	&gxbb_wave420l_0_div.hw,
	&gxbb_wave420l_0.hw,
	&gxbb_wave420l_1_sel.hw,
	&gxbb_wave420l_1_div.hw,
	&gxbb_wave420l_1.hw,
	&gxbb_wave420l.hw,
};

static struct clk_regmap *const gxbb_clk_regmaps[] = {
	&gxbb_dos_gate_vdec1,
	&gxbb_dos_gate_hevc,
	&gxbb_vdec_hcodec_sel,
	&gxbb_vdec_hcodec_div,
	&gxbb_vdec_hcodec,
	&gxbb_dos_gate_hcodec,
	&gxbb_wave420l_0_sel,
	&gxbb_wave420l_0_div,
	&gxbb_wave420l_0,
	&gxbb_wave420l_1_sel,
	&gxbb_wave420l_1_div,
	&gxbb_wave420l_1,
	&gxbb_wave420l,
};

static const struct meson_eeclkc_data gxbb_clkc_data = {
	.regmap_clks = gxbb_clk_regmaps,
	.regmap_clk_num = ARRAY_SIZE(gxbb_clk_regmaps),
	.hw_clks = {
		.hws = gxbb_hw_clks,
		.num = ARRAY_SIZE(gxbb_hw_clks),
	},
};

static struct meson_ee_pwrc_domain_desc gx_pwrc_domains[] = {
	[PWRC_VDEC1] = DOS_PD("VDEC1",
		GX_EE_PD(DOS_VDEC1_PWR, DOS_VDEC1_ISO),
		DOS_MEM_PD_VDEC, DOS_MEM_PD_VDEC_ALL),
	[PWRC_HEVC] = DOS_PD("HEVC",
		GX_EE_PD(DOS_HEVC_PWR, DOS_HEVC_ISO),
		DOS_MEM_PD_HEVC, DOS_MEM_PD_HEVC_ALL),
	[PWRC_HCODEC] = DOS_PD("HCODEC",
		GX_EE_PD(DOS_HCODEC_PWR, DOS_HCODEC_ISO),
		DOS_MEM_PD_HCODEC, DOS_MEM_PD_HCODEC_ALL),
	[PWRC_WAVE420L] = DOS_PD("WAVE420L",
		GX_EE_PD(DOS_WAVE420L_PWR, DOS_WAVE420L_ISO),
		DOS_MEM_PD_WAVE420L, DOS_MEM_PD_WAVE420L_ALL),
};

#if 0
static struct meson_ee_pwrc_domain_desc sm1_pwrc_domains[] = {
	[PWRC_VDEC1] = DOS_PD("VDEC1",
			SM1_EE_PD(1),
			DOS_MEM_PD_VDEC, DOS_MEM_PD_VDEC_ALL),
	[PWRC_HEVC] = DOS_PD("HEVC",
			SM1_EE_PD(2),
			DOS_MEM_PD_HEVC, DOS_MEM_PD_HEVC_ALL),
	[PWRC_HCODEC] = DOS_PD("HCODEC",
			SM1_EE_PD(1),
			DOS_MEM_PD_HCODEC, DOS_MEM_PD_HCODEC_ALL),
	[PWRC_WAVE420L] = DOS_PD("WAVE420L",
			SM1_EE_PD(8),
			DOS_MEM_PD_WAVE420L, DOS_MEM_PD_WAVE420L_ALL),
};
#endif

static const struct meson_codec_formats gxl_codecs[] = {
	DECODER(mpeg1, 1920, 1080, nv12, yuv420)
	DECODER(mpeg2, 1920, 1080, nv12, yuv420)
	DECODER(h264,  3840, 2160, nv12, yuv420)
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
		[CLK_VDEC1] = &gxbb_dos_gate_vdec1.hw,
		[CLK_HEVC] = &gxbb_dos_gate_hevc.hw,
		[CLK_HCODEC] = &gxbb_dos_gate_hcodec.hw,
		[CLK_WAVE420L] = &gxbb_wave420l.hw,
	},
	.pwrc = gx_pwrc_domains,
	.codecs = gxl_codecs,
	.num_codecs = ARRAY_SIZE(gxl_codecs),
	.firmwares = {
		[MPEG1_DECODER] = "meson/vdec/gxl_mpeg12.bin",
		[MPEG2_DECODER] = "meson/vdec/gxl_mpeg12.bin",
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
