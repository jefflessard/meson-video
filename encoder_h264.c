#include <linux/clk.h>
#include <linux/firmware.h>
#include <linux/iopoll.h>
#include <linux/regmap.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/videobuf2-core.h>
#include <media/v4l2-mem2mem.h>

#include "meson-formats.h"
#include "meson-codecs.h"
#include "meson-vcodec.h"

#include "amlogic.h"
#include "amlvenc_h264.h"
#include "register.h"

#define MHz (1000000)

struct encoder_h264_ctx {
	enum amlvenc_hcodec_encoder_status status;
};

static void amvenc_reset(void) {
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SC2) {
		hcodec_hw_reset();
	} else {
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		amlvenc_dos_sw_reset1(
			(1 << 2)  | (1 << 6)  |
			(1 << 7)  | (1 << 8)  |
			(1 << 14) | (1 << 16) |
			(1 << 17)
		);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
	}
}

static void amvenc_start(void) {
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SC2) {
		hcodec_hw_reset();
	} else {
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		amlvenc_dos_sw_reset1(
			(1 << 12) | (1 << 11)
		);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
	}

	amlvenc_hcodec_start();
}

static void amvenc_stop(void)
{
	ulong timeout = jiffies + HZ;

	amlvenc_hcodec_stop();

	while (!amlvenc_hcodec_dma_completed()) {
		if (time_after(jiffies, timeout))
			break;
	}

	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SC2) {
		hcodec_hw_reset();
	} else {
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		amlvenc_dos_sw_reset1(
			(1 << 12) | (1 << 11) |
			(1 << 2)  | (1 << 6)  |
			(1 << 7)  | (1 << 8)  |
			(1 << 14) | (1 << 16) |
			(1 << 17)
		);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
	}
}

static void amvenc_configure(void) {
	/* encode_monitor_thread */
	/* manager->inited == false */
	/* avc_init */
	/* amvenc_avc_start */
	// encode_manager.need_reset = true
	// avc_canvas_init
	// amvenc_reset();
	// amlvenc_h264_init_encoder
	// amlvenc_h264_init_input_dct_buffer
	// amlvenc_h264_init_output_stream_buffer
	/* avc_prot_init */
	// amlvenc_h264_configure_encoder
	/* amvenc_avc_start */
	// amlvenc_h264_init_dblk_buffer
	// amlvenc_h264_init_input_reference_buffer
	// amlvenc_h264_init_firmware_assist_buffer
	// amlvenc_h264_configure_ie_me
	// amlvenc_hcodec_clear_encoder_status
	// amlvenc_h264_configure_fixed_slice
	// amvenc_start();

	/* encode_monitor_thread */
	/* encode_process_request */
	// #ifdef H264_ENC_CBR

	/* amvenc_avc_start_cmd encode_manager.need_reset */
	// amvenc_stop
	// amvenc_reset
	// avc_canvas_init
	// amlvenc_h264_init_encoder
	// amlvenc_h264_init_input_dct_buffer
	// amlvenc_h264_init_output_stream_buffer
	/* avc_prot_init */
	// amlvenc_h264_configure_encoder
	/* amvenc_avc_start_cmd encode_manager.need_reset */
	// amlvenc_h264_init_firmware_assist_buffer

	/* amvenc_avc_start_cmd encode request */
	// amlvenc_h264_configure_svc_pic
	// amlvenc_h264_init_dblk_buffer
	// amlvenc_h264_init_input_reference_buffer
	/* set_input_format */
	// canvas_config_proxy
	// amlvenc_h264_configure_mdfin
	/* amvenc_avc_start_cmd encode request */
	// amlvenc_h264_configure_ie_me
	// amlvenc_h264_configure_fixed_slice
	// amlvenc_hcodec_set_encoder_status
	
	/* amvenc_avc_start_cmd encode_manager.need_reset */
	// amvenc_start();
}

static irqreturn_t hcodec_isr(int irq, void *data)
{
	const struct meson_codec_job *job = data;
	struct encoder_h264_ctx *ctx = job->priv;

	// read encoder status and clear interrupt
	enum amlvenc_hcodec_encoder_status status = amlvenc_hcodec_encoder_status();

	switch (status) {
		case ENCODER_IDLE:
		case ENCODER_SEQUENCE:
		case ENCODER_PICTURE:
		case ENCODER_IDR:
		case ENCODER_NON_IDR:
		case ENCODER_MB_HEADER:
		case ENCODER_MB_DATA:
		case ENCODER_MB_HEADER_DONE:
		case ENCODER_MB_DATA_DONE:
		case ENCODER_NON_IDR_INTRA:
		case ENCODER_NON_IDR_INTER:
		default:
			// ignore irq
			return IRQ_HANDLED;
		case ENCODER_SEQUENCE_DONE:
		case ENCODER_PICTURE_DONE:
		case ENCODER_IDR_DONE:
		case ENCODER_NON_IDR_DONE:
		case ENCODER_ERROR:
			// save status for threaded isr
			ctx->status = status;
			return IRQ_WAKE_THREAD;
	}
}

