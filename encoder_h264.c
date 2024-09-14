#include <linux/clk.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/videobuf2-core.h>
#include <media/v4l2-mem2mem.h>

#include "meson-formats.h"
#include "meson-codecs.h"
#include "meson-vcodec.h"

#include "amlogic.h"
#include "amlvenc_h264.h"

#define MHz (1000000)

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

int encoder_h264_init(struct meson_codec_job *job) {
	struct meson_vcodec_session *session = job->session;
	struct meson_vcodec_core *core = session->core;
	int ret;

	/* amvenc_avc_probe */
	// hcodec_clk_prepare hcodec_aclk
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SC2) {
		ret = meson_vcodec_clk_prepare(core, CLK_HCODEC, 667 * MHz);
		if (ret) {
			session_err(session, "Failed to enable HCODEC clock");
			return ret;
		}
	}
	// hcodec_rst
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SC2) {
		if (!core->resets[RESET_HCODEC]) {
			session_err(session, "Failed to get HCODEC reset");
			return -EINVAL;
		}
	}
	// irq
	if (!core->resets[RESET_HCODEC]) {
		session_err(session, "Failed to get HCODEC reset");
		return -EINVAL;
	}

	/* encode_wq_init */
	// amlvenc_h264_init_me
	/* encode_start_monitor */
	/* encode_monitor_thread */
	/* manager->inited == false */
	/* avc_init */
	/* amvenc_avc_start */
	/* avc_poweron */

	/* avc_poweron */
	// switch gate vdec
	ret = meson_vcodec_pwrc_on(core, PWRC_VDEC);
	if (ret) {
		session_err(session, "Failed to power on VDEC");
		return ret;
	}
	// hcodec_clk_config: clk_enable hcodec_aclk
	// 	handled in previous meson_vcodec_clk_prepare
	// TODO vdec_poweron(VDEC_HCODEC) or pwr_ctrl_psci_smc PDID_T3_DOS_HCODEC
	// or
	// TODO AO_RTI_PWR_CNTL_REG0 BITS 3 and 4
	// Powerup HCODEC: AO_RTI_GEN_PWR_SLEEP0 BIT 0 or BITS 0 and 1
	// Remove HCODEC ISO: AO_RTI_GEN_PWR_ISO0 BIT 0 or BITS 4 and 5
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SC2) {
	} else {
		ret = meson_vcodec_pwrc_on(core, PWRC_HCODEC);
		if (ret) {
			session_err(session, "Failed to power on HCODEC");
			return ret;
		}
	}
	// DOS_SW_RESET1
	amlvenc_dos_sw_reset1(0xffffffff);
	// Enable Dos internal clock gating
	// Powerup HCODEC memories
	amlvenc_dos_hcodec_enable(6);
	// Remove HCODEC ISO
	//	managed by previous meson_vcodec_pwrc_on
	//	TODO might be an issue when powering up HCODEC mem
	// Disable auto-clock gate	
	amlvenc_dos_disable_auto_clock_gate();

	/* amvenc_avc_start */
	// enable hcodec assist
	amlvenc_hcodec_assist_enable();
	// TODO load firmware

	// TODO request irq
	
	return 0;
}

int encoder_h264_start(struct meson_codec_job *job, struct vb2_queue *vq, u32 count) {
	return 0;
}

int encoder_h264_queue(struct meson_codec_job *job, struct vb2_buffer *vb) {
	return 0;
}

void encoder_h264_run(struct meson_codec_job *job) {
}

void encoder_h264_abort(struct meson_codec_job *job) {
}

int encoder_h264_stop(struct meson_codec_job *job, struct vb2_queue *vq) {
	return 0;
}

int encoder_h264_release(struct meson_codec_job *job) {
	struct meson_vcodec_session *session = job->session;
	struct meson_vcodec_core *core = session->core;

	meson_vcodec_clk_unprepare(core, CLK_VDEC1);
	meson_vcodec_pwrc_off(core, PWRC_HCODEC);

	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SC2) {
		 meson_vcodec_clk_unprepare(core, CLK_HCODEC);
	}

	MESON_VCODEC_CORE = NULL;

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
	.run = &encoder_h264_run,
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

