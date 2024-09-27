#ifndef _MESON_CODECS_H
#define _MESON_CODECS_H

#include <linux/types.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/videobuf2-v4l2.h>

#include "meson-formats.h"

#define MIN_RESOLUTION_WIDTH 320
#define MIN_RESOLUTION_HEIGHT 240

enum meson_codecs: u8 {
	MPEG1_DECODER,
	MPEG2_DECODER,
	H264_DECODER,
	VP9_DECODER,
	HEVC_DECODER,
	H264_ENCODER,
	HEVC_ENCODER,
	MAX_CODECS
};

struct meson_vcodec_core;
struct meson_vcodec_session;

struct v4l2_std_ctrl {
	u32 id;
	s32 min;
	s32 max;
	s32 step;
	s32 def;
};

struct meson_codec_spec {
	const enum meson_codecs type;
	const struct meson_codec_ops *ops;
	const struct v4l2_std_ctrl *ctrls;
	const int num_ctrls;
};

struct meson_codec_dev {
	const struct meson_codec_spec *spec;
	struct meson_vcodec_core *core;
	struct v4l2_ctrl_handler ctrl_handler;
	void *priv;
};

struct meson_codec_job {
	struct meson_vcodec_session *session;
	struct meson_codec_dev *codec;
	const struct v4l2_pix_format_mplane *src_fmt;
	const struct v4l2_pix_format_mplane *dst_fmt;
	void *priv;
};

struct meson_codec_ops {
	int (*init)(struct meson_codec_dev *codec);
	int (*prepare)(struct meson_codec_job *job);
	int (*start)(struct meson_codec_job *job, struct vb2_queue *vq, u32 count);
	int (*queue)(struct meson_codec_job *job, struct vb2_v4l2_buffer *vb);
	void (*run)(struct meson_codec_job *job);
	void (*abort)(struct meson_codec_job *job);
	int (*stop)(struct meson_codec_job *job, struct vb2_queue *vq);
	void (*unprepare)(struct meson_codec_job *job);
	void (*release)(struct meson_codec_dev *codec);
};

struct meson_codec_formats {
	const struct meson_format *src_fmt;
	const struct meson_format *dst_fmt;
	const struct meson_format *int_fmt;
	const struct meson_codec_spec *decoder;
	const struct meson_codec_spec *encoder;
	const u16 max_width;
	const u16 max_height;
};

extern const struct meson_codec_spec mpeg1_decoder;
extern const struct meson_codec_spec mpeg2_decoder;
extern const struct meson_codec_spec h264_decoder;
extern const struct meson_codec_spec vp9_decoder;
extern const struct meson_codec_spec hevc_decoder;
extern const struct meson_codec_spec h264_encoder;
extern const struct meson_codec_spec hevc_encoder;

#endif