static irqreturn_t hcodec_threaded_isr(int irq, void *data)
{
	const struct meson_codec_job *job = data;
	struct encoder_h264_ctx *ctx = job->priv;

	switch (ctx->status) {
		case ENCODER_IDLE:
		case ENCODER_SEQUENCE:
		case ENCODER_PICTURE:
		case ENCODER_IDR:
		case ENCODER_NON_IDR:
		case ENCODER_MB_HEADER:
		case ENCODER_MB_DATA:
		case ENCODER_MB_HEADER_DONE:
		case ENCODER_MB_DATA_DONE:
		case ENCODER_NON_IDR_INTRA:
		case ENCODER_NON_IDR_INTER:
		default:
			// won't happen
			break;
		case ENCODER_SEQUENCE_DONE:
		case ENCODER_PICTURE_DONE:
		case ENCODER_IDR_DONE:
		case ENCODER_NON_IDR_DONE:
			// queue encoded bitstream
			break;
		case ENCODER_ERROR:
			// stop encoding and report error
			break;
	}

	return IRQ_HANDLED;
}

static int load_firmware(struct meson_codec_job *job) {
	struct meson_vcodec_session *session = job->session;
	struct meson_vcodec_core *core = session->core;
	struct device *dev = core->dev;
	const char *path = core->platform_specs->firmwares[job->codec->type];
	const struct firmware *fw;
	static void *fw_vaddr;
	static dma_addr_t fw_paddr;
	size_t fw_size;
	int ret;

	if (!path) {
		session_info(session, "Codec has no firmware");
		return 0;
	}

	//if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SC2) {
	//	fw_size = 4096 * 16;
	//} else {
		fw_size = 4096 * 8;
	//}

	fw_vaddr = dma_alloc_coherent(dev, fw_size, &fw_paddr, GFP_KERNEL);
	if (!fw_vaddr) {
		return -ENOMEM;
	}

	ret = request_firmware_into_buf(&fw, path, dev, fw_vaddr, fw_size);
	if (ret < 0) {
		session_err(session, "Failed to request firmware %s", path);
		goto free_dma;
	}

	/* >= AM_MESON_CPU_MAJOR_ID_SC2 */
	// amlvenc_hcodec_stop
	// get_firmware_data VIDEO_ENC_H264
	// amvdec_loadmc_ex VFORMAT_H264_ENC
	//	am_loadmc_ex 
	//	amvdec_loadmc 
	//	tee_load_video_fw VIDEO_ENC_H264 OPTEE_VDEC_HCDEC
	//	tee_load_firmware
	//	arm_smccc_smc TEE_SMC_LOAD_VIDEO_FW
	// return 0

	amlvenc_hcodec_stop();

	amlvenc_hcodec_dma_load_firmware(fw_paddr, fw_size);

	ret = read_poll_timeout(amlvenc_hcodec_dma_completed, ret, ret, 10000, 1000000, true);
	if (ret) {
		session_err(session, "Load firmware timed out (dma hang?)");
		goto release_firmware;
	}

	session_info(session, "Firmware %s loaded", path);

release_firmware:
	release_firmware(fw);
free_dma:
	dma_free_coherent(dev, fw_size, fw_vaddr, fw_paddr);
	return ret;
}

static int encoder_h264_init(struct meson_codec_job *job) {
	struct meson_vcodec_session *session = job->session;
	struct meson_vcodec_core *core = session->core;
	struct encoder_h264_ctx	*ctx;
	int ret;

	ctx = kcalloc(1, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
	job->priv = ctx;

	/* hcodec_clk */
	ret = meson_vcodec_clk_prepare(core, CLK_HCODEC, 667 * MHz);
	if (ret) {
		session_err(session, "Failed to enable HCODEC clock");
		goto free_ctx;
	}

	/* hcodec_rst */
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SC2) {
		if (!core->resets[RESET_HCODEC]) {
			session_err(session, "Failed to get HCODEC reset");
			ret = -EINVAL;
			goto disable_clk;
		}
	}

	/* hcodec power control */
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SC2) {
		// TODO vdec_poweron(VDEC_HCODEC) or pwr_ctrl_psci_smc PDID_T3_DOS_HCODEC
	} else {
		regmap_update_bits(core->regmaps[BUS_AO], AO_RTI_PWR_CNTL_REG0, BIT(4) | BIT(3), 0);
		usleep_range(10, 20);
	}

	/* Powerup HCODEC & Remove HCODEC ISO */
	ret = meson_vcodec_pwrc_on(core, PWRC_HCODEC);
	if (ret) {
		session_err(session, "Failed to power on HCODEC");
		goto disable_clk;
	}

	/* DOS_SW_RESET1 */
	amlvenc_dos_sw_reset1(0xffffffff);
	/* dos internal clock gating */
	amlvenc_dos_hcodec_gateclk(true);
	/* Powerup HCODEC memories */
	amlvenc_dos_hcodec_memory(true);
	/* disable auto-clock gate */
	amlvenc_dos_disable_auto_gateclk();
	/* enable hcodec assist */
	amlvenc_hcodec_assist_enable();

	/* load firmware */
	ret = load_firmware(job);
	if (ret) {
		session_err(session, "Failed to load firmware");
		goto disable_hcodec;
	}

	/* request irq */
	if (!core->irqs[IRQ_HCODEC]) {
		session_err(session, "Failed to get HCODEC irq");
		ret = -EINVAL;
		goto disable_hcodec;
	}
	ret = request_threaded_irq(session->core->irqs[IRQ_HCODEC], hcodec_isr, hcodec_threaded_isr, IRQF_SHARED, "hcodec", job);
	if (ret) {
		session_err(session, "Failed to request HCODEC irq");
		goto disable_hcodec;
	}

	return 0;

