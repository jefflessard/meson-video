#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>

#include "meson-formats.h"
#include "meson-codecs.h"
#include "meson-vcodec.h"

#include "vdec/vdec.h"
#include "vdec/codec_mpeg12.h"
#include "vdec/codec_h264.h"
#include "vdec/codec_vp9.h"
#include "vdec/codec_hevc.h"
#include "vdec/vdec_1.h"
#include "vdec/vdec_hevc.h"
#include "vdec/esparser.h"
#include "vdec/vdec_helpers.h"

struct meson_vdec_adapter {
	const struct meson_codec_spec *decoder;
	struct amvdec_codec_ops *vdec_ops;
	struct amvdec_session *vdec_sess;
	struct amvdec_core *vdec_core;
};

static irqreturn_t vdec_isr(int irq, void *data)
{
	const struct meson_codec_spec *decoder = data;
	struct meson_vdec_adapter *adapter = container_of(&decoder, struct meson_vdec_adapter, decoder);

	return adapter->vdec_ops->isr(adapter->vdec_sess);
}

static irqreturn_t vdec_threaded_isr(int irq, void *data)
{
	const struct meson_codec_spec *decoder = data;
	struct meson_vdec_adapter *adapter = container_of(&decoder, struct meson_vdec_adapter, decoder);

	return adapter->vdec_ops->threaded_isr(adapter->vdec_sess);
}

static int meson_vdec_adapter_init(struct meson_vcodec_session *session){
	struct meson_vdec_adapter *adapter = container_of(&session->codec->decoder, struct meson_vdec_adapter, decoder);
	struct amvdec_format *fmt_out;
	int ret;

	adapter->vdec_core = kcalloc(1, sizeof(*adapter->vdec_core), GFP_KERNEL);
	if (!adapter->vdec_core)
		return -ENOMEM;
	
	adapter->vdec_core->platform = kcalloc(1, sizeof(struct vdec_platform), GFP_KERNEL);
	if (!adapter->vdec_core->platform)
		return -ENOMEM;

	adapter->vdec_sess = kcalloc(1, sizeof(*adapter->vdec_sess), GFP_KERNEL);
	if (!adapter->vdec_sess)
		return -ENOMEM;

	fmt_out = kcalloc(1, sizeof(struct amvdec_format), GFP_KERNEL);
	if (!fmt_out)
		return -ENOMEM;
	// bypass sess fmt_out const by using fmt_out
	adapter->vdec_sess->fmt_out = fmt_out;

	// TODO populate vdec core and sess values
	//adapter->vdec_core->canvas
	//adapter->vdec_core->platform->revision
	adapter->vdec_core->dev = session->core->dev;
	adapter->vdec_core->dev_dec = session->core->dev;
	adapter->vdec_core->dos_base = session->core->regs[DOS_BASE];
	adapter->vdec_core->esparser_base = session->core->regs[PARSER_BASE];
	adapter->vdec_core->regmap_ao = session->core->regmap_ao;
	adapter->vdec_core->vdec_1_clk = session->core->clks[CLK_VDEC1];
	adapter->vdec_core->vdec_hevc_clk = session->core->clks[CLK_HEVC];
	adapter->vdec_core->vdec_hevcf_clk = session->core->clks[CLK_HEVCF];

	fmt_out->codec_ops = adapter->vdec_ops;
	fmt_out->pixfmt = session->input_format.fmt.pix_mp.pixelformat;
	fmt_out->firmware_path = (char *)session->core->platform_specs->firmwares[session->codec->decoder->type];
	if (session->codec->decoder->type >= VP9_DECODER) {
		fmt_out->vdec_ops = &vdec_hevc_ops;
	} else {
		fmt_out->vdec_ops = &vdec_1_ops;
	}
	
	//adapter->vdec_sess->canvas_alloc
	//adapter->vdec_sess->canvas_num
	//adapter->vdec_sess->ctrl_min_buf_capture
	//adapter->vdec_sess->esparser_queue_work
	//adapter->vdec_sess->num_dst_bufs
	//adapter->vdec_sess->should_stop
	//adapter->vdec_sess->status
	//adapter->vdec_sess->streamon_cap
	//adapter->vdec_sess->timestamps
	//adapter->vdec_sess->vififo_paddr
	// kfree only adapter->vdec_sess->priv
	spin_lock_init(&adapter->vdec_sess->ts_spinlock);
	adapter->vdec_sess->core = adapter->vdec_core;
	adapter->vdec_sess->fh = session->fh;
	adapter->vdec_sess->width = session->input_format.fmt.pix_mp.height;
	adapter->vdec_sess->height = session->input_format.fmt.pix_mp.height;
	adapter->vdec_sess->lock = session->lock;
	adapter->vdec_sess->m2m_ctx = session->m2m_ctx;
	adapter->vdec_sess->pixfmt_cap = session->output_format.fmt.pix_mp.pixelformat;
	atomic_set(&adapter->vdec_sess->esparser_queued_bufs, 0);
	adapter->vdec_sess->changed_format = 1;
	adapter->vdec_sess->keyframe_found = 0;
	adapter->vdec_sess->last_offset = 0;
	adapter->vdec_sess->pixelaspect.denominator = 1;
	adapter->vdec_sess->pixelaspect.numerator = 1;
	adapter->vdec_sess->sequence_cap = 0;
	adapter->vdec_sess->sequence_out = 0;
	adapter->vdec_sess->vififo_size = SZ_16M;
	adapter->vdec_sess->wrap_count = 0;


	ret = request_threaded_irq(session->core->irqs[IRQ_VDEC], vdec_isr, vdec_threaded_isr, IRQF_ONESHOT, "vdec", (void *) adapter->decoder);
	if (ret) {
		return ret;
	}
	
	return adapter->vdec_ops->load_extended_firmware(adapter->vdec_sess, NULL, 0); //TODO
}

