#include <linux/clk.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/videobuf2-core.h>
#include <media/v4l2-mem2mem.h>

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

/* 16 MiB for parsed bitstream swap exchange */
#define SIZE_VIFIFO SZ_16M

static struct amvdec_codec_ops *vdec_decoders[MAX_CODECS] = {
	[MPEG1_DECODER] = &codec_mpeg12_ops,
	[MPEG2_DECODER] = &codec_mpeg12_ops,
	[H264_DECODER] = &codec_h264_ops,
	[VP9_DECODER] = &codec_vp9_ops,
	[HEVC_DECODER] = &codec_hevc_ops,
	//[H264_ENCODER] = NULL,
	//[HEVC_ENCODER] = NULL,
};

struct meson_vdec_adapter {
	struct amvdec_codec_ops *vdec_codec_ops;
	struct amvdec_session vdec_sess;
	struct amvdec_core vdec_core;
	struct amvdec_format vdec_fmt_out;
	struct vdec_platform vdec_platform;
};

// TODO meson_vcodec_queue_setup -> process_num_buffers
// TODO vdec_g_pixelaspect

static u32 get_output_size(u32 width, u32 height)
{
	return ALIGN(width * height, SZ_64K);
}

u32 amvdec_get_output_size(struct amvdec_session *sess)
{
	return get_output_size(sess->width, sess->height);
}


static int vdec_codec_needs_recycle(struct amvdec_session *sess)
{
	struct amvdec_codec_ops *codec_ops = sess->fmt_out->codec_ops;

	return codec_ops->can_recycle && codec_ops->recycle;
}

static int vdec_recycle_thread(void *data)
{
	struct amvdec_session *sess = data;
	struct amvdec_core *core = sess->core;
	struct amvdec_codec_ops *codec_ops = sess->fmt_out->codec_ops;
	struct amvdec_buffer *tmp, *n;

	while (!kthread_should_stop()) {
		mutex_lock(&sess->bufs_recycle_lock);
		list_for_each_entry_safe(tmp, n, &sess->bufs_recycle, list) {
			if (!codec_ops->can_recycle(core))
				break;

			codec_ops->recycle(core, tmp->vb->index);
			list_del(&tmp->list);
			kfree(tmp);
		}
		mutex_unlock(&sess->bufs_recycle_lock);

		usleep_range(5000, 10000);
	}

	return 0;
}

static int vdec_poweron(struct amvdec_session *sess)
{
	int ret;
	struct amvdec_ops *vdec_ops = sess->fmt_out->vdec_ops;

	ret = clk_prepare_enable(sess->core->dos_parser_clk);
	if (ret) {
		dev_err(sess->core->dev, "Failed to enable dos_parser_clk: %d", ret);
		return ret;
	}

	ret = clk_prepare_enable(sess->core->dos_clk);
	if (ret) {
		dev_err(sess->core->dev, "Failed to enable dos_clk: %d", ret);
		goto disable_dos_parser;
	}

	ret = vdec_ops->start(sess);
	if (ret) {
		dev_err(sess->core->dev, "Failed to start vdec: %d", ret);
		goto disable_dos;
	}

	esparser_power_up(sess);

	return 0;

disable_dos:
	clk_disable_unprepare(sess->core->dos_clk);
disable_dos_parser:
	clk_disable_unprepare(sess->core->dos_parser_clk);

	return ret;
}

static void vdec_wait_inactive(struct amvdec_session *sess)
{
	/* We consider 50ms with no IRQ to be inactive. */
	while (time_is_after_jiffies64(sess->last_irq_jiffies + msecs_to_jiffies(50)))
		msleep(25);
}

static void vdec_poweroff(struct amvdec_session *sess)
{
	struct amvdec_ops *vdec_ops = sess->fmt_out->vdec_ops;
	struct amvdec_codec_ops *codec_ops = sess->fmt_out->codec_ops;

	sess->should_stop = 1;
	vdec_wait_inactive(sess);
	if (codec_ops->drain)
		codec_ops->drain(sess);

	vdec_ops->stop(sess);
	clk_disable_unprepare(sess->core->dos_clk);
	clk_disable_unprepare(sess->core->dos_parser_clk);
}

