#include <linux/clk.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/videobuf2-core.h>
#include <media/v4l2-mem2mem.h>

#include "meson-formats.h"
#include "meson-codecs.h"
#include "meson-vcodec.h"

#include "encoder_h264_hwops.h"


#define MHz (1000000)

struct meson_vcodec_core *MESON_VCODEC_CORE;

inline u8 get_cpu_type(void) {
	return MESON_VCODEC_CORE->platform_specs->platform_id;
}

inline void WRITE_HREG(u32 reg, u32 val) {
	meson_vcodec_reg_write(MESON_VCODEC_CORE, DOS_BASE, reg, val);
}

inline u32 READ_HREG(u32 reg) {
	return meson_vcodec_reg_read(MESON_VCODEC_CORE, DOS_BASE, reg);
}

inline void WRITE_VREG(u32 reg, u32 val) {
	meson_vcodec_reg_write(MESON_VCODEC_CORE, DOS_BASE, reg, val);
}

inline u32 READ_VREG(u32 reg) {
	return meson_vcodec_reg_read(MESON_VCODEC_CORE, DOS_BASE, reg);
}

inline s32 hcodec_hw_reset(void) {
	return meson_vcodec_reset(MESON_VCODEC_CORE, RESET_HCODEC);
}

	
int encoder_h264_init(struct meson_codec_job *job) {
	struct meson_vcodec_session *session = job->session;
	struct meson_vcodec_core *core = session->core;
	int ret;

	MESON_VCODEC_CORE = core;

	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_SC2) {
		ret = meson_vcodec_clk_prepare(core, CLK_HCODEC, 667 * MHz);
		if (ret) {
			session_err(session, "Failed to init HCODEC clock");
			return ret;
		}
	}

	/* avc_poweron */
	// enable vdec power domain (switch gate)
	// enable hcodec clock
	// power on vdec_hcodec / dos_hcodec / hcodec power domain (aoreg)
	// sw reset dos / internal dos clock gating / dos clock level
	// power up HCODEC memory
	// Remove HCODEC ISO
	// Disable auto-clock gate

	// init canvas
	// enable hcodec assist
	// load firmware
	// init encoder
	// init input buffer
	// init output buffer
	// configure encoder parameters (vlc, qdtc, qp, cbr, ie/me, mv, sad, etc.)
	// configure encoder irq
	// configure decoder buffer
	// configure reference buffer
	// configure assist buffer
	// init ie/me
	// init encoder status
	// configure slices
	
   /* amvenc_start */	

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

	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_SC2) {
		 meson_vcodec_clk_unprepare(core, CLK_HCODEC);
	}

	MESON_VCODEC_CORE = NULL;

	return 0;
}

static const struct v4l2_ctrl_config h264_encoder_ctrls[] = {
};

static const struct v4l2_ctrl_ops h264_encoder_ctrl_ops = {
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
	.ctrl_ops = &h264_encoder_ctrl_ops,
	.ctrls = h264_encoder_ctrls,
	.num_ctrls = ARRAY_SIZE(h264_encoder_ctrls),
};

