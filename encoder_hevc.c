#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/firmware.h>
#include <linux/iopoll.h>
#include <linux/regmap.h>
#include <linux/soc/amlogic/meson-canvas.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-contig.h>

#include "meson-formats.h"
#include "meson-codecs.h"
#include "meson-vcodec.h"

#include "amlogic.h"
#include "register.h"

#define MHz (1000000)


struct encoder_task_ctx {
	struct vb2_v4l2_buffer *src_buf;
	struct vb2_v4l2_buffer *dst_buf;
};

struct encoder_hevc_ctx {
	struct meson_codec_job *job;
	struct meson_vcodec_session *session;
	struct v4l2_m2m_ctx *m2m_ctx;
	struct completion encoder_completion;

	struct encoder_task_ctx task;

	bool is_next_idr;
};

static void encoder_loop(struct encoder_hevc_ctx *ctx);
static void encoder_abort(struct encoder_hevc_ctx *ctx);

static void encoder_done(struct encoder_hevc_ctx *ctx) {
	struct encoder_task_ctx *t = &ctx->task;
}

static void encoder_abort(struct encoder_hevc_ctx *ctx) {
	struct encoder_task_ctx *t = &ctx->task;
}

static void encoder_start(struct encoder_hevc_ctx *ctx) {
	struct encoder_task_ctx *t = &ctx->task;
}

static void encoder_loop(struct encoder_hevc_ctx *ctx) {
	struct encoder_task_ctx *t = &ctx->task;
}

static irqreturn_t hcodec_isr(int irq, void *data)
{
	const struct meson_codec_job *job = data;
	struct encoder_hevc_ctx *ctx = job->priv;
	struct encoder_task_ctx *t = &ctx->task;

	return IRQ_NONE;
}

static int encoder_hevc_prepare(struct meson_codec_job *job) {
	struct meson_vcodec_session *session = job->session;
	struct encoder_hevc_ctx	*ctx;
	int ret;

	ctx = kcalloc(1, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
	ctx->job = job;
	ctx->session = session;
	ctx->m2m_ctx = session->m2m_ctx;
	init_completion(&ctx->encoder_completion);
	job->priv = ctx;

	ret = request_irq(session->core->irqs[IRQ_WAVE420L], hcodec_isr, IRQF_SHARED, "hcodec", job);
	if (ret) {
		job_err(job, "Failed to request WAVE420L irq\n");
		goto free_ctx;
	}

	return 0;

free_ctx:
	kfree(ctx);
	job->priv = NULL;
	return ret;
}

static int encoder_hevc_init(struct meson_codec_dev *codec) {
	struct meson_vcodec_core *core = codec->core;
	int ret;

	ret = meson_vcodec_clk_prepare(core, CLK_WAVE420L, 667 * MHz);
	if (ret) {
		dev_err(core->dev, "Failed to enable WAVE420L clock\n");
		return ret;
	}

#if 0
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SC2) {
		if (!core->resets[RESET_WAVE420L]) {
			dev_err(core->dev, "Failed to get WAVE420L reset\n");
			ret = -EINVAL;
			goto disable_clk;
		}
	}
#endif

	/* Powerup WAVE420L & Remove WAVE420L ISO */
	ret = meson_vcodec_pwrc_on(core, PWRC_WAVE420L);
	if (ret) {
		dev_err(core->dev, "Failed to power on WAVE420L\n");
		goto disable_clk;
	}

	usleep_range(10, 20);

	uint32_t data32;
	if (get_cpu_major_id() <= AM_MESON_CPU_MAJOR_ID_TXLX) {
		data32 = 0x700;
		data32 |= READ_VREG(DOS_SW_RESET4);
		WRITE_VREG(DOS_SW_RESET4, data32);
		data32 &= ~0x700;
		WRITE_VREG(DOS_SW_RESET4, data32);
	} else {
		data32 = 0xf00;
		data32 |= READ_VREG(DOS_SW_RESET4);
		WRITE_VREG(DOS_SW_RESET4, data32);
		data32 &= ~0xf00;
		WRITE_VREG(DOS_SW_RESET4, data32);
	}

	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SC2) {
		// consider using reset control
	} else {
		WRITE_MPEG_REG(RESET0_REGISTER, data32 & ~(1<<21));
		WRITE_MPEG_REG(RESET0_REGISTER, data32 | (1<<21));
		READ_MPEG_REG(RESET0_REGISTER);
		READ_MPEG_REG(RESET0_REGISTER);
		READ_MPEG_REG(RESET0_REGISTER);
		READ_MPEG_REG(RESET0_REGISTER);
	}


	/* Enable wave420l_vpu_idle_rise_irq,
	 *	Disable wave420l_vpu_idle_fall_irq
	 */
	WRITE_VREG(DOS_WAVE420L_CNTL_STAT, 0x1);
	WRITE_VREG(DOS_MEM_PD_WAVE420L, 0x0);

	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SC2) {

	} else {
		WRITE_AOREG(AO_RTI_GEN_PWR_ISO0,
				READ_AOREG(AO_RTI_GEN_PWR_ISO0) &
				(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_SM1
				 ? ~0x8 : ~(0x3<<12)));
	}

	/* request irq */
	if (!core->irqs[IRQ_WAVE420L]) {
		dev_err(core->dev, "Failed to get WAVE420L irq\n");
		ret = -EINVAL;
		goto disable_hcodec;
	}

	return 0;

