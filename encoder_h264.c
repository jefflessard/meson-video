#include <linux/clk.h>
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
#include "amlvenc_h264.h"
#include "register.h"

#define MHz (1000000)

#define V4L2_CID(x) V4L2_CID_MPEG_VIDEO_##x
#define V4L2_FMT(x) V4L2_PIX_FMT_##x
#define ENC_FMT(x) MDFIN_IFMT_##x
#define ENC_EFMT(x) MDFIN_IFMT_EXTRA_##x
#define ENC_YUV(x) MDFIN_SUBSAMPLING_##x
#define MAX_NUM_CANVAS 3

struct encoder_h264_spec {
	u32 pixelformat;
	u8 num_canvas;
	u8 canvas_bits_per_pixel;
	u8 width_align;
	u8 height_align;
	enum mdfin_iformat_enum iformat;
	enum mdfin_ifmt_extra_enum ifmt_extra;
	enum mdfin_subsampling_enum subsampling;
	enum mdfin_r2y_mode_enum r2y_mode;
	enum mdfin_bpp_enum canvas0_bppx;
	enum mdfin_bpp_enum canvas0_bppy;
	enum mdfin_bpp_enum canvasN_bppx;
	enum mdfin_bpp_enum canvasN_bppy;
};

const struct encoder_h264_spec encoder_h264_specs[] = {
	{ V4L2_FMT(NV12M),   2, 8, 32, 16, ENC_FMT(NV12P),   ENC_EFMT(NONE), ENC_YUV(420), 0, 1, 1, 1, 0 },
	{ V4L2_FMT(YUV420M), 3, 8, 32, 16, ENC_FMT(YUV420P), ENC_EFMT(NONE), ENC_YUV(420), 0, 1, 1, 0, 0 },
};

enum encoder_h264_buffers : u8 {
	BUF_QDCT,
	BUF_DBLK_Y,
	BUF_DBLK_UV,
	BUF_REF_Y,
	BUF_REF_UV,
	BUF_ASSIST,
	BUF_DUMP_INFO,
	BUF_CBR_INFO,
	MAX_BUFFERS,
};

struct encoder_h264_buffer {
	size_t size;
	void* vaddr;
	dma_addr_t paddr;
};

enum encoder_h264_canvas_type : u8 {
	CANVAS_TYPE_DBLK,  // index 0, 1, 2
	CANVAS_TYPE_REF,   // index 3, 4, 5
	CANVAS_TYPE_INPUT, // index 6, 7, 8
	// CANVAS_TYPE_SCALE, // index 9, 10, 11
	MAX_CANVAS_TYPE
};

struct encoder_h264_ctx {
	struct meson_codec_job *job;
	struct meson_vcodec_session *session;
	struct v4l2_m2m_ctx *m2m_ctx;
	struct work_struct encoder_work;

	enum amlvenc_hcodec_encoder_status hcodec_status;

	struct vb2_v4l2_buffer *src_buf;
	struct vb2_v4l2_buffer *dst_buf;

	u32 src_frame_count;
	u32 dst_frame_count;

	struct encoder_h264_buffer buffers[MAX_BUFFERS];

	u8 canvases[MAX_CANVAS_TYPE][MAX_NUM_CANVAS];

	qp_table_t qtable;

	const struct encoder_h264_spec *encoder_spec;
	struct amlvenc_h264_init_encoder_params init_params;
	struct amlvenc_h264_qtable_params qtable_params;
	struct amlvenc_h264_me_params me_params;
	struct amlvenc_h264_configure_encoder_params conf_params;
	struct amlvenc_h264_mdfin_params mdfin_params;
};

static const struct encoder_h264_spec* find_encoder_spec(u32 pixelformat) {
	int i;

	for(i = 0; i < ARRAY_SIZE(encoder_h264_specs); i++) {
		if (encoder_h264_specs[i].pixelformat == pixelformat) {
			return &encoder_h264_specs[i];
		}
	}

	return NULL;
}

static int canvas_alloc(struct meson_vcodec_core *core, u8 canvases[], u8 num_canvas) {
	int i, ret;

	for (i = 0; i < num_canvas; i++) {
		ret = meson_canvas_alloc(core->canvas, &canvases[i]);
		if (ret)
			goto free_canvases;
	}

	return 0;

free_canvases:
	while (--i >= 0) {
		meson_canvas_free(core->canvas, canvases[i]);
	}
	return ret;
}

