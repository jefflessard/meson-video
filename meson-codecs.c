#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>

#include "meson-formats.h"
#include "meson-codecs.h"
#include "meson-vcodec.h"

static const struct meson_codec_ops hevc_encoder_ops = {
};

static const struct v4l2_ctrl_config hevc_encoder_ctrls[] = {
};

const struct meson_codec_spec hevc_encoder = {
	.type = HEVC_ENCODER,
	.name = "hevc_encoder",
	.ops = &hevc_encoder_ops,
	.ctrls = hevc_encoder_ctrls,
	.num_ctrls = ARRAY_SIZE(hevc_encoder_ctrls),
};