disable_hcodec:
//	amlvenc_dos_hcodec_memory(false);
//pwrc_off:
	meson_vcodec_pwrc_off(core, PWRC_WAVE420L);
disable_clk:
	meson_vcodec_clk_unprepare(core, CLK_WAVE420L);
	return ret;
}

static int encoder_hevc_start(struct meson_codec_job *job, struct vb2_queue *vq, u32 count) {
	struct encoder_hevc_ctx *ctx = job->priv;

	v4l2_m2m_update_start_streaming_state(ctx->m2m_ctx, vq);

	return 0;
}

static int encoder_hevc_queue(struct meson_codec_job *job, struct vb2_v4l2_buffer *vb) {
	struct encoder_hevc_ctx *ctx = job->priv;

	if (IS_SRC_STREAM(vb->vb2_buf.type)) {
		// handle force key frame
		if (ctx->is_next_idr) {
			job_info(job, "ts=%llu, force_key_frame=%d\n", vb->vb2_buf.timestamp, ctx->is_next_idr);
			vb->flags |= V4L2_BUF_FLAG_KEYFRAME;
			vb->flags &= ~(V4L2_BUF_FLAG_PFRAME | V4L2_BUF_FLAG_BFRAME);
			ctx->is_next_idr = false;
		}
	}

	return 0;
}

static void encoder_hevc_run(struct meson_codec_job *job) {
	struct encoder_hevc_ctx *ctx = job->priv;

	encoder_start(ctx);
}

static void encoder_hevc_abort(struct meson_codec_job *job) {
	struct encoder_hevc_ctx *ctx = job->priv;

	// v4l2-m2m may call abort even if stopped
	// and job finished earlier with error state
	// let finish the job once again to prevent
	// v4l2-m2m hanging forever
	if (v4l2_m2m_has_stopped(ctx->m2m_ctx)) {
		v4l2_m2m_job_finish(ctx->m2m_ctx->m2m_dev, ctx->m2m_ctx);
	}
}

static int encoder_hevc_stop(struct meson_codec_job *job, struct vb2_queue *vq) {
	struct encoder_hevc_ctx *ctx = job->priv;

	v4l2_m2m_update_stop_streaming_state(ctx->m2m_ctx, vq);

	if (V4L2_TYPE_IS_OUTPUT(vq->type) &&
			v4l2_m2m_has_stopped(ctx->m2m_ctx))
		meson_vcodec_event_eos(ctx->session);

	return 0;
}

static void encoder_hevc_unprepare(struct meson_codec_job *job) {
	struct encoder_hevc_ctx *ctx = job->priv;

	free_irq(job->session->core->irqs[IRQ_WAVE420L], (void *)job);

	kfree(ctx);
	job->priv = NULL;
}

static void encoder_hevc_release(struct meson_codec_dev *codec) {

	WRITE_VREG(DOS_MEM_PD_WAVE420L, 0xffffffff);
	//amlvenc_dos_hcodec_memory(false);
	meson_vcodec_pwrc_off(codec->core, PWRC_WAVE420L);
	meson_vcodec_clk_unprepare(codec->core, CLK_WAVE420L);
}

static int hevc_encoder_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct meson_codec_job *job = (struct meson_codec_job *)ctrl->priv;
	struct encoder_hevc_ctx *ctx = job->priv;
	job_dbg(job, "FORCE_KEY_FRAME");

	/* ctx may have not been created yet since first frame has not been queued yet */
	if (ctx) {
		ctx->is_next_idr = true;
	}
	return 0;
}

static const struct v4l2_ctrl_ops hevc_encoder_ctrl_ops = {
	.s_ctrl = hevc_encoder_s_ctrl,
};

static const struct v4l2_ctrl_config hevc_encoder_ctrls[] = {
//	V4L2_CTRL(B_FRAMES, 0, 16, 1, 2 ),
	V4L2_CTRL(GOP_SIZE, 1, 300, 1, 30 ),
	V4L2_CTRL(BITRATE, 100000, 100000000, 100000, 2000000 ),
	V4L2_CTRL(FRAME_RC_ENABLE, 0, 1, 1, 1 ),
	V4L2_CTRL(HEADER_MODE, 0, 1, 0, 0 ),
	V4L2_CTRL(REPEAT_SEQ_HEADER, 0, 1, 1, 0 ),
	V4L2_CTRL_OPS(FORCE_KEY_FRAME, &hevc_encoder_ctrl_ops, 0, 0, 0, 0 ),
	V4L2_CTRL(H264_MIN_QP, 0, 51, 1, 10 ),
	V4L2_CTRL(H264_MAX_QP, 0, 51, 1, 51 ),
//	V4L2_CTRL(H264_PROFILE, 0, 4, 1, 0 ),
};

static const struct meson_codec_ops hevc_encoder_ops = {
	.init = &encoder_hevc_init,
	.prepare = &encoder_hevc_prepare,
	.start = &encoder_hevc_start,
	.queue = &encoder_hevc_queue,
	.run = &encoder_hevc_run,
	.abort = &encoder_hevc_abort,
	.stop = &encoder_hevc_stop,
	.unprepare = &encoder_hevc_unprepare,
	.release = &encoder_hevc_release,
};

const struct meson_codec_spec hevc_encoder = {
	.type = HEVC_ENCODER,
	.name = "hevc_encoder",
	.ops = &hevc_encoder_ops,
	.ctrls = hevc_encoder_ctrls,
	.num_ctrls = ARRAY_SIZE(hevc_encoder_ctrls),
};

