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

/* PARSER_CONTROL */
	#define ES_PACK_SIZE_BIT	8
	#define ES_WRITE		BIT(5)
	#define ES_SEARCH		BIT(1)
	#define ES_PARSER_START		BIT(0)
/* PARSER_CONFIG */
	#define PS_CFG_MAX_FETCH_CYCLE_BIT	0
	#define PS_CFG_STARTCODE_WID_24_BIT	10
	#define PS_CFG_MAX_ES_WR_CYCLE_BIT	12
	#define PS_CFG_PFIFO_EMPTY_CNT_BIT	16
/* PARSER_SEARCH_PATTERN */
	#define ES_START_CODE_PATTERN 0x00000100
/* PARSER_SEARCH_MASK */
	#define ES_START_CODE_MASK	0xffffff00
	#define FETCH_ENDIAN_BIT	27
/* PARSER_INT_ENABLE */
	#define PARSER_INT_HOST_EN_BIT	8
/* PARSER_INT_STATUS */
	#define PARSER_INTSTAT_SC_FOUND	1

#define SEARCH_PATTERN_LEN	512

#define MC_SIZE         (16 * SZ_1K)
#define SIZE_EXT_FW     (20 * SZ_1K)
#define SIZE_WORKSPACE    0x1ee000
#define SIZE_SEI        ( 8 * SZ_1K)

/* Size of Motion Vector per macroblock */
#define MB_MV_SIZE	96

enum decoder_h264_buffers : u8 {
	BUF_EXT_FW,
	BUF_WORKSPACE,
	BUF_SEI,
	BUF_REF,
	MAX_BUFFERS,
};

struct decoder_task_ctx {
	struct vb2_v4l2_buffer *src_buf;
	struct vb2_v4l2_buffer *dst_buf;
};

struct decoder_h264_ctx {
	struct meson_codec_job *job;
	struct meson_vcodec_session *session;
	struct v4l2_m2m_ctx *m2m_ctx;
	struct completion decoder_completion;

	struct decoder_task_ctx task;
	struct meson_vcodec_buffer buffers[MAX_BUFFERS];

	bool is_next_idr;
};

static void decoder_loop(struct decoder_h264_ctx *ctx);
static void decoder_abort(struct decoder_h264_ctx *ctx);

static void decoder_done(struct decoder_h264_ctx *ctx) {
	struct decoder_task_ctx *t = &ctx->task;
}

static void decoder_abort(struct decoder_h264_ctx *ctx) {
	struct decoder_task_ctx *t = &ctx->task;
}

static void decoder_start(struct decoder_h264_ctx *ctx) {
	struct decoder_task_ctx *t = &ctx->task;
}

static void decoder_loop(struct decoder_h264_ctx *ctx) {
	struct decoder_task_ctx *t = &ctx->task;
}

static irqreturn_t vdec1_isr(int irq, void *data)
{
	const struct meson_codec_job *job = data;
	struct decoder_h264_ctx *ctx = job->priv;
	struct decoder_task_ctx *t = &ctx->task;

	return IRQ_NONE;
}

static irqreturn_t parser_isr(int irq, void *data)
{
	const struct meson_codec_job *job = data;
	struct decoder_h264_ctx *ctx = job->priv;
	struct decoder_task_ctx *t = &ctx->task;

	return IRQ_NONE;
}

static int vdec1_load_firmware(struct meson_vcodec_core *core, dma_addr_t paddr) {
	int ret;

	meson_dos_write(core, MPSR, 0);
	meson_dos_write(core, CPSR, 0);

	meson_dos_clear_bits(core, MDEC_PIC_DC_CTRL, BIT(31));

	meson_dos_write(core, IMEM_DMA_ADR, paddr);
	meson_dos_write(core, IMEM_DMA_COUNT, MC_SIZE >> 2);
	meson_dos_write(core, IMEM_DMA_CTRL, (0x8000 | (7 << 16)));

	bool is_dma_complete(void) {
		return !(meson_dos_read(core, IMEM_DMA_CTRL) & 0x8000);
	}
	return read_poll_timeout(
			is_dma_complete, ret, ret, 10000, 1000000, true);
}

static int load_firmware(struct decoder_h264_ctx *ctx) {
	struct meson_codec_dev *codec = ctx->job->codec;
	struct meson_vcodec_core *core = codec->core;
	struct meson_vcodec_buffer buf;
	int ret;

	buf.size = MC_SIZE + SIZE_EXT_FW;
	ret = meson_vcodec_request_firmware(codec, &buf);
	if (ret) {
		return ret;
	};

	vdec1_load_firmware(core, buf.paddr);
	if (ret) {
		codec_err(codec, "Load firmware timed out (dma hang?)\n");
		goto release_firmware;
	}

	memcpy(ctx->buffers[BUF_EXT_FW].vaddr, buf.vaddr + MC_SIZE, SIZE_EXT_FW);

release_firmware:
	meson_vcodec_release_firmware(codec, &buf);
	return ret;
}

