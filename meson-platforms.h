#ifndef _MESON_PLATFORMS_H
#define _MESON_PLATFORMS_H

#include <linux/args.h>
#include <linux/types.h>

#include "meson-formats.h"
#include "meson-codecs.h"
#include "amlogic.h"

#include "clk/meson-eeclk.h"

enum meson_vcodec_pwrc: u8 {
	PWRC_VDEC,
	PWRC_HEVC,
	PWRC_HCODEC,
	PWRC_WAVE420L,
	MAX_PWRC
};

struct meson_vcodec_core;
struct meson_ee_pwrc_top_domain;

struct meson_platform_specs {
	const enum AM_MESON_CPU_MAJOR_ID platform_id;
	const struct meson_eeclkc_data *clks;
	const struct clk_hw *hwclks[MAX_CLOCKS];
	const struct meson_ee_pwrc_top_domain *pwrc;
	const struct meson_codec_formats *codecs;
	const u32 num_codecs;
	const char *firmwares[MAX_CODECS];
};

extern const struct meson_platform_specs gxl_platform_specs;

int meson_platform_register_clks(struct meson_vcodec_core *core);

#endif
