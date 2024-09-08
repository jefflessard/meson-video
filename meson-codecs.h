#ifndef _MESON_CODECS_H
#define _MESON_CODECS_H

#include <linux/types.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>

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

struct meson_vcodec_session;

struct meson_codec_spec {
	enum meson_codecs type;
	const struct meson_codec_ops *ops;
	const struct v4l2_ctrl_ops *ctrl_ops;
	const struct v4l2_ctrl_config *ctrls;
	const int num_ctrls;
};

struct meson_codec_job {
	struct meson_vcodec_session *session;
	const struct meson_codec_spec *codec;
	const struct v4l2_pix_format_mplane *src_fmt;
	const struct v4l2_pix_format_mplane *dst_fmt;
	void *priv;
};

struct meson_codec_ops {
	int (*init)(struct meson_codec_job *job, struct vb2_queue *vq);
//	int (*configure)(struct meson_codec_job *job);
	int (*start)(struct meson_codec_job *job, struct vb2_queue *vq, u32 count);
	int (*queue)(struct meson_codec_job *job, struct vb2_buffer *vb);
//	irqreturn_t (*isr)(int irq, void *dev_id);
//	int (*cancel)(struct meson_codec_job *job);
	int (*stop)(struct meson_codec_job *job, struct vb2_queue *vq);
//	int (*reset)(struct meson_codec_job *job);
	int (*release)(struct meson_codec_job *job, struct vb2_queue *vq);
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


extern const struct meson_codec_spec mpeg1_decoder;
extern const struct meson_codec_spec mpeg2_decoder;
extern const struct meson_codec_spec h264_decoder;
extern const struct meson_codec_spec vp9_decoder;
extern const struct meson_codec_spec hevc_decoder;
extern const struct meson_codec_spec h264_encoder;
extern const struct meson_codec_spec hevc_encoder;


/* helper macros */

#define ENCODER_OPS(__session, __ops, ...) \
	__session->enc_job.codec->ops->__ops(&__session->enc_job, ##__VA_ARGS__)

#define DECODER_OPS(__session, __ops, ...) \
	__session->dec_job.codec->ops->__ops(&__session->dec_job, ##__VA_ARGS__)

#endif