int parser_start(struct decoder_h264_ctx *ctx)
{
	struct meson_vcodec_core *core = ctx->session->core;

	meson_vcodec_reset(core, RESET_PARSER);
	meson_parser_write(core, PARSER_CONFIG,
			(10 << PS_CFG_PFIFO_EMPTY_CNT_BIT) |
			(1  << PS_CFG_MAX_ES_WR_CYCLE_BIT) |
			(16 << PS_CFG_MAX_FETCH_CYCLE_BIT));

	meson_parser_write(core, PFIFO_RD_PTR, 0);
	meson_parser_write(core, PFIFO_WR_PTR, 0);

	meson_parser_write(core, PARSER_SEARCH_PATTERN,
			ES_START_CODE_PATTERN);
	meson_parser_write(core, PARSER_SEARCH_MASK, ES_START_CODE_MASK);

	meson_parser_write(core, PARSER_CONFIG,
			(10 << PS_CFG_PFIFO_EMPTY_CNT_BIT) |
			(1  << PS_CFG_MAX_ES_WR_CYCLE_BIT) |
			(16 << PS_CFG_MAX_FETCH_CYCLE_BIT) |
			(2  << PS_CFG_STARTCODE_WID_24_BIT));

	meson_parser_write(core, PARSER_CONTROL,
			(ES_SEARCH | ES_PARSER_START));

#if 0
	meson_parser_write(core, PARSER_VIDEO_START_PTR, sess->vififo_paddr);
	meson_parser_write(core, PARSER_VIDEO_END_PTR,
			sess->vififo_paddr + sess->vififo_size - 8);
	meson_parser_write(core, PARSER_ES_CONTROL,
			meson_parser_read(core, PARSER_ES_CONTROL) & ~1);

	if (vdec_ops->conf_esparser)
		vdec_ops->conf_esparser(sess);

	amvdec_write_parser(core, PARSER_INT_STATUS, 0xffff);
	amvdec_write_parser(core, PARSER_INT_ENABLE,
			BIT(PARSER_INT_HOST_EN_BIT));
#endif
	return 0;
}

static int parser_init(struct meson_vcodec_core *core) {
	int ret
;
	ret = meson_vcodec_clk_prepare(core, CLK_PARSER, 0);
	if (ret) {
		dev_err(core->dev, "Failed to enable PARSER clock\n");
		return ret;
	}

	ret = meson_vcodec_clk_prepare(core, CLK_DOS, 0);
	if (ret) {
		dev_err(core->dev, "Failed to enable PARSER clock\n");
		goto disable_parser_clk;
	}

	return 0;

disable_parser_clk:
	meson_vcodec_clk_unprepare(core, CLK_PARSER);
	return ret;
}

static void parser_release(struct meson_vcodec_core *core) {
	meson_vcodec_clk_unprepare(core, CLK_DOS);
	meson_vcodec_clk_unprepare(core, CLK_PARSER);
}


static int vdec1_init(struct meson_vcodec_core *core)
{
	int ret;

	/* Configure the vdec clk to the maximum available */
	ret = meson_vcodec_clk_prepare(core, CLK_VDEC1, 667 * MHz);
	if (ret) {
		dev_err(core->dev, "Failed to enable VDEC1 clock\n");
		return ret;
	}

	/* Powerup VDEC1 & Remove VDEC1 ISO */
	ret = meson_vcodec_pwrc_on(core, PWRC_VDEC1);
	if (ret) {
		dev_err(core->dev, "Failed to power on VDEC1\n");
		goto disable_clk;
	}

	usleep_range(10, 20);

	/* Reset VDEC1 */
	meson_dos_write(core, DOS_SW_RESET0, 0xfffffffc);
	meson_dos_write(core, DOS_SW_RESET0, 0x00000000);

	/* Reset DOS top registers */
	meson_dos_write(core, DOS_VDEC_MCRCC_STALL_CTRL, 0);

	meson_dos_write(core, GCLK_EN, 0x3ff);
	meson_dos_clear_bits(core, MDEC_PIC_DC_CTRL, BIT(31));

#if 0
	vdec_1_stbuf_power_up(sess);

	ret = vdec_1_load_firmware(sess, sess->fmt_out->firmware_path);
	if (ret)
		goto stop;

	ret = codec_ops->start(sess);
	if (ret)
		goto stop;
#endif

	/* Enable IRQ */
	meson_dos_write(core, VDEC_ASSIST_MBOX1_CLR_REG, 1);
	meson_dos_write(core, VDEC_ASSIST_MBOX1_MASK, 1);

#if 0
	/* Enable 2-plane output */
	if (sess->pixfmt_cap == V4L2_PIX_FMT_NV12M)
		meson_dos_set_bits(core, MDEC_PIC_DC_CTRL, BIT(17));
	else
		meson_dos_clear_bits(core, MDEC_PIC_DC_CTRL, BIT(17));
#endif

	/* Enable firmware processor */
	meson_dos_write(core, MPSR, 1);
	/* Let the firmware settle */
	usleep_range(10, 20);

	return 0;

//disable_vdec1:
//pwrc_off:
//	meson_vcodec_pwrc_off(core, PWRC_VDEC1);
disable_clk:
	meson_vcodec_clk_unprepare(core, CLK_VDEC1);
	return ret;
}