static int meson_vdec_adapter_configure(struct meson_vcodec_session *session){
	struct meson_vdec_adapter *adapter = container_of(&session->codec->decoder, struct meson_vdec_adapter, decoder);

	adapter->vdec_ops->resume(adapter->vdec_sess);
	return 0;
}

static int meson_vdec_adapter_start(struct meson_vcodec_session *session){
	struct meson_vdec_adapter *adapter = container_of(&session->codec->decoder, struct meson_vdec_adapter, decoder);

	return adapter->vdec_ops->start(adapter->vdec_sess);
}

static int meson_vdec_adapter_cancel(struct meson_vcodec_session *session){
	struct meson_vdec_adapter *adapter = container_of(&session->codec->decoder, struct meson_vdec_adapter, decoder);

	adapter->vdec_ops->drain(adapter->vdec_sess);
	return 0;
}

static int meson_vdec_adapter_stop(struct meson_vcodec_session *session){
	struct meson_vdec_adapter *adapter = container_of(&session->codec->decoder, struct meson_vdec_adapter, decoder);

	return adapter->vdec_ops->stop(adapter->vdec_sess);
}

static int meson_vdec_adapter_release(struct meson_vcodec_session *session){
	struct meson_vdec_adapter *adapter = container_of(&session->codec->decoder, struct meson_vdec_adapter, decoder);

	free_irq(session->core->irqs[IRQ_VDEC], (void *)adapter->decoder);

	kfree(adapter->vdec_sess->priv);
	kfree(adapter->vdec_sess->fmt_out);
	kfree(adapter->vdec_sess);
	adapter->vdec_sess = NULL;

	kfree(adapter->vdec_core->platform);
	kfree(adapter->vdec_core);
	adapter->vdec_core = NULL;

	return 0;
}

static const struct v4l2_ctrl_config mpeg12_decoder_ctrls[] = {
};

static const struct v4l2_ctrl_ops mpeg12_decoder_ctrl_ops = {
};

static const struct v4l2_ctrl_config h264_decoder_ctrls[] = {
};

static const struct v4l2_ctrl_ops h264_decoder_ctrl_ops = {
};

static const struct v4l2_ctrl_config vp9_decoder_ctrls[] = {
};

static const struct v4l2_ctrl_ops vp9_decoder_ctrl_ops = {
};

static const struct v4l2_ctrl_config hevc_decoder_ctrls[] = {
};

static const struct v4l2_ctrl_ops hevc_decoder_ctrl_ops = {
};

const struct meson_codec_ops codec_ops_vdec_adapter = {
	.init = &meson_vdec_adapter_init,
	.configure = &meson_vdec_adapter_configure,
	.start = &meson_vdec_adapter_start,
	.cancel = &meson_vdec_adapter_cancel,
	.stop = &meson_vdec_adapter_stop,
	.release = &meson_vdec_adapter_release,
};

const struct meson_codec_spec mpeg1_decoder = {
	.type = MPEG1_DECODER,
	.ops = &codec_ops_vdec_adapter,
	.ctrl_ops = &mpeg12_decoder_ctrl_ops,
	.ctrls = mpeg12_decoder_ctrls,
	.num_ctrls = ARRAY_SIZE(mpeg12_decoder_ctrls),
};

const struct meson_codec_spec mpeg2_decoder = {
	.type = MPEG2_DECODER,
	.ops = &codec_ops_vdec_adapter,
	.ctrl_ops = &mpeg12_decoder_ctrl_ops,
	.ctrls = mpeg12_decoder_ctrls,
	.num_ctrls = ARRAY_SIZE(mpeg12_decoder_ctrls),
};

const struct meson_codec_spec h264_decoder = {
	.type = H264_DECODER,
	.ops = &codec_ops_vdec_adapter,
	.ctrl_ops = &h264_decoder_ctrl_ops,
	.ctrls = h264_decoder_ctrls,
	.num_ctrls = ARRAY_SIZE(h264_decoder_ctrls),
};

const struct meson_codec_spec vp9_decoder = {
	.type = VP9_DECODER,
	.ops = &codec_ops_vdec_adapter,
	.ctrl_ops = &vp9_decoder_ctrl_ops,
	.ctrls = vp9_decoder_ctrls,
	.num_ctrls = ARRAY_SIZE(vp9_decoder_ctrls),
};

const struct meson_codec_spec hevc_decoder = {
	.type = HEVC_DECODER,
	.ops = &codec_ops_vdec_adapter,
	.ctrl_ops = &hevc_decoder_ctrl_ops,
	.ctrls = hevc_decoder_ctrls,
	.num_ctrls = ARRAY_SIZE(hevc_decoder_ctrls),
};

static const struct meson_vdec_adapter adapters[] = {
	{ .decoder = &mpeg1_decoder, .vdec_ops = &codec_mpeg12_ops },
	{ .decoder = &mpeg2_decoder, .vdec_ops = &codec_mpeg12_ops },
	{ .decoder = &h264_decoder, .vdec_ops = &codec_h264_ops },
	{ .decoder = &vp9_decoder, .vdec_ops = &codec_vp9_ops },
	{ .decoder = &hevc_decoder, .vdec_ops = &codec_hevc_ops },
};

