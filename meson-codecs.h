#ifndef _MESON_CODECS_H
#define _MESON_CODECS_H

#include <media/v4l2-ctrls.h>

#include "meson-formats.h"

#define MIN_RESOLUTION_WIDTH 320
#define MIN_RESOLUTION_HEIGHT 240

struct meson_vcodec_session;

struct meson_codec_ops {
	int (*init)(struct meson_vcodec_session *session);
	int (*cleanup)(struct meson_vcodec_session *session);
	int (*configure)(struct meson_vcodec_session *session);
	int (*start)(struct meson_vcodec_session *session);
	int (*stop)(struct meson_vcodec_session *session);
	int (*reset)(struct meson_vcodec_session *session);
	int (*process_frame)(struct meson_vcodec_session *session);
	irqreturn_t (*isr)(int irq, void *dev_id);
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

static const struct meson_codec_ops h264_decoder_ops = {
};

static const struct v4l2_ctrl_config h264_decoder_ctrls[] = {
	// H.264 decoder specific controls
};

static const struct v4l2_ctrl_ops h264_decoder_ctrl_ops = {
};

static const struct meson_codec_ops hevc_decoder_ops = {
};

static const struct v4l2_ctrl_config hevc_decoder_ctrls[] = {
	// HEVC decoder specific controls
};

static const struct v4l2_ctrl_ops hevc_decoder_ctrl_ops = {
};

static const struct meson_codec_ops h264_encoder_ops = {
};

static const struct v4l2_ctrl_config h264_encoder_ctrls[] = {
	// H.264 encoder specific controls
};

static const struct v4l2_ctrl_ops h264_encoder_ctrl_ops = {
};

static const struct meson_codec_ops hevc_encoder_ops = {
};

static const struct v4l2_ctrl_config hevc_encoder_ctrls[] = {
	// HEVC encoder specific controls
};

static const struct v4l2_ctrl_ops hevc_encoder_ctrl_ops = {
};

const struct meson_codec_spec h264_decoder = {
	.ops = &h264_decoder_ops,
	.ctrl_ops = &h264_decoder_ctrl_ops,
	.ctrls = h264_decoder_ctrls,
	.num_ctrls = ARRAY_SIZE(h264_decoder_ctrls),
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