static void canvas_free(struct meson_vcodec_core *core, u8 canvases[], u8 num_canvas) {
	int i;

	for (i = 0; i < num_canvas; i++) {
		meson_canvas_free(core->canvas, canvases[i]);
	}
}

static int canvas_config(struct meson_vcodec_core *core, u8 canvas_index, u32 paddr, u32 width, u32 height) {

	return meson_canvas_config(
			core->canvas,
			canvas_index, paddr, width, height,
			MESON_CANVAS_WRAP_NONE, MESON_CANVAS_BLKMODE_LINEAR, 0
		);
}

static int wait_hcodec_dma_completed(void) {
	int ret;

	ret = read_poll_timeout(amlvenc_hcodec_dma_completed, ret, ret, 10000, 1000000, true);
	if (ret) {
		return -ETIMEDOUT;
	}

	return 0;
}

static int load_firmware(struct meson_vcodec_core *core) {
	struct device *dev = core->dev;
	const char *path = core->platform_specs->firmwares[H264_ENCODER];
	const struct firmware *fw;
	static void *fw_vaddr;
	static dma_addr_t fw_paddr;
	size_t fw_size;
	int ret;

	if (!path) {
		dev_info(dev, "h264 encoder: No firmware");
		return 0;
	}

	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SC2) {
		fw_size = 4096 * 16;
	} else {
		fw_size = 4096 * 8;
	}

	fw_vaddr = dma_alloc_coherent(dev, fw_size, &fw_paddr, GFP_KERNEL);
	if (!fw_vaddr) {
		return -ENOMEM;
	}

	ret = request_firmware_into_buf(&fw, path, dev, fw_vaddr, fw_size);
	if (ret < 0) {
		dev_err(dev, "h264 encoder: Failed to request firmware %s", path);
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

	amlvenc_hcodec_encode(false);

	amlvenc_hcodec_dma_load_firmware(fw_paddr, fw_size);

	ret = wait_hcodec_dma_completed();
	if (ret) {
		dev_err(dev, "h264 encoder: Load firmware timed out (dma hang?)");
		goto release_firmware;
	}

	dev_info(dev, "h264 encoder: Firmware %s loaded", path);

release_firmware:
	release_firmware(fw);
free_dma:
	dma_free_coherent(dev, fw_size, fw_vaddr, fw_paddr);
	return ret;
}

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

	amlvenc_hcodec_encode(true);
}