static void vdec_queue_recycle(struct amvdec_session *sess, struct vb2_buffer *vb)
{
	struct amvdec_buffer *new_buf;

	new_buf = kmalloc(sizeof(*new_buf), GFP_KERNEL);
	if (!new_buf)
		return;
	new_buf->vb = vb;

	mutex_lock(&sess->bufs_recycle_lock);
	list_add_tail(&new_buf->list, &sess->bufs_recycle);
	mutex_unlock(&sess->bufs_recycle_lock);
}

static void process_num_buffers(struct vb2_queue *q,
				struct amvdec_session *sess,
				unsigned int *num_buffers,
				bool is_reqbufs)
{
	const struct amvdec_format *fmt_out = sess->fmt_out;
	unsigned int buffers_total = q->num_buffers + *num_buffers;
	u32 min_buf_capture = v4l2_ctrl_g_ctrl(sess->ctrl_min_buf_capture);

	if (q->num_buffers + *num_buffers < min_buf_capture)
		*num_buffers = min_buf_capture - q->num_buffers;
	if (is_reqbufs && buffers_total < fmt_out->min_buffers)
		*num_buffers = fmt_out->min_buffers - q->num_buffers;
	if (buffers_total > fmt_out->max_buffers)
		*num_buffers = fmt_out->max_buffers - q->num_buffers;

	/* We need to program the complete CAPTURE buffer list
	 * in registers during start_streaming, and the firmwares
	 * are free to choose any of them to write frames to. As such,
	 * we need all of them to be queued into the driver
	 */
	sess->num_dst_bufs = q->num_buffers + *num_buffers;
	q->min_buffers_needed = max(fmt_out->min_buffers, sess->num_dst_bufs);
}

static void vdec_free_canvas(struct amvdec_session *sess)
{
	int i;

	for (i = 0; i < sess->canvas_num; ++i)
		meson_canvas_free(sess->core->canvas, sess->canvas_alloc[i]);

	sess->canvas_num = 0;
}

static void vdec_reset_timestamps(struct amvdec_session *sess)
{
	struct amvdec_timestamp *tmp, *n;

	list_for_each_entry_safe(tmp, n, &sess->timestamps, list) {
		list_del(&tmp->list);
		kfree(tmp);
	}
}

static void vdec_reset_bufs_recycle(struct amvdec_session *sess)
{
	struct amvdec_buffer *tmp, *n;

	list_for_each_entry_safe(tmp, n, &sess->bufs_recycle, list) {
		list_del(&tmp->list);
		kfree(tmp);
	}
}

static void vdec_vb2_buf_queue(struct amvdec_session *sess, struct vb2_buffer *vb)
{
	if (!sess->streamon_out)
		return;

	if (sess->streamon_cap &&
	    vb->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE &&
	    vdec_codec_needs_recycle(sess))
		vdec_queue_recycle(sess, vb);

	schedule_work(&sess->esparser_queue_work);
}