static int vdec1_release(struct meson_vcodec_core *core)
{
	meson_dos_write(core, MPSR, 0);
	meson_dos_write(core, CPSR, 0);
	meson_dos_write(core, VDEC_ASSIST_MBOX1_MASK, 0);

	meson_dos_write(core, DOS_SW_RESET0, BIT(12) | BIT(11));
	meson_dos_write(core, DOS_SW_RESET0, 0);
	meson_dos_read(core, DOS_SW_RESET0);

	meson_vcodec_pwrc_off(core, PWRC_VDEC1);
	meson_vcodec_clk_unprepare(core, CLK_VDEC1);

#if 0
	if (sess->priv)
		codec_ops->stop(sess);
#endif

	return 0;
}

static int decoder_h264_prepare(struct meson_codec_job *job) {
	struct meson_vcodec_session *session = job->session;
	struct decoder_h264_ctx	*ctx;
	int ret;

	ctx = kcalloc(1, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
	ctx->job = job;
	ctx->session = session;
	ctx->m2m_ctx = session->m2m_ctx;
	init_completion(&ctx->decoder_completion);
	job->priv = ctx;

	// TODO replace with actual parsed size and refs
	u32 width = V4L2_FMT_WIDTH(job->src_fmt);
	u32 height = V4L2_FMT_HEIGHT(job->src_fmt);
	u32 canvas_width = ALIGN(width, 32);
	u32 canvas_height = ALIGN(height, 16);
	u32 mb_width = (width + 15) >> 4;
	u32 mb_height = (height + 15) >> 4;
	mb_width = ALIGN(mb_width, 4);
	mb_height = ALIGN(mb_height, 4);
	u32 mb_total = mb_width * mb_height;
	u32 max_ref = 16;

	ctx->buffers[BUF_EXT_FW].size = SIZE_EXT_FW;
	ctx->buffers[BUF_WORKSPACE].size = SIZE_WORKSPACE;
	ctx->buffers[BUF_SEI].size = SIZE_SEI;
	ctx->buffers[BUF_REF].size = mb_total * MB_MV_SIZE * max_ref;
	ret = meson_vcodec_buffers_alloc(job->codec, ctx->buffers, MAX_BUFFERS);
	if (ret) {
		job_err(job, "Failed to allocate buffer\n");
		goto free_ctx;
	}

	ret = load_firmware(ctx);
	if (ret) {
		job_err(job, "Failed to load firmware\n");
		goto free_buffers;
	}

	ret = request_irq(session->core->irqs[IRQ_PARSER], parser_isr, IRQF_SHARED, "parser", job);
	if (ret) {
		job_err(job, "Failed to request PARSER irq\n");
		goto free_buffers;
	}

	ret = request_irq(session->core->irqs[IRQ_MBOX1], vdec1_isr, IRQF_SHARED, "vdec1", job);
	if (ret) {
		job_err(job, "Failed to request MBOX1 irq\n");
		goto free_parser_irq;
	}

	return 0;

free_parser_irq:
	free_irq(job->session->core->irqs[IRQ_PARSER], (void *)job);
free_buffers:
	meson_vcodec_buffers_free(job->codec, ctx->buffers, MAX_BUFFERS);
free_ctx:
	kfree(ctx);
	job->priv = NULL;
	return ret;
}

static int decoder_h264_init(struct meson_codec_dev *codec) {
	struct meson_vcodec_core *core = codec->core;
	int ret;

	ret = parser_init(core);
	if (ret) {
		dev_err(core->dev, "Failed to init PARSER\n");
		return ret;
	}

	ret = vdec1_init(core);
	if (ret) {
		dev_err(core->dev, "Failed to init VDEC1\n");
		goto release_parser;
	}

	/* request irq */
	if (!core->irqs[IRQ_PARSER]) {
		dev_err(core->dev, "Failed to get PARSER irq\n");
		ret = -EINVAL;
		goto release_vdec1;
	}
	if (!core->irqs[IRQ_MBOX1]) {
		dev_err(core->dev, "Failed to get MBOX1 irq\n");
		ret = -EINVAL;
		goto release_vdec1;
	}

	return 0;

release_parser:
	parser_release(core);
release_vdec1:
	vdec1_release(core);
	return ret;
}

static int decoder_h264_start(struct meson_codec_job *job, struct vb2_queue *vq, u32 count) {
	struct decoder_h264_ctx *ctx = job->priv;

	v4l2_m2m_update_start_streaming_state(ctx->m2m_ctx, vq);

	return 0;
}

static int decoder_h264_queue(struct meson_codec_job *job, struct vb2_v4l2_buffer *vb) {
	struct decoder_h264_ctx *ctx = job->priv;

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

static void decoder_h264_run(struct meson_codec_job *job) {
	struct decoder_h264_ctx *ctx = job->priv;

	decoder_start(ctx);
}

static void decoder_h264_abort(struct meson_codec_job *job) {
	struct decoder_h264_ctx *ctx = job->priv;

	v4l2_m2m_job_finish(ctx->m2m_ctx->m2m_dev, ctx->m2m_ctx);

#if 0
	// v4l2-m2m may call abort even if stopped
	// and job finished earlier with error state
	// let finish the job once again to prevent
	// v4l2-m2m hanging forever
	if (v4l2_m2m_has_stopped(ctx->m2m_ctx)) {
		v4l2_m2m_job_finish(ctx->m2m_ctx->m2m_dev, ctx->m2m_ctx);
	}
#endif
}

static int decoder_h264_stop(struct meson_codec_job *job, struct vb2_queue *vq) {
	struct decoder_h264_ctx *ctx = job->priv;

	v4l2_m2m_update_stop_streaming_state(ctx->m2m_ctx, vq);

	if (V4L2_TYPE_IS_OUTPUT(vq->type) &&
			v4l2_m2m_has_stopped(ctx->m2m_ctx))
		meson_vcodec_event_eos(ctx->session);

	return 0;
}

static void decoder_h264_unprepare(struct meson_codec_job *job) {
	struct decoder_h264_ctx *ctx = job->priv;

	free_irq(job->session->core->irqs[IRQ_MBOX1], (void *)job);
	free_irq(job->session->core->irqs[IRQ_PARSER], (void *)job);
	meson_vcodec_buffers_free(job->codec, ctx->buffers, MAX_BUFFERS);
	kfree(ctx);
	job->priv = NULL;
}

static void decoder_h264_release(struct meson_codec_dev *codec) {
	vdec1_release(codec->core);
	parser_release(codec->core);
}

static int h264_decoder_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct meson_codec_job *job = (struct meson_codec_job *)ctrl->priv;
	struct decoder_h264_ctx *ctx = job->priv;
	job_dbg(job, "FORCE_KEY_FRAME");

	/* ctx may have not been created yet since first frame has not been queued yet */
	if (ctx) {
		ctx->is_next_idr = true;
	}
	return 0;
}

static const struct v4l2_ctrl_ops h264_decoder_ctrl_ops = {
	.s_ctrl = h264_decoder_s_ctrl,
};

static const struct v4l2_ctrl_config h264_decoder_ctrls[] = {
//	V4L2_CTRL(B_FRAMES, 0, 16, 1, 2 ),
	V4L2_CTRL(GOP_SIZE, 1, 300, 1, 30 ),
	V4L2_CTRL(BITRATE, 100000, 100000000, 100000, 2000000 ),
	V4L2_CTRL(FRAME_RC_ENABLE, 0, 1, 1, 1 ),
	V4L2_CTRL(HEADER_MODE, 0, 1, 0, 0 ),
	V4L2_CTRL(REPEAT_SEQ_HEADER, 0, 1, 1, 0 ),
	V4L2_CTRL_OPS(FORCE_KEY_FRAME, &h264_decoder_ctrl_ops, 0, 0, 0, 0 ),
	V4L2_CTRL(H264_MIN_QP, 0, 51, 1, 10 ),
	V4L2_CTRL(H264_MAX_QP, 0, 51, 1, 51 ),
//	V4L2_CTRL(H264_PROFILE, 0, 4, 1, 0 ),
};

static const struct meson_codec_ops h264_decoder_ops = {
	.init = &decoder_h264_init,
	.prepare = &decoder_h264_prepare,
	.start = &decoder_h264_start,
	.queue = &decoder_h264_queue,
	.run = &decoder_h264_run,
	.abort = &decoder_h264_abort,
	.stop = &decoder_h264_stop,
	.unprepare = &decoder_h264_unprepare,
	.release = &decoder_h264_release,
};

const struct meson_codec_spec h264_decoder = {
	.type = H264_DECODER,
	.name = "h264_decoder",
	.ops = &h264_decoder_ops,
	.ctrls = h264_decoder_ctrls,
	.num_ctrls = ARRAY_SIZE(h264_decoder_ctrls),
};

