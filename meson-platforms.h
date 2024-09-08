#ifndef _MESON_PLATFORMS_H
#define _MESON_PLATFORMS_H

#include <linux/args.h>
#include <linux/types.h>

#include "meson-formats.h"
#include "meson-codecs.h"

enum meson_platform_id: u8 {
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
	MAX_MESON_MAJOR_ID,
};

enum meson_vcodec_pwrc: u8 {
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
	const char *firmwares[MAX_FORMATS];
};

extern const struct meson_platform_specs gxl_platform_specs;


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
		.input_format = &__format, \
		.output_format = &__name, \
		.intermediate_format = NULL, \
		.decoder = NULL, \
		.encoder = &__name##_encoder, \
		.max_width = __max_width, \
		.max_height = __max_height, \
	},

#define DECODER(__name, __max_width, __max_height, ...) \
	FOREACH(DECODER_ITEM,(__name, __max_width, __max_height), __VA_ARGS__)

#define DECODER_ITEM(__name, __max_width, __max_height, __format) \
	{ \
		.input_format = &__name, \
		.output_format = &__format, \
		.intermediate_format = NULL, \
		.decoder = &__name##_decoder, \
		.encoder = NULL, \
		.max_width = __max_width, \
		.max_height = __max_height, \
	},

#define TRANSCODER(__encoder, __format, __max_width, __max_height, ...) \
	FOREACH(TRANSCODER_ITEM,(__encoder, __format, __max_width, __max_height), __VA_ARGS__)

#define TRANSCODER_ITEM(__encoder, __format, __max_width, __max_height, __decoder) \
	{ \
		.input_format = &__decoder, \
		.output_format = &__encoder, \
		.intermediate_format = &__format, \
		.decoder = &__decoder##_decoder, \
		.encoder = &__encoder##_encoder, \
		.max_width = __max_width, \
		.max_height = __max_height, \
	},

#endif