static int vdec_start_streaming(struct amvdec_session *sess, struct vb2_queue *q, unsigned int count)
{
	struct amvdec_codec_ops *codec_ops = sess->fmt_out->codec_ops;
	struct amvdec_core *core = sess->core;
	int ret;

	if (core->cur_sess && core->cur_sess != sess) {
		ret = -EBUSY;
		goto bufs_done;
	}

	if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		sess->streamon_out = 1;
	else
		sess->streamon_cap = 1;

	if (!sess->streamon_out)
		return 0;

	dev_dbg(core->dev, "%s: tyoe=%d, status=%d, changed_format=%d", __func__, q->type, sess->status, sess->changed_format);

	if (sess->status == STATUS_NEEDS_RESUME &&
	    q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE && sess->changed_format) {
		codec_ops->resume(sess);
		sess->status = STATUS_RUNNING;
		return 0;
	}

	if (sess->status == STATUS_RUNNING ||
	    sess->status == STATUS_NEEDS_RESUME ||
	    sess->status == STATUS_INIT)
		return 0;

	sess->vififo_size = SIZE_VIFIFO;
	sess->vififo_vaddr = dma_alloc_coherent(sess->core->dev, sess->vififo_size, &sess->vififo_paddr, GFP_KERNEL);
	if (!sess->vififo_vaddr) {
		dev_err(sess->core->dev, "Failed to request VIFIFO buffer\n");
		ret = -ENOMEM;
		goto bufs_done;
	}

	sess->should_stop = 0;
	sess->keyframe_found = 0;
	sess->last_offset = 0;
	sess->wrap_count = 0;
	sess->pixelaspect.numerator = 1;
	sess->pixelaspect.denominator = 1;
	atomic_set(&sess->esparser_queued_bufs, 0);
	v4l2_ctrl_s_ctrl(sess->ctrl_min_buf_capture, 1);

	ret = vdec_poweron(sess);
	if (ret)
		goto vififo_free;

	sess->sequence_cap = 0;
	sess->sequence_out = 0;
	if (vdec_codec_needs_recycle(sess))
		sess->recycle_thread = kthread_run(vdec_recycle_thread, sess,
						   "vdec_recycle");

	sess->status = STATUS_INIT;
	core->cur_sess = sess;
	schedule_work(&sess->esparser_queue_work);
	return 0;

vififo_free:
	dma_free_coherent(sess->core->dev, sess->vififo_size,
			  sess->vififo_vaddr, sess->vififo_paddr);
bufs_done:
	if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		sess->streamon_out = 0;
	else
		sess->streamon_cap = 0;

	return ret;
}

static void vdec_stop_streaming(struct amvdec_session *sess, struct vb2_queue *q)
{
	struct amvdec_codec_ops *codec_ops = sess->fmt_out->codec_ops;
	struct amvdec_core *core = sess->core;

	if (sess->status == STATUS_RUNNING ||
	    sess->status == STATUS_INIT ||
	    (sess->status == STATUS_NEEDS_RESUME &&
	     (!sess->streamon_out || !sess->streamon_cap))) {
		if (vdec_codec_needs_recycle(sess))
			kthread_stop(sess->recycle_thread);

		vdec_poweroff(sess);
		vdec_free_canvas(sess);
		dma_free_coherent(sess->core->dev, sess->vififo_size, sess->vififo_vaddr, sess->vififo_paddr);
		vdec_reset_timestamps(sess);
		vdec_reset_bufs_recycle(sess);
		kfree(sess->priv);
		sess->priv = NULL;
		core->cur_sess = NULL;
		sess->status = STATUS_STOPPED;
	}

	if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		sess->streamon_out = 0;
	} else {
		/* Drain remaining refs if was still running */
		if (sess->status >= STATUS_RUNNING && codec_ops->drain)
			codec_ops->drain(sess);

		sess->streamon_cap = 0;
	}
}

static int vdec_init_ctrls(struct amvdec_session *sess)
{
	struct v4l2_ctrl_handler *ctrl_handler = sess->ctrl_handler;
	int ret;

	sess->ctrl_min_buf_capture =
		v4l2_ctrl_new_std(ctrl_handler, NULL,
				  V4L2_CID_MIN_BUFFERS_FOR_CAPTURE, 1, 32, 1,
				  1);

	ret = ctrl_handler->error;
	if (ret) {
		v4l2_ctrl_handler_free(ctrl_handler);
		return ret;
	}

	return 0;
}

static int vdec_open(struct amvdec_session *sess)
{
	int ret;

	ret = vdec_init_ctrls(sess);
	if (ret)
		return ret;

	sess->pixelaspect.numerator = 1;
	sess->pixelaspect.denominator = 1;
	sess->src_buffer_size = SZ_1M;

	INIT_LIST_HEAD(&sess->timestamps);
	INIT_LIST_HEAD(&sess->bufs_recycle);
	INIT_WORK(&sess->esparser_queue_work, esparser_queue_all_src);
	mutex_init(&sess->bufs_recycle_lock);
	spin_lock_init(&sess->ts_spinlock);

	sess->fh->ctrl_handler = sess->ctrl_handler;

	return 0;
}

