#ifndef _MESON_CODECS_H
#define _MESON_CODECS_H

#include <linux/args.h>
#include <media/v4l2-ctrls.h>

#include "meson-formats.h"

#define MIN_RESOLUTION_WIDTH 320
#define MIN_RESOLUTION_HEIGHT 240

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


/* Helper codec macros */

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


struct meson_vcodec_session;

struct meson_codec_ops {
	int (*init)(struct meson_vcodec_session *session);
	int (*configure)(struct meson_vcodec_session *session);
	int (*start)(struct meson_vcodec_session *session);
	int (*process_frame)(struct meson_vcodec_session *session);
	irqreturn_t (*isr)(int irq, void *dev_id);
	int (*cancel_frame)(struct meson_vcodec_session *session);
	int (*stop)(struct meson_vcodec_session *session);
	int (*reset)(struct meson_vcodec_session *session);
	int (*cleanup)(struct meson_vcodec_session *session);
};

struct meson_codec_spec {
	const struct meson_codec_ops *ops;
	const struct v4l2_ctrl_ops *ctrl_ops;
	const struct v4l2_ctrl_config *ctrls;
	const int num_ctrls;
};

struct meson_codec_formats {
	const struct meson_format *input_format;
	const struct meson_format *output_format;
	const struct meson_format *intermediate_format;
	const struct meson_codec_spec *decoder;
	const struct meson_codec_spec *encoder;
	const u16 max_width;
	const u16 max_height;
};

static const struct meson_codec_ops mpeg1_decoder_ops = {
};

static const struct v4l2_ctrl_config mpeg1_decoder_ctrls[] = {
};

static const struct v4l2_ctrl_ops mpeg1_decoder_ctrl_ops = {
};

static const struct meson_codec_ops mpeg2_decoder_ops = {
};

static const struct v4l2_ctrl_config mpeg2_decoder_ctrls[] = {
};

static const struct v4l2_ctrl_ops mpeg2_decoder_ctrl_ops = {
};

static const struct meson_codec_ops h264_decoder_ops = {
};

static const struct v4l2_ctrl_config h264_decoder_ctrls[] = {
};

static const struct v4l2_ctrl_ops h264_decoder_ctrl_ops = {
};

static const struct meson_codec_ops vp9_decoder_ops = {
};

static const struct v4l2_ctrl_config vp9_decoder_ctrls[] = {
};

static const struct v4l2_ctrl_ops vp9_decoder_ctrl_ops = {
};

static const struct meson_codec_ops hevc_decoder_ops = {
};

static const struct v4l2_ctrl_config hevc_decoder_ctrls[] = {
};

static const struct v4l2_ctrl_ops hevc_decoder_ctrl_ops = {
};

static const struct meson_codec_ops h264_encoder_ops = {
};

static const struct v4l2_ctrl_config h264_encoder_ctrls[] = {
};

static const struct v4l2_ctrl_ops h264_encoder_ctrl_ops = {
};

static const struct meson_codec_ops hevc_encoder_ops = {
};

static const struct v4l2_ctrl_config hevc_encoder_ctrls[] = {
};

static const struct v4l2_ctrl_ops hevc_encoder_ctrl_ops = {
};

const struct meson_codec_spec mpeg1_decoder = {
	.ops = &mpeg1_decoder_ops,
	.ctrl_ops = &mpeg1_decoder_ctrl_ops,
	.ctrls = mpeg1_decoder_ctrls,
	.num_ctrls = ARRAY_SIZE(mpeg1_decoder_ctrls),
};

const struct meson_codec_spec mpeg2_decoder = {
	.ops = &mpeg2_decoder_ops,
	.ctrl_ops = &mpeg2_decoder_ctrl_ops,
	.ctrls = mpeg2_decoder_ctrls,
	.num_ctrls = ARRAY_SIZE(mpeg2_decoder_ctrls),
};

const struct meson_codec_spec h264_decoder = {
	.ops = &h264_decoder_ops,
	.ctrl_ops = &h264_decoder_ctrl_ops,
	.ctrls = h264_decoder_ctrls,
	.num_ctrls = ARRAY_SIZE(h264_decoder_ctrls),
};

const struct meson_codec_spec vp9_decoder = {
	.ops = &vp9_decoder_ops,
	.ctrl_ops = &vp9_decoder_ctrl_ops,
	.ctrls = vp9_decoder_ctrls,
	.num_ctrls = ARRAY_SIZE(vp9_decoder_ctrls),
};

const struct meson_codec_spec hevc_decoder = {
	.ops = &hevc_decoder_ops,
	.ctrl_ops = &hevc_decoder_ctrl_ops,
	.ctrls = hevc_decoder_ctrls,
	.num_ctrls = ARRAY_SIZE(hevc_decoder_ctrls),
};

const struct meson_codec_spec h264_encoder = {
	.ops = &h264_encoder_ops,
	.ctrl_ops = &h264_encoder_ctrl_ops,
	.ctrls = h264_encoder_ctrls,
	.num_ctrls = ARRAY_SIZE(h264_encoder_ctrls),
};

const struct meson_codec_spec hevc_encoder = {
	.ops = &hevc_encoder_ops,
	.ctrl_ops = &hevc_encoder_ctrl_ops,
	.ctrls = hevc_encoder_ctrls,
	.num_ctrls = ARRAY_SIZE(hevc_encoder_ctrls),
};

#endif
