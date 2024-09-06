#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>

#include "meson-formats.h"
#include "meson-codecs.h"
#include "meson-vcodec.h"


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

const struct meson_codec_spec h264_encoder = {
	.type = H264_ENCODER,
	.ops = &h264_encoder_ops,
	.ctrl_ops = &h264_encoder_ctrl_ops,
	.ctrls = h264_encoder_ctrls,
	.num_ctrls = ARRAY_SIZE(h264_encoder_ctrls),
};

const struct meson_codec_spec hevc_encoder = {
	.type = HEVC_ENCODER,
	.ops = &hevc_encoder_ops,
	.ctrl_ops = &hevc_encoder_ctrl_ops,
	.ctrls = hevc_encoder_ctrls,
	.num_ctrls = ARRAY_SIZE(hevc_encoder_ctrls),
};