static int vdec_close(struct amvdec_session *sess)
{
	mutex_destroy(&sess->bufs_recycle_lock);

	return 0;
}

static irqreturn_t vdec_isr(int irq, void *data)
{
	const struct meson_codec_job *job = data;
	struct meson_vdec_adapter *adapter = job->priv;

//	session_dbg(job->session, "%s", __func__);

	adapter->vdec_sess.last_irq_jiffies = get_jiffies_64();

	return adapter->vdec_codec_ops->isr(&adapter->vdec_sess);
}

static irqreturn_t vdec_threaded_isr(int irq, void *data)
{
	const struct meson_codec_job *job = data;
	struct meson_vdec_adapter *adapter = job->priv;

//	session_dbg(job->session, "%s", __func__);

	return adapter->vdec_codec_ops->threaded_isr(&adapter->vdec_sess);
}

static int meson_vdec_adapter_init(struct meson_codec_job *job) {
	struct meson_vcodec_session *session = job->session;
	struct meson_vdec_adapter *adapter;
	struct amvdec_format *fmt_out;
	struct vdec_platform *platform;
	int ret;

	adapter = kcalloc(1, sizeof(*adapter), GFP_KERNEL);
	if (!adapter)
		return -ENOMEM;

	adapter->vdec_codec_ops = vdec_decoders[job->codec->type];
	job->priv = adapter;
	// bypass sess fmt_out const
	fmt_out = &adapter->vdec_fmt_out;
	adapter->vdec_sess.fmt_out = fmt_out;
	// bypass core platform const
	platform = &adapter->vdec_platform;
	adapter->vdec_core.platform = platform;


	// populate vdec core and sess values

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wswitch"
	switch(session->core->platform_specs->platform_id) {
		case MESON_MAJOR_ID_GXBB:
			platform->revision = VDEC_REVISION_GXBB;
			break;
		case MESON_MAJOR_ID_GXL:
			platform->revision = VDEC_REVISION_GXL;
			break;
		case MESON_MAJOR_ID_GXM:
			platform->revision = VDEC_REVISION_GXM;
			break;
		case MESON_MAJOR_ID_G12A:
			platform->revision = VDEC_REVISION_G12A;
			break;
		case MESON_MAJOR_ID_SM1:
			platform->revision = VDEC_REVISION_SM1;
			break;
	}
	adapter->vdec_core.canvas = session->core->canvas;
	adapter->vdec_core.dev = session->core->dev;
	adapter->vdec_core.cur_sess = &adapter->vdec_sess;
	adapter->vdec_core.dev_dec = session->core->dev;
	adapter->vdec_core.lock = &session->core->lock;

	adapter->vdec_core.regmap_ao = session->core->regmap_ao;
	adapter->vdec_core.dos_base = session->core->regs[DOS_BASE];
	adapter->vdec_core.dos_clk = session->core->clks[CLK_DOS];
	adapter->vdec_core.vdec_1_clk = session->core->clks[CLK_VDEC1];
	adapter->vdec_core.vdec_hevc_clk = session->core->clks[CLK_HEVC];
	adapter->vdec_core.vdec_hevcf_clk = session->core->clks[CLK_HEVCF];

	adapter->vdec_core.dos_parser_clk = session->core->clks[CLK_PARSER];
	adapter->vdec_core.esparser_base = session->core->regs[PARSER_BASE];
	adapter->vdec_core.esparser_reset = session->core->resets[RESET_PARSER];
	ret = request_irq(session->core->irqs[IRQ_PARSER], esparser_isr, IRQF_SHARED, "esparserirq", &adapter->vdec_core);
	if (ret) {
		return ret;
	}

	fmt_out->codec_ops = adapter->vdec_codec_ops;
	fmt_out->pixfmt = job->src_fmt->pixelformat;
	fmt_out->firmware_path = (char *)session->core->platform_specs->firmwares[job->codec->type];
	if (job->codec->type >= VP9_DECODER) {
		fmt_out->vdec_ops = &vdec_hevc_ops;
	} else {
		fmt_out->vdec_ops = &vdec_1_ops;
	}

	adapter->vdec_sess.core = &adapter->vdec_core;
	adapter->vdec_sess.fh = &session->fh;
	adapter->vdec_sess.lock = &session->lock;
	adapter->vdec_sess.ctrl_handler = &session->ctrl_handler;
	adapter->vdec_sess.width = job->src_fmt->height;
	adapter->vdec_sess.height = job->src_fmt->height;
	adapter->vdec_sess.m2m_ctx = session->m2m_ctx;
	adapter->vdec_sess.pixfmt_cap = job->dst_fmt->pixelformat;

	// TODO IRQF_SHARED vs IRQF_ONESHOT
	ret = request_threaded_irq(session->core->irqs[IRQ_VDEC], vdec_isr, vdec_threaded_isr, IRQF_ONESHOT, "vdec", job);
	if (ret) {
		return ret;
	}

#if 0
	adapter->vdec_sess.num_dst_bufs = session->m2m_ctx->cap_q_ctx.q.num_buffers;
	session_dbg(job->session, "%s: num_buffers=%d", __func__, adapter->vdec_sess.num_dst_bufs);
#endif

	return vdec_open(&adapter->vdec_sess);
}