static void amvenc_stop(void)
{
	int ret;

	amlvenc_hcodec_encode(false);

	ret = wait_hcodec_dma_completed();
	if (ret) {
		pr_warn("HCODEC dma timed out (dma hang?)");
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

static void init_fixed_quant(struct encoder_h264_ctx *ctx, u32 quant) {
	qp_table_t *qtable = &ctx->qtable;

	memset(qtable, quant, sizeof(qp_table_t));
}

static void init_linear_quant(struct encoder_h264_ctx *ctx, u32 quant) {
	qp_table_t *qtable = &ctx->qtable;

	if (quant < 4)
		quant = 4;
	for (int i = 0; i < 8; i++) {
		memset(qtable + i, quant + (i - 4), 4);
		memset(qtable + sizeof(qp_table_t) / 4 / 3 + i, quant + (i - 4), 4);
		memset(qtable + sizeof(qp_table_t) / 4 / 3 * 2 + i, quant + (i - 4), 4);
	}	
}

static int init_mdfin(struct encoder_h264_ctx *ctx, bool is_idr) {
	struct amlvenc_h264_mdfin_params *p = &ctx->mdfin_params;
	struct vb2_buffer *vb2 = &ctx->src_buf->vb2_buf;
	const struct encoder_h264_spec *s = ctx->encoder_spec;
	enum mdfin_bpp_enum canvas_bppx, canvas_bppy;
	u32 canvas_w, canvas_h;
	dma_addr_t canvas_paddr;
	int i, ret;

	// input params	
	p->width = ALIGN(ctx->job->src_fmt->width, s->width_align);
	p->height = ALIGN(ctx->job->src_fmt->height, s->height_align);
	p->iformat = s->iformat;
	p->ifmt_extra = s->ifmt_extra;
	p->r2y_mode = s->r2y_mode;
	p->canv_idx0_bppx = s->canvas0_bppx;
	p->canv_idx0_bppy = s->canvas0_bppy;
	p->canv_idx1_bppx = s->canvasN_bppx;
	p->canv_idx1_bppy = s->canvasN_bppy;

	// output params
	p->oformat = MDFIN_SUBSAMPLING_420;
	p->interp_en = p->oformat > s->subsampling;
	p->dsample_en = p->oformat < s->subsampling;
	p->y_sampl_rate = MDFIN_YSAMPLE_8PX;

	// nr params
	// p->nr_mode = MDFIN_NR_DISABLED;
	p->nr_mode = is_idr ? MDFIN_NR_SNR : MDFIN_NR_3D;
	p->y_snr = &amlvenc_h264_snr_defaults;
	p->c_snr = &amlvenc_h264_snr_defaults;
	p->y_tnr = &amlvenc_h264_tnr_defaults;
	p->c_tnr = &amlvenc_h264_tnr_defaults;

	if (V4L2_TYPE_IS_MULTIPLANAR(vb2->type) && vb2->num_planes != s->num_canvas) {
		// TODO not supported
		return -EINVAL; 
	}

	for (i = 0; i < s->num_canvas; i++) {
		if (vb2->num_planes == s->num_canvas) {
			canvas_paddr = vb2_dma_contig_plane_dma_addr(vb2, i);
		} else {
			// TODO single plane to multi canvas paddr offset mapping
		}
	
		canvas_bppx = i == 0 ? s->canvas0_bppx : s->canvasN_bppx;
		canvas_bppy = i == 0 ? s->canvas0_bppy : s->canvasN_bppy;

		canvas_w = (p->width * s->canvas_bits_per_pixel) >> 3; // bytes = pixels / 8
		canvas_h = (p->height * s->canvas_bits_per_pixel) >> 3; // bytes = pixels / 8

		if (canvas_bppx == MDFIN_BPP_HALF) {
			canvas_w = canvas_w >> 1; // bpp==0: /2
		} else {
			canvas_w = canvas_w * canvas_bppx;
		}

		if (canvas_bppy == MDFIN_BPP_HALF) {
			canvas_h = canvas_h >> 1; // bpp==0: /2
		} else {
			canvas_h = canvas_h * canvas_bppy;
		}

		ret = canvas_config(
				ctx->session->core,
				ctx->canvases[CANVAS_TYPE_INPUT][i],
				canvas_paddr,
				ALIGN(canvas_w, s->width_align),
				ALIGN(canvas_h, s->height_align)
			);
		if (ret) {
			// TODO report config canvas failed
			return -EINVAL;
		}
	}

	p->input.canvas = COMBINE_CANVAS_ARRAY(
				ctx->canvases[CANVAS_TYPE_INPUT],
				s->num_canvas
			);

	return 0;
}

static int configure_encoder(struct encoder_h264_ctx *ctx) {
	dma_addr_t buf_paddr;
	u64 buf_size;
	bool is_idr;
	u32 quant, width, height;
	int ret;

	is_idr = (ctx->src_buf->flags & V4L2_BUF_FLAG_KEYFRAME) == V4L2_BUF_FLAG_KEYFRAME;
	quant = meson_vcodec_g_ctrl(ctx->session, V4L2_CID(H264_MIN_QP));
	if (!quant) quant = 26;
	width = ctx->job->src_fmt->width;
	height = ctx->job->src_fmt->height;

	// amlvenc_h264_init_encoder
	if (is_idr) {
		ctx->init_params.idr = is_idr;
		ctx->init_params.idr_pic_id = (ctx->init_params.idr_pic_id + 1) % 65536;
		ctx->init_params.frame_number = 1;
		ctx->init_params.pic_order_cnt_lsb = 2;
	} else {
		ctx->init_params.idr = is_idr;

// TODO H264_ENC_SVC
#if 0 //#ifdef H264_ENC_SVC
		/* only update when there is reference frame */
		if (wq->pic.enable_svc == 0 || wq->pic.non_ref_cnt == 0) {
			wq->pic.frame_number++;
			enc_pr(LOG_INFO, "Increase frame_num to %d\n",
					wq->pic.frame_number);
		}
#endif

		ctx->init_params.frame_number = (ctx->init_params.frame_number + 1) % 65536;
		ctx->init_params.pic_order_cnt_lsb += 2;
	}
	ctx->init_params.init_qppicture = quant;
	amlvenc_h264_init_encoder(&ctx->init_params);

	// amlvenc_h264_init_input_dct_buffer
	amlvenc_h264_init_input_dct_buffer(
			ctx->buffers[BUF_DBLK_Y].paddr,
			ctx->buffers[BUF_DBLK_Y].paddr + ctx->buffers[BUF_DBLK_Y].size - 1);

	// amlvenc_h264_init_output_stream_buffer
	buf_paddr = vb2_dma_contig_plane_dma_addr(&ctx->dst_buf->vb2_buf, 0);
	buf_size = vb2_plane_size(&ctx->dst_buf->vb2_buf, 0);
	amlvenc_h264_init_output_stream_buffer(buf_paddr, buf_paddr + buf_size - 1);

	// amlvenc_h264_configure_encoder
	ctx->conf_params.idr = is_idr;
	ctx->conf_params.quant = quant;
	ctx->conf_params.qp_mode = 1;
	ctx->conf_params.encoder_width = width;
	ctx->conf_params.encoder_height = height;
	ctx->conf_params.i4_weight = I4MB_WEIGHT_OFFSET;
	ctx->conf_params.i16_weight = I16MB_WEIGHT_OFFSET;
	ctx->conf_params.me_weight = ME_WEIGHT_OFFSET;
#ifdef H264_ENC_CBR
	ctx->conf_params.cbr_ddr_start_addr = ctx->buffers[BUF_CBR_INFO].paddr;
	ctx->conf_params.cbr_start_tbl_id = START_TABLE_ID;
	ctx->conf_params.cbr_short_shift = CBR_SHORT_SHIFT;
	ctx->conf_params.cbr_long_mb_num = CBR_LONG_MB_NUM;
	ctx->conf_params.cbr_long_th = CBR_LONG_THRESH;
	ctx->conf_params.cbr_block_w = 16;
	ctx->conf_params.cbr_block_h = 9;
#endif
	ctx->conf_params.dump_ddr_start_addr = ctx->buffers[BUF_DUMP_INFO].paddr;
	init_linear_quant(ctx, quant);
	amlvenc_h264_configure_encoder(&ctx->conf_params);

	// amlvenc_h264_init_firmware_assist_buffer
	amlvenc_h264_init_firmware_assist_buffer(ctx->buffers[BUF_ASSIST].paddr);


	/* amvenc_avc_start_cmd encode request */
	// amlvenc_h264_configure_svc_pic
// TODO H264_ENC_SVC
#if 0 //#ifdef H264_ENC_SVC
		/* encode non reference frame or not */
		if (request->cmd == ENCODER_IDR)
			wq->pic.non_ref_cnt = 0; //IDR reset counter

		if (wq->pic.enable_svc && wq->pic.non_ref_cnt) {
			enc_pr(LOG_INFO,
				"PIC is NON REF cmd %d cnt %d value %s\n",
				request->cmd, wq->pic.non_ref_cnt,
				"ENC_SLC_NON_REF");
			amlvenc_h264_configure_svc_pic(false);
		} else {
			enc_pr(LOG_INFO,
				"PIC is REF cmd %d cnt %d val %s\n",
				request->cmd, wq->pic.non_ref_cnt,
				"ENC_SLC_REF");
			amlvenc_h264_configure_svc_pic(true);			
		}
#else
		/* if FW defined but not defined SVC in driver here*/
		amlvenc_h264_configure_svc_pic(true);
#endif
	
	// amlvenc_h264_init_dblk_buffer
	amlvenc_h264_init_dblk_buffer(
			COMBINE_CANVAS_ARRAY(
				ctx->canvases[CANVAS_TYPE_DBLK],
				MAX_NUM_CANVAS
			));

	// amlvenc_h264_init_input_reference_buffer
	amlvenc_h264_init_input_reference_buffer(
			COMBINE_CANVAS_ARRAY(
				ctx->canvases[CANVAS_TYPE_REF],
				MAX_NUM_CANVAS
			));

	// amlvenc_h264_configure_mdfin
	ret = init_mdfin(ctx, is_idr);
	if (ret) {
		stream_err(ctx->session, ctx->src_buf->vb2_buf.type, "Failed to initialize MDFIN parameters");
		return ret;
	}
	amlvenc_h264_configure_mdfin(&ctx->mdfin_params);

	// amlvenc_h264_configure_ie_me
	amlvenc_h264_configure_ie_me(is_idr ? HENC_MB_Type_I4MB :
			(HENC_SKIP_RUN_AUTO << 16) |
			(HENC_MB_Type_AUTO  <<  4) |
			(HENC_MB_Type_AUTO  <<  0));

	// amlvenc_h264_configure_fixed_slice
	// TODO enable multi slice in I frames
	amlvenc_h264_configure_fixed_slice(height, 0, 0);

	// amlvenc_hcodec_set_encoder_status
	amlvenc_hcodec_set_encoder_status(is_idr ? ENCODER_IDR : ENCODER_NON_IDR);

	return 0;
}

static void encoder_start(struct encoder_h264_ctx *ctx) {
	int ret;

	// Ensure there are buffers to work on
	ctx->src_buf = v4l2_m2m_next_src_buf(ctx->m2m_ctx);
	ctx->dst_buf = v4l2_m2m_next_dst_buf(ctx->m2m_ctx);
	if (!ctx->src_buf || !ctx->dst_buf) {
		session_trace(ctx->session, "no buffers available");
		v4l2_m2m_job_finish(ctx->m2m_ctx->m2m_dev, ctx->m2m_ctx);
		ctx->src_buf = NULL;
		ctx->dst_buf = NULL;
		return;
	}

	session_trace(ctx->session);

	// Dequeue buffers
	v4l2_m2m_src_buf_remove_by_buf(ctx->m2m_ctx, ctx->src_buf);
	v4l2_m2m_dst_buf_remove_by_buf(ctx->m2m_ctx, ctx->dst_buf);

	// amvenc_stop
	amvenc_stop();

	// amvenc_reset
	amvenc_reset();

	// Configure encoding parameters
	ret = configure_encoder(ctx);
	if (ret) {
		v4l2_m2m_buf_done(ctx->src_buf, VB2_BUF_STATE_ERROR);
		v4l2_m2m_buf_done(ctx->dst_buf, VB2_BUF_STATE_ERROR);
		ctx->src_buf = NULL;
		ctx->dst_buf = NULL;
		v4l2_m2m_job_finish(ctx->m2m_ctx->m2m_dev, ctx->m2m_ctx);
		return;
	}

	// update stats
	ctx->src_frame_count++;

	// Start encoding
	amvenc_start();
}

static void encoder_done(struct encoder_h264_ctx *ctx) {
	session_trace(ctx->session);
	// return encoded bitstream buffer
	v4l2_m2m_buf_copy_metadata(ctx->src_buf, ctx->dst_buf, true);
	vb2_set_plane_payload(&ctx->dst_buf->vb2_buf, 0, amlvenc_hcodec_vlc_total_bytes());

	v4l2_m2m_buf_done(ctx->src_buf, VB2_BUF_STATE_DONE);
	v4l2_m2m_buf_done(ctx->dst_buf, VB2_BUF_STATE_DONE);
	ctx->src_buf = NULL;
	ctx->dst_buf = NULL;

	// update stats
	ctx->dst_frame_count++;

	session_trace(ctx->session,
			"mb info: 0x%x, encode status: 0x%x, dct status: 0x%x ",
			amlvenc_hcodec_mb_info(),
			ctx->hcodec_status,
			amlvenc_hcodec_qdct_status());
	session_trace(ctx->session,
			"vlc status: 0x%x, me status: 0x%x, risc pc:0x%x, debug:0x%x\n",
			amlvenc_hcodec_vlc_status(),
			amlvenc_hcodec_me_status(),
			amlvenc_hcodec_mpc_risc(),
			amlvenc_hcodec_debug());

	v4l2_m2m_job_finish(ctx->m2m_ctx->m2m_dev, ctx->m2m_ctx);
}

static void encoder_abort(struct encoder_h264_ctx *ctx) {
	bool job_finish = false;

	session_trace(ctx->session);
	
	amvenc_stop();

	if(ctx->src_buf) {
		job_finish = true;
		v4l2_m2m_buf_done(ctx->src_buf, VB2_BUF_STATE_ERROR);
		ctx->src_buf = NULL;
	}

	if(ctx->dst_buf) {
		job_finish = true;
		v4l2_m2m_buf_done(ctx->dst_buf, VB2_BUF_STATE_ERROR);
		ctx->dst_buf = NULL;
	}

	if (job_finish) {
		v4l2_m2m_job_finish(ctx->m2m_ctx->m2m_dev, ctx->m2m_ctx);
	}
}

static void encoder_work(struct work_struct *work) {
	struct encoder_h264_ctx *ctx = container_of(work, struct encoder_h264_ctx, encoder_work);

	session_trace(ctx->session, "hcodec_status %d", ctx->hcodec_status);

	if (ctx->hcodec_status == ENCODER_ERROR) {
		encoder_abort(ctx);
	} else {
		encoder_done(ctx);
	}
}

static irqreturn_t hcodec_isr(int irq, void *data)
{
	const struct meson_codec_job *job = data;
	struct encoder_h264_ctx *ctx = job->priv;

	// read encoder status and clear interrupt
	ctx->hcodec_status = amlvenc_hcodec_encoder_status();

	switch (ctx->hcodec_status) {
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
			return IRQ_NONE;
		case ENCODER_SEQUENCE_DONE:
		case ENCODER_PICTURE_DONE:
		case ENCODER_IDR_DONE:
		case ENCODER_NON_IDR_DONE:
		case ENCODER_ERROR:
			schedule_work(&ctx->encoder_work);
			return IRQ_HANDLED;
	}
}

static void buffers_free(struct device *dev, struct encoder_h264_buffer *buffers, int num_buffers) {
	int i;

	for (i = 0; i < num_buffers; i++) {
		if (buffers[i].size) {

			dma_free_coherent(dev, buffers[i].size, buffers[i].vaddr, buffers[i].paddr);
		}

		buffers[i].vaddr = NULL;
		buffers[i].paddr = 0;
	}
}

static int buffers_alloc(struct device *dev, struct encoder_h264_buffer *buffers, int num_buffers) {
	int i, ret;

	for (i = 0; i < num_buffers; i++) {
		if (buffers[i].size) {

			buffers[i].vaddr = dma_alloc_coherent(dev, buffers[i].size, &buffers[i].paddr, GFP_KERNEL);
			if (!buffers[i].vaddr) {
				ret = -ENOMEM;
				goto free_buffers;
			}
		}
	}

	return 0;

free_buffers:
	while (--i >= 0) {
		if (buffers[i].size) {
			dma_free_coherent(dev, buffers[i].size, buffers[i].vaddr, buffers[i].paddr);
		}
		buffers[i].vaddr = NULL;
		buffers[i].paddr = 0;
	}
	return ret;
}

static int __encoder_h264_configure(struct meson_codec_job *job) {
	struct meson_vcodec_session *session = job->session;
	struct encoder_h264_ctx	*ctx;
	int ret;

	ctx = kcalloc(1, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
	ctx->job = job;
	ctx->session = session;
	ctx->m2m_ctx = job->session->m2m_ctx;
	INIT_WORK(&ctx->encoder_work, encoder_work);
	ctx->init_params.init_qppicture = 26;
	ctx->init_params.log2_max_frame_num = 4;
	ctx->init_params.log2_max_pic_order_cnt_lsb = 4;
	ctx->init_params.idr_pic_id = 0;
	ctx->init_params.frame_number = 0;
	ctx->init_params.pic_order_cnt_lsb = 0;
	ctx->qtable_params.quant_tbl_i4 = (u32 *) &ctx->qtable.i4_qp;
	ctx->qtable_params.quant_tbl_i16 = (u32 *) &ctx->qtable.i16_qp;
	ctx->qtable_params.quant_tbl_me = (u32 *) &ctx->qtable.p16_qp;
	ctx->conf_params.qtable = &ctx->qtable_params;
	ctx->conf_params.me = &ctx->me_params;
	job->priv = ctx;

	u32 width = job->src_fmt->width;
	u32 height = job->src_fmt->height;
	u32 canvas_width = ((width + 31) >> 5) << 5;
	u32 canvas_height = ((height + 15) >> 4) << 4;
	u32 mb_width = (width + 15) >> 4;
	u32 mb_height = (height + 15) >> 4;

	ctx->buffers[BUF_QDCT].size = mb_width * mb_height * 80;
	ctx->buffers[BUF_ASSIST].size = 0xc0000;
	ctx->buffers[BUF_CBR_INFO].size = 0x2000;
	ctx->buffers[BUF_DUMP_INFO].size = mb_width * mb_height * 80;
	ctx->buffers[BUF_DBLK_Y].size = canvas_width * canvas_height;
	ctx->buffers[BUF_DBLK_UV].size = canvas_width * canvas_height / 2;
	ctx->buffers[BUF_REF_Y].size = canvas_width * canvas_height;
	ctx->buffers[BUF_REF_UV].size = canvas_width * canvas_height / 2;

	ctx->encoder_spec = find_encoder_spec(job->src_fmt->pixelformat);
	if (!ctx->encoder_spec) {
		session_err(session, "Failed to find spec of %.4s", (char *)&job->src_fmt->pixelformat);
		ret = -EINVAL;
		goto free_ctx;
	}

	ret = buffers_alloc(session->core->dev, ctx->buffers, MAX_BUFFERS);
	if (ret) {
		session_err(session, "Failed to allocate buffer");
		goto free_ctx;
	}
	
	/* alloc canvases */
	ret = canvas_alloc(session->core, &ctx->canvases[0][0], MAX_CANVAS_TYPE * MAX_NUM_CANVAS);
	if (ret) {
		session_err(session, "Failed to allocate canvases");
		goto free_buffers;
	}

	ret = canvas_config(
			session->core,
			ctx->canvases[CANVAS_TYPE_DBLK][0],
			ctx->buffers[BUF_DBLK_Y].paddr,
			canvas_width,
			canvas_height
		);
	if (ret) goto canvas_config_failed;
	ret = canvas_config(
			session->core,
			ctx->canvases[CANVAS_TYPE_DBLK][1],
			ctx->buffers[BUF_DBLK_UV].paddr,
			canvas_width,
			canvas_height / 2
		);
	if (ret) goto canvas_config_failed;
	ret = canvas_config(
			session->core,
			ctx->canvases[CANVAS_TYPE_DBLK][2],
			ctx->buffers[BUF_DBLK_UV].paddr,
			canvas_width,
			canvas_height / 2
		);
	if (ret) goto canvas_config_failed;
	ret = canvas_config(
			session->core,
			ctx->canvases[CANVAS_TYPE_REF][0],
			ctx->buffers[BUF_REF_Y].paddr,
			canvas_width,
			canvas_height
		);
	if (ret) goto canvas_config_failed;
	ret = canvas_config(
			session->core,
			ctx->canvases[CANVAS_TYPE_REF][1],
			ctx->buffers[BUF_REF_UV].paddr,
			canvas_width,
			canvas_height / 2
		);
	if (ret) goto canvas_config_failed;
	ret = canvas_config(
			session->core,
			ctx->canvases[CANVAS_TYPE_REF][2],
			ctx->buffers[BUF_REF_UV].paddr,
			canvas_width,
			canvas_height / 2
		);
	if (ret) goto canvas_config_failed;

	ret = request_irq(session->core->irqs[IRQ_HCODEC], hcodec_isr, IRQF_SHARED, "hcodec", job);
	if (ret) {
		session_err(session, "Failed to request HCODEC irq");
		goto free_canvas;
	}

	return 0;

canvas_config_failed:
	session_err(session, "Failed to configure buffer canvases");
	goto free_canvas;

//free_irq:
	free_irq(session->core->irqs[IRQ_HCODEC], (void *)job);
free_canvas:
	canvas_free(session->core, &ctx->canvases[0][0], MAX_CANVAS_TYPE * MAX_NUM_CANVAS);
free_buffers:
	buffers_free(session->core->dev, ctx->buffers, MAX_BUFFERS);
free_ctx:
	kfree(ctx);
	job->priv = NULL;
	return ret;
}

static int __encoder_h264_init(struct meson_vcodec_core *core) {
	int ret;

	/* hcodec_clk */
	ret = meson_vcodec_clk_prepare(core, CLK_HCODEC, 667 * MHz);
	if (ret) {
		dev_err(core->dev, "Failed to enable HCODEC clock");
		return ret;
	}

	/* hcodec_rst */
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SC2) {
		if (!core->resets[RESET_HCODEC]) {
			dev_err(core->dev, "Failed to get HCODEC reset");
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
		dev_err(core->dev, "Failed to power on HCODEC");
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
	ret = load_firmware(core);
	if (ret) {
		dev_err(core->dev, "Failed to load firmware");
		goto disable_hcodec;
	}

	/* request irq */
	if (!core->irqs[IRQ_HCODEC]) {
		dev_err(core->dev, "Failed to get HCODEC irq");
		ret = -EINVAL;
		goto disable_hcodec;
	}

	return 0;

disable_hcodec:
	amlvenc_dos_hcodec_memory(false);
	amlvenc_dos_hcodec_gateclk(false);
//pwrc_off:
	meson_vcodec_pwrc_off(core, PWRC_HCODEC);
disable_clk:
	meson_vcodec_clk_unprepare(core, CLK_HCODEC);
	return ret;
}

static int encoder_h264_start(struct meson_codec_job *job, struct vb2_queue *vq, u32 count) {
	return 0;
}

static int encoder_h264_queue(struct meson_codec_job *job, struct vb2_v4l2_buffer *vb) {
	bool force_key_frame;

	if (IS_SRC_STREAM(vb->vb2_buf.type)) {
		// handle force key frame
		force_key_frame = meson_vcodec_g_ctrl(job->session, V4L2_CID(FORCE_KEY_FRAME));
		if (force_key_frame) {
			vb->flags |= V4L2_BUF_FLAG_KEYFRAME;
			vb->flags &= ~(V4L2_BUF_FLAG_PFRAME | V4L2_BUF_FLAG_BFRAME);
		}
	}

	return 0;
}

static void encoder_h264_resume(struct meson_codec_job *job) {
	struct encoder_h264_ctx *ctx = job->priv;

	encoder_start(ctx);
}

static void encoder_h264_abort(struct meson_codec_job *job) {
}

static int encoder_h264_stop(struct meson_codec_job *job, struct vb2_queue *vq) {
	return 0;
}

static void __encoder_h264_unconfigure(struct meson_codec_job *job) {
	struct encoder_h264_ctx *ctx = job->priv;

	free_irq(job->session->core->irqs[IRQ_HCODEC], (void *)job);

	buffers_free(job->session->core->dev, ctx->buffers, MAX_BUFFERS);
	canvas_free(job->session->core, &ctx->canvases[0][0], MAX_CANVAS_TYPE * MAX_NUM_CANVAS);

	kfree(ctx);
	job->priv = NULL;
}

static void __encoder_h264_release(struct meson_vcodec_core *core) {

	amlvenc_dos_hcodec_memory(false);
	amlvenc_dos_hcodec_gateclk(false);
	meson_vcodec_pwrc_off(core, PWRC_HCODEC);
	meson_vcodec_clk_unprepare(core, CLK_HCODEC);
}

static int encoder_h264_init(struct meson_codec_job *job) {
	int ret;

	ret = __encoder_h264_init(job->session->core);
	if (ret) {
		return ret;
	}

	ret = __encoder_h264_configure(job);
	if (ret) {
		__encoder_h264_release(job->session->core);
		return ret;
	}
	
	return 0;
}

static int encoder_h264_release(struct meson_codec_job *job) {
	__encoder_h264_unconfigure(job);
	__encoder_h264_release(job->session->core);
	return 0;
}

static const struct v4l2_std_ctrl h264_encoder_ctrls[] = {
//	{ V4L2_CID(B_FRAMES), 0, 16, 1, 2 },
//	{ V4L2_CID(BITRATE), 100000, 10000000, 100000, 4000000 },
	{ V4L2_CID(FORCE_KEY_FRAME), 0, 1, 1, 0 },
//	{ V4L2_CID(FRAME_RC_ENABLE), 0, 1, 1, 1 },
//	{ V4L2_CID(GOP_SIZE), 1, 300, 1, 30 },
//	{ V4L2_CID(H264_MAX_QP), 0, 51, 1, 51 },
	{ V4L2_CID(H264_MIN_QP), 0, 51, 1, 10 },
//	{ V4L2_CID(H264_PROFILE), 0, 4, 1, 0 },
//	{ V4L2_CID(HEADER_MODE), 0, 1, 1, 0 }
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

