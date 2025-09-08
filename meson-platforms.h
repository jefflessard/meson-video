#ifndef _MESON_PLATFORMS_H
#define _MESON_PLATFORMS_H

#include <linux/args.h>
#include <linux/types.h>

#include "meson-formats.h"
#include "meson-codecs.h"
#include "amlogic.h"

enum meson_vcodec_pwrc: u8 {
	PWRC_VDEC1,
	PWRC_HEVC,
	PWRC_HCODEC,
	MAX_PWRC
};

struct meson_vcodec_core;
struct meson_dos_pwrc_domain;

struct meson_platform_specs {
	const enum AM_MESON_CPU_MAJOR_ID platform_id;
	const struct meson_dos_pwrc_domain *pwrc;
	const struct meson_codec_formats *codecs;
	const u32 num_codecs;
	const char *firmwares[MAX_CODECS];
};

extern const struct meson_platform_specs gxl_platform_specs;

#endif