static int meson_vdec_adapter_start(struct meson_codec_job *job, struct vb2_queue *vq, u32 count) {
	struct meson_vdec_adapter *adapter = job->priv;

	stream_dbg(job->session, vq->type, "%s: num_buffers=%d", __func__, vq->num_buffers);

	if (!IS_SRC_STREAM(vq->type)) {
		adapter->vdec_sess.num_dst_bufs = vq->num_buffers;
	}

	adapter->vdec_sess.changed_format = 1;

	return vdec_start_streaming(&adapter->vdec_sess, vq, count);
}

static int meson_vdec_adapter_queue(struct meson_codec_job *job, struct vb2_buffer *vb) {
	struct meson_vdec_adapter *adapter = job->priv;

	vdec_vb2_buf_queue(&adapter->vdec_sess, vb);
	return 0;
}

static void meson_vdec_adapter_run(struct meson_codec_job *job) {
	struct meson_vdec_adapter *adapter = job->priv;
	struct amvdec_session *sess = &adapter->vdec_sess;

	v4l2_m2m_clear_state(sess->m2m_ctx);
	sess->should_stop = 0;

	schedule_work(&adapter->vdec_sess.esparser_queue_work);
}

static void meson_vdec_adapter_abort(struct meson_codec_job *job) {
	struct meson_vdec_adapter *adapter = job->priv;
	struct amvdec_session *sess = &adapter->vdec_sess;
	struct amvdec_codec_ops *codec_ops = sess->fmt_out->codec_ops;

	sess->should_stop = 1;

	v4l2_m2m_mark_stopped(sess->m2m_ctx);

	if (codec_ops->drain) {
		vdec_wait_inactive(sess);
		codec_ops->drain(sess);
	} else if (codec_ops->eos_sequence) {
		u32 len;
		const u8 *data = codec_ops->eos_sequence(&len);

		esparser_queue_eos(sess->core, data, len);
		vdec_wait_inactive(sess);
	}

	v4l2_m2m_job_finish(job->session->core->m2m_dev, job->session->m2m_ctx);
}

static int meson_vdec_adapter_stop(struct meson_codec_job *job, struct vb2_queue *vq) {
	struct meson_vdec_adapter *adapter = job->priv;

	vdec_stop_streaming(&adapter->vdec_sess, vq);
	return 0;
}

static int meson_vdec_adapter_release(struct meson_codec_job *job) {
	struct meson_vdec_adapter *adapter = job->priv;

	vdec_close(&adapter->vdec_sess);

	free_irq(job->session->core->irqs[IRQ_PARSER], (void *)&adapter->vdec_core);
	free_irq(job->session->core->irqs[IRQ_VDEC], (void *)job);
	job->priv = NULL;
	kfree(adapter);

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
	.start = &meson_vdec_adapter_start,
	.queue = &meson_vdec_adapter_queue,
	.run = &meson_vdec_adapter_run,
	.abort = &meson_vdec_adapter_abort,
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