free_irq:
	free_irq(job->session->core->irqs[IRQ_HCODEC], (void *)job);
disable_hcodec:
	amlvenc_dos_hcodec_memory(false);
	amlvenc_dos_hcodec_gateclk(false);
pwrc_off:
	meson_vcodec_pwrc_off(core, PWRC_HCODEC);
disable_clk:
	meson_vcodec_clk_unprepare(core, CLK_HCODEC);
free_ctx:
	kfree(ctx);
	job->priv = NULL;
	return ret;
}

static int encoder_h264_start(struct meson_codec_job *job, struct vb2_queue *vq, u32 count) {

	return -EINVAL;

	if (IS_SRC_STREAM(vq->type)) {
		// configure encoder
		// encode queued frames
	}

	return 0;
}

static int encoder_h264_queue(struct meson_codec_job *job, struct vb2_v4l2_buffer *vb) {
	bool force_key_frame;

	if (IS_SRC_STREAM(vb->vb2_buf.type)) {
		// handle force key frame
		force_key_frame = meson_vcodec_g_ctrl(job->session, V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME);
		if (force_key_frame) {
			vb->flags |= V4L2_BUF_FLAG_KEYFRAME;
			vb->flags &= ~(V4L2_BUF_FLAG_PFRAME | V4L2_BUF_FLAG_BFRAME);
		}
	}

	// don't process buffer until started
	if (STREAM_STATUS(job->session, vb->vb2_buf.type) == STREAM_STATUS_INIT) {
		return 0;
	}

	if (IS_SRC_STREAM(vb->vb2_buf.type)) {
		// encode queued frames
	}

	return 0;
}

static void encoder_h264_resume(struct meson_codec_job *job) {
}

static void encoder_h264_abort(struct meson_codec_job *job) {
}

static int encoder_h264_stop(struct meson_codec_job *job, struct vb2_queue *vq) {
	return 0;
}

static int encoder_h264_release(struct meson_codec_job *job) {
	struct meson_vcodec_session *session = job->session;
	struct meson_vcodec_core *core = session->core;
	struct encoder_h264_ctx *ctx = job->priv;

	free_irq(job->session->core->irqs[IRQ_HCODEC], (void *)job);
	amlvenc_dos_hcodec_memory(false);
	amlvenc_dos_hcodec_gateclk(false);
	meson_vcodec_pwrc_off(core, PWRC_HCODEC);
	meson_vcodec_clk_unprepare(core, CLK_HCODEC);
	kfree(ctx);
	job->priv = NULL;

	return 0;
}

static const struct v4l2_std_ctrl h264_encoder_ctrls[] = {
	{ V4L2_CID_MPEG_VIDEO_B_FRAMES, 0, 16, 1, 2 },
	{ V4L2_CID_MPEG_VIDEO_BITRATE, 100000, 10000000, 100000, 4000000 },
	{ V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME, 0, 1, 1, 0 },
	{ V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE, 0, 1, 1, 1 },
	{ V4L2_CID_MPEG_VIDEO_GOP_SIZE, 1, 300, 1, 30 },
	{ V4L2_CID_MPEG_VIDEO_H264_MAX_QP, 0, 51, 1, 51 },
	{ V4L2_CID_MPEG_VIDEO_H264_MIN_QP, 0, 51, 1, 10 },
	{ V4L2_CID_MPEG_VIDEO_H264_PROFILE, 0, 4, 1, 0 },
	{ V4L2_CID_MPEG_VIDEO_HEADER_MODE, 0, 1, 1, 0 }
};

static const struct meson_codec_ops h264_encoder_ops = {
	.init = &encoder_h264_init,
	.start = &encoder_h264_start,
	.queue = &encoder_h264_queue,
	.resume = &encoder_h264_resume,
	.abort = &encoder_h264_abort,
	.stop = &encoder_h264_stop,
	.release = &encoder_h264_release,
};

const struct meson_codec_spec h264_encoder = {
	.type = H264_ENCODER,
	.ops = &h264_encoder_ops,
	.ctrls = h264_encoder_ctrls,
	.num_ctrls = ARRAY_SIZE(h264_encoder_ctrls),
};

