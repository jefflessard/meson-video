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
#include "amlvenc_h264.h"
#include "amlvenc_h264_info.h"
#include "register.h"

#define MHz (1000000)
#define ENCODER_TIMEOUT_MS 5000
#define MAX_NUM_CANVAS 3
#define INITIAL_QP 16

static const qp_union_t qp_adj_idr = {
	.rows = {
		0x00000000, 0x01010101,
		0x02020202, 0x03030303,
		0x04040404, 0x05050505,
		0x06060606, 0x07070707,
	},
};
static const qp_union_t qp_adj_non_idr = {
	.rows = {
		0x00000000, 0x01010101,
		0x03030202, 0x05050404,
		0x06060606, 0x07070707,
		0x08080808, 0x09090909
	},
};
static const qp_union_t qp_adj_cbr_i4i16 = {
	.rows = {
		0x00000000, 0x01010101,
		0x03030202, 0x04040303,
		0x05050404, 0x06060505,
		0x07070606, 0x07070707
	},
};

struct encoder_h264_spec {
	u32 pixelformat;
	u8 num_canvas;
	u8 bits_per_pixel;
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

#define FMT(v4l2, can, bpp, wa, ha, ifmt, ifmte, yuv, r2y, p0bx, p0by, pnbx, pnby) \
	{	V4L2_PIX_FMT_##v4l2, \
		can, bpp, wa, ha, \
		MDFIN_IFMT_##ifmt, \
		MDFIN_IFMT_EXTRA_##ifmte, \
		MDFIN_SUBSAMPLING_##yuv, \
		MDFIN_R2Y_MODE##r2y, \
		MDFIN_BPP_##p0bx, MDFIN_BPP_##p0by, \
		MDFIN_BPP_##pnbx, MDFIN_BPP_##pnby, \
	}

const struct encoder_h264_spec encoder_h264_specs[] = {
	FMT(  NV12M, 2, 8, 32, 16,   NV12P, NONE, 420, 0, 1, 1,    1, HALF),
	FMT(YUV420M, 3, 8, 32, 16, YUV420P, NONE, 420, 0, 1, 1, HALF, HALF),
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

struct encoder_task_ctx {
	enum amlvenc_hcodec_cmd cmd;
	enum amlvenc_hcodec_status hcodec_status;

	struct vb2_v4l2_buffer *src_buf;
	struct vb2_v4l2_buffer *dst_buf;

	u32 dst_buf_offset;
	u32 dst_buf_size;
	dma_addr_t dst_buf_paddr;

	int gop_size;
	int min_qp;
	int max_qp;

	bool reset_encoder;
	bool join_header;
	bool repeat_header;
	bool needs_2pass;
	bool is_2nd_pass;
	bool is_retry;
};

struct encoder_h264_ctx {
	struct meson_codec_job *job;
	struct meson_vcodec_session *session;
	struct v4l2_m2m_ctx *m2m_ctx;
	struct completion encoder_completion;

	struct encoder_task_ctx task;

	u32 enc_start_count;
	u32 enc_done_count;

	struct amlvenc_h264_rc_ctx rc_ctx;
	struct encoder_h264_buffer buffers[MAX_BUFFERS];

	u8 canvases[MAX_CANVAS_TYPE][MAX_NUM_CANVAS];
	bool is_next_idr;

	qp_table_t qtable;

	const struct encoder_h264_spec *encoder_spec;
	struct amlvenc_h264_init_encoder_params init_params;
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
		dev_info(dev, "h264 encoder: No firmware\n");
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
		dev_err(dev, "h264 encoder: Failed to request firmware %s\n", path);
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

	usleep_range(10, 20);

	amlvenc_hcodec_dma_load_firmware(fw_paddr, fw_size);

	ret = wait_hcodec_dma_completed();
	if (ret) {
		dev_err(dev, "h264 encoder: Load firmware timed out (dma hang?)\n");
		goto release_firmware;
	}

	dev_info(dev, "h264 encoder: Firmware %s loaded\n", path);

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

	usleep_range(10, 20);

	amlvenc_hcodec_encode(true);
}

static void amvenc_stop(void)
{
	int ret;

	amlvenc_hcodec_encode(false);

	usleep_range(10, 20);

	ret = wait_hcodec_dma_completed();
	if (ret) {
		pr_warn(DRIVER_NAME ": " "HCODEC dma timed out (dma hang?)\n");
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

int validate_canvas_align(u32 canvas_paddr, u32 canvas_w, u32 canvas_h, u32 width_align, u32 height_align) {
	if ( canvas_w % width_align != 0 ||
		(canvas_w * canvas_h) % height_align != 0) {
		pr_warn(DRIVER_NAME ": " "Canvas width or height is not aligned. Canvas width = %u, Canvas height = %u\n", canvas_w, canvas_h);
		return -EINVAL;
	}

	if (canvas_paddr % width_align != 0) {
		pr_warn(DRIVER_NAME ": " "Canvas physical address is not aligned. Canvas paddr = 0x%x\n", canvas_paddr);
		return -EINVAL;
	}

	return 0;
}

static void clamp_qptbl(qp_union_t *tbl, bool is_idr) {
#if 0
	uint8_t qp_min = is_idr ? 15 : 20;
	uint8_t qp_max = is_idr ? 30 : 35;
#else
	uint8_t qp_min = 8;
	uint8_t qp_max = 43;
#endif

	for (int i = 0; i < QP_ROWS; i++) {
		for (int j = 0; j < QP_COLS; j++) {
			tbl->cells[i][j] = clamp(tbl->cells[i][j], qp_min, qp_max);
		}
	}
}

static void init_fixed_quant(struct encoder_h264_ctx *ctx, u8 quant) {
	qp_table_t *qtable = &ctx->qtable;

	job_trace(ctx->job, "quant=%d", quant);

	memset(qtable, quant, sizeof(qp_table_t));
}

static void init_linear_quant(struct encoder_h264_ctx *ctx, u8 quant) {
	qp_table_t *qtable = &ctx->qtable;

	job_trace(ctx->job, "quant=%d", quant);

	if (quant < 4)
		quant = 4;
	for (int i = 0; i < QP_ROWS; i++) {
		memset(&qtable->i4_qp.rows[i], quant + (i - 4), QP_COLS);
		memset(&qtable->i16_qp.rows[i], quant + (i - 4), QP_COLS);
		memset(&qtable->me_qp.rows[i], quant + (i - 4), QP_COLS);
	}

	//clamp_qptbl(&qtable->i4_qp, ...);
	//clamp_qptbl(&qtable->i16_qp, ...);
	//clamp_qptbl(&qtable->me_qp, ...);
}

static void init_curve_quant(struct encoder_h264_ctx *ctx, u8 quant, const qp_union_t *qp_tbl) {
	qp_table_t *qtable = &ctx->qtable;
	u32 qp_base = quant | quant << 8 | quant << 16 | quant << 24;

	job_trace(ctx->job, "quant=%d", quant);

	for (int i = 0; i < QP_ROWS; i++) {
		qtable->i4_qp.rows[i] = qp_base + qp_tbl->rows[i];
		qtable->i16_qp.rows[i] = qp_base + qp_tbl->rows[i];
		qtable->me_qp.rows[i] = qp_base + qp_tbl->rows[i];
	}
	
	clamp_qptbl(&qtable->i4_qp, qp_tbl == &qp_adj_idr);
	clamp_qptbl(&qtable->i16_qp, qp_tbl == &qp_adj_idr);
	clamp_qptbl(&qtable->me_qp, qp_tbl == &qp_adj_idr);
}

static void amlvenc_h264_rc_cbr_fill_weight_offsets(struct cbr_tbl_block *blk, u16 factor) {
	blk->i4mb_weight_offset  = I4MB_WEIGHT_OFFSET;
	blk->i16mb_weight_offset = I16MB_WEIGHT_OFFSET;
	blk->me_weight_offset    = ME_WEIGHT_OFFSET;
	blk->ie_f_zero_sad_i4    = V3_IE_F_ZERO_SAD_I4 + (factor * 0x480);
	blk->ie_f_zero_sad_i16   = V3_IE_F_ZERO_SAD_I16 + (factor * 0x200);
	blk->me_f_zero_sad       = V3_ME_F_ZERO_SAD + (factor * 0x280);
	blk->end_marker1 = CBR_TBL_BLOCK_END_MARKER1;
	blk->end_marker2 = CBR_TBL_BLOCK_END_MARKER2;
}

static void amlvenc_h264_rc_update_cbr_quant_tables(struct amlvenc_h264_rc_ctx *ctx, bool apply_rc, bool is_idr, const qp_union_t *qp_adj, const qp_union_t *qp_adj_i4i16) {
	const u8 quant = ctx->state.current_qp;
	struct cbr_tbl_block *block_info;
	qp_union_t tbl, tbl_i4i16;
	u32 qp_step;
	u16 i, j;

	// Initialize CBR table values
	if (apply_rc) {
		u8 qp = (quant > START_TABLE_ID) ? (quant - START_TABLE_ID) : 0;
		u32 qp_base = qp | qp << 8 | qp << 16 | qp << 24;
		qp_step = 0x01010101;
		// Fill the table with base values
		for (i = 0; i < QP_ROWS; i++) {
			tbl.rows[i] = qp_base + qp_adj->rows[i];
			tbl_i4i16.rows[i] = qp_base + qp_adj_i4i16->rows[i];
		}
	} else {
		memset(&tbl, quant, sizeof(tbl));
		memset(&tbl_i4i16, quant, sizeof(tbl_i4i16));
		qp_step = 0;
	}

	// Fill the CBR table in memory
	for (i = 0; i < CBR_BLOCK_COUNT; i++) {
		block_info = &ctx->cbr_info->blocks[i];

		// Apply smoothing at each cycle
		clamp_qptbl(&tbl, is_idr);
		clamp_qptbl(&tbl_i4i16, is_idr);

		memcpy(&block_info->qp_table.i4_qp, &tbl_i4i16, sizeof(tbl_i4i16));
		memcpy(&block_info->qp_table.i16_qp, &tbl_i4i16, sizeof(tbl_i4i16));
		memcpy(&block_info->qp_table.me_qp, &tbl, sizeof(tbl));

		amlvenc_h264_rc_cbr_fill_weight_offsets(block_info, (i < 13 ? 0 : i - 13));

		// Apply quantization step for next iteration
		if (is_idr) {
			qp_step = (i == 4 || i == 6 || i == 8 || i >= 10) ? 0x01010101 : 0;
		} else {
			qp_step = (i == 1 || i == 3 || i == 5 || i >= 6) ? 0x01010101 : 0;
		}

		// Update table entries for next iteration
		for (j = 0; j < QP_ROWS; j++) {
			tbl.rows[j] += qp_step;
			tbl_i4i16.rows[j] += qp_step;
		}
	}

	pr_debug("h264_encoder: quant=%d, min_qp=%d, max_qp=%d",
			quant,
			ctx->cbr_info->blocks[0].qp_table.i4_qp.cells[0][0],
			ctx->cbr_info->blocks[CBR_BLOCK_COUNT - 1].qp_table.i4_qp.cells[QP_ROWS - 1][QP_COLS - 1]);
}

static void encoder_config_qp(struct encoder_h264_ctx *ctx) {
	struct encoder_task_ctx *t = &ctx->task;
	struct amlvenc_h264_rc_ctx *rc_ctx = &ctx->rc_ctx;
	bool is_idr = t->cmd == CMD_ENCODE_IDR;
	bool apply_rc = rc_ctx->params.rate_control && (
			!t->needs_2pass || t->is_2nd_pass);
	u8 quant;


	if (!t->is_2nd_pass && (
			t->cmd == CMD_ENCODE_IDR ||
			t->cmd == CMD_ENCODE_NON_IDR)) {
		job_trace(ctx->job, "Updating rate control");
		amlvenc_h264_rc_frame_prepare(rc_ctx, is_idr);
	}

	quant = rc_ctx->state.current_qp;

	if (apply_rc) {
		init_curve_quant(ctx, quant, is_idr ? &qp_adj_idr : &qp_adj_non_idr);
	} else {
		init_fixed_quant(ctx, quant);
	}

	if (t->cmd == CMD_ENCODE_IDR || t->cmd == CMD_ENCODE_NON_IDR) {
		job_trace(ctx->job, "Preparing block sizes");
		amlvenc_h264_rc_update_cbr_mb_sizes(rc_ctx, apply_rc);

		job_trace(ctx->job, "Filling CBR table");
		amlvenc_h264_rc_update_cbr_quant_tables(
				rc_ctx, apply_rc, is_idr,
				is_idr ? &qp_adj_idr : &qp_adj_non_idr,
				&qp_adj_cbr_i4i16);

		// Convert table to RISC format if needed
		if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_GXTVBB) {
			job_trace(ctx->job, "to_risc");
			amlvenc_h264_rc_cbr_to_risc(rc_ctx->cbr_info);
		}

		// TODO configure verbose
#if 0
		print_hex_dump(KERN_DEBUG,
				DRIVER_NAME ": " "CBR Buffer MB Size: ",
				DUMP_PREFIX_OFFSET,
				16, 2,
				rc_ctx->cbr_info->block_mb_size,
				rc_ctx->block_width_n * rc_ctx->block_height_n,
				false);
#endif
#if 0
		print_hex_dump(KERN_DEBUG,
				DRIVER_NAME ": " "CBR Buffer: ",
				DUMP_PREFIX_OFFSET,
				32, 8,
				ctx->buffers[BUF_CBR_INFO].vaddr,
				0xa00, //ctx->buffers[BUF_CBR_INFO].size,
				false);
#endif
	}
}

static int encoder_init_mdfin(struct encoder_h264_ctx *ctx) {
	struct encoder_task_ctx *t = &ctx->task;
	struct amlvenc_h264_mdfin_params *p = &ctx->mdfin_params;
	struct vb2_buffer *vb2 = &t->src_buf->vb2_buf;
	const struct encoder_h264_spec *s = ctx->encoder_spec;
	bool is_idr = t->cmd == CMD_ENCODE_IDR;
	u32 y_width, y_height;
	u32 uv_width, uv_height;
	u8 uv_bppx, uv_bppy;
	u32 canvas_w, canvas_h;
	dma_addr_t canvas_paddr;
	int i, ret;

	// input params
	p->width = V4L2_FMT_WIDTH(ctx->job->src_fmt);
	p->height = V4L2_FMT_HEIGHT(ctx->job->src_fmt);
	p->iformat = s->iformat;
	p->ifmt_extra = s->ifmt_extra;
	p->r2y_mode = s->r2y_mode;
	p->canv_idx0_bppx = s->canvas0_bppx;
	p->canv_idx0_bppy = s->canvas0_bppy;
	p->canv_idx1_bppx = s->canvasN_bppx;
	p->canv_idx1_bppy = s->canvasN_bppy;

	// output params
	p->oformat = MDFIN_SUBSAMPLING_420;

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

	// Y plane
	y_width = ALIGN((p->width * s->bits_per_pixel) >> 3, s->width_align);
	y_height = (p->height * s->bits_per_pixel) >> 3;

	// UV planes
	uv_bppx = s->canvasN_bppx == MDFIN_BPP_HALF ? 4 : s->canvasN_bppx * 8;
	uv_bppy = s->canvasN_bppy == MDFIN_BPP_HALF ? 4 : s->canvasN_bppy * 8;
	uv_width = (y_width * uv_bppx) >> 3;
	uv_height = (y_height * uv_bppy) >> 3;

	for (i = 0; i < s->num_canvas; i++) {
		if (vb2->num_planes == s->num_canvas) {
			canvas_paddr = vb2_dma_contig_plane_dma_addr(vb2, i);
		} else {
			// TODO single plane to multi canvas paddr offset mapping
		}

		canvas_w = i == 0 ? y_width : uv_width;
		canvas_h = i == 0 ? y_height : uv_height;

#ifdef DEBUG
		u32 canvas_size = canvas_w * canvas_h;
		job_trace(ctx->job, "plane %d: bytes=%lu, size=%lu, canvas=%d, canvas_size=%d",
				i,
				vb2_get_plane_payload(&t->src_buf->vb2_buf, i),
				vb2_plane_size(&t->src_buf->vb2_buf, i),
				ctx->canvases[CANVAS_TYPE_INPUT][i],
				canvas_size);
#endif

		ret = validate_canvas_align(
				canvas_paddr,
				canvas_w, canvas_h,
				s->width_align, s->height_align);

		ret = canvas_config(
				ctx->session->core,
				ctx->canvases[CANVAS_TYPE_INPUT][i],
				canvas_paddr,
				canvas_w, canvas_h
			);
		if (ret) {
			// TODO report config canvas failed
			return ret;
		}
	}

	p->input.canvas = COMBINE_CANVAS_ARRAY(
				ctx->canvases[CANVAS_TYPE_INPUT],
				s->num_canvas
			);

	return 0;
}

static int encoder_configure(struct encoder_h264_ctx *ctx) {
	struct encoder_task_ctx *t = &ctx->task;
	bool is_idr = t->cmd == CMD_ENCODE_IDR;
	int ret;

	if (t->reset_encoder) {
		// amlvenc_h264_init_encoder
		ctx->init_params.idr = is_idr;
		amlvenc_h264_init_encoder(&ctx->init_params);

		// amlvenc_h264_init_input_dct_buffer
		amlvenc_h264_init_input_dct_buffer(
				ctx->buffers[BUF_QDCT].paddr,
				ctx->buffers[BUF_QDCT].paddr + ctx->buffers[BUF_QDCT].size - 1);

		// amlvenc_h264_init_output_stream_buffer
		amlvenc_h264_init_output_stream_buffer(
				t->dst_buf_paddr,
				t->dst_buf_paddr + t->dst_buf_size - 1);

		// amlvenc_h264_configure_encoder
		ctx->conf_params.idr = is_idr;
		amlvenc_h264_configure_encoder(&ctx->conf_params);

		// amlvenc_h264_init_firmware_assist_buffer
		amlvenc_h264_init_firmware_assist_buffer(ctx->buffers[BUF_ASSIST].paddr);
	}

	if (t->cmd == CMD_ENCODE_IDR || t->cmd == CMD_ENCODE_NON_IDR) {

		/* amvenc_avc_start_cmd encode request */
		// amlvenc_h264_configure_svc_pic	
// TODO H264_ENC_SVC
#if 0 //#ifdef H264_ENC_SVC
			/* encode non reference frame or not */
			if (request->cmd == CMD_ENCODE_IDR)
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
		ret = encoder_init_mdfin(ctx);
		if (ret) {
			stream_err(ctx->session, t->src_buf->vb2_buf.type, "Failed to initialize MDFIN parameters\n");
			return ret;
		}

		amlvenc_h264_configure_mdfin(&ctx->mdfin_params);

		// amlvenc_h264_configure_ie_me
		amlvenc_h264_configure_ie_me(is_idr ? HENC_MB_Type_I4MB :
				(HENC_SKIP_RUN_AUTO << 16) |
				(HENC_MB_Type_AUTO  <<  4) |
				(HENC_MB_Type_AUTO  <<  0));
	} else {
		amlvenc_h264_configure_ie_me(0);
	}

	// amlvenc_h264_configure_fixed_slice
	// TODO enable multi slice in I frames
	amlvenc_h264_configure_fixed_slice(V4L2_FMT_HEIGHT(ctx->job->src_fmt), 0, 0);

	return 0;
}

static void encoder_loop(struct encoder_h264_ctx *ctx);
static void encoder_abort(struct encoder_h264_ctx *ctx);

static void encoder_done(struct encoder_h264_ctx *ctx) {
	struct encoder_task_ctx *t = &ctx->task;
	u32 full_len, frame_len;
	bool is_last = false;

	frame_len = amlvenc_hcodec_vlc_total_bytes();
	full_len = frame_len + t->dst_buf_offset; 

	// not applicable to SPS and PPS
	if (t->hcodec_status > ENCODER_PICTURE_DONE) {
		v4l2_m2m_buf_copy_metadata(t->src_buf, t->dst_buf, true);
#if 0
		if (t->cmd == CMD_ENCODE_IDR) {
			t->dst_buf->flags |= V4L2_BUF_FLAG_KEYFRAME;
			t->dst_buf->flags &= ~V4L2_BUF_FLAG_PFRAME;
		} else if (t->cmd == CMD_ENCODE_NON_IDR) {
			t->dst_buf->flags &= ~V4L2_BUF_FLAG_KEYFRAME;
			t->dst_buf->flags |= V4L2_BUF_FLAG_PFRAME;
		}
#endif
	}

	job_trace(ctx->job, "id=%d, status=%d, len=%d, ts=%llu",
			ctx->enc_done_count, t->hcodec_status, full_len,
			t->src_buf ? t->src_buf->vb2_buf.timestamp : 0l);

#ifdef DEBUG
	void *data;
	data = vb2_plane_vaddr(&t->dst_buf->vb2_buf, 0);

	job_trace(ctx->job, "data=%*ph",
			full_len < 16 ? full_len : 16, data);

	job_trace(ctx->job,
			"hcodec status: %d, mb info: 0x%x, dct status: 0x%x, vlc status: 0x%x, me status: 0x%x, risc pc:0x%x, debug:0x%x",
			t->hcodec_status,
			amlvenc_hcodec_mb_info(),
			amlvenc_hcodec_qdct_status(),
			amlvenc_hcodec_vlc_status(),
			amlvenc_hcodec_me_status(),
			amlvenc_hcodec_mpc_risc(),
			amlvenc_hcodec_debug());
#endif

	// update stats
	ctx->enc_done_count++;

	if (t->hcodec_status > ENCODER_PICTURE_DONE) {
		amlvenc_h264_rc_update_stats(&ctx->rc_ctx, &ctx->conf_params);
#if 0
		print_hex_dump(KERN_DEBUG,
				DRIVER_NAME ": " "MB SAD Size: ",
				DUMP_PREFIX_OFFSET,
				16, 4,
				ctx->rc_ctx.block_sad_size,
				ctx->rc_ctx.block_width_n * ctx->rc_ctx.block_height_n,
				false);
#endif
#if 0
		amlvenc_h264_rc_hexdump_mb_info(&ctx->rc_ctx);
#endif

		if (t->needs_2pass && !t->is_2nd_pass) {
			t->is_2nd_pass = true;
			encoder_loop(ctx);
			return;
		}

		if (amlvenc_h264_rc_frame_done(&ctx->rc_ctx, frame_len << 3)) {
			if (t->is_retry) {
				job_err(ctx->job, "frame size too large for bitrate, aborting (len=%d, target=%d)\n", frame_len, ctx->rc_ctx.state.avg_bits_per_frame >> 3);
				encoder_abort(ctx);
				return;
			}
			// Reencode frames that are to large
			// using the increased qp
			job_info(ctx->job, "frame size too large, retrying with higher qp (len=%d, target=%d)\n", frame_len, ctx->rc_ctx.state.avg_bits_per_frame >> 3);
			t->is_retry = true;
			t->is_2nd_pass = false;
			encoder_loop(ctx);
			return;
		}
	}

	if (t->hcodec_status == ENCODER_SEQUENCE_DONE) {
		if (t->join_header) {
			t->dst_buf_offset += frame_len;
			t->dst_buf_paddr += frame_len;
			t->dst_buf_size -= frame_len;
		}
		t->cmd = CMD_ENCODE_PICTURE;
		encoder_loop(ctx);
		return;

	} else if (t->hcodec_status == ENCODER_PICTURE_DONE && t->join_header) {
		t->dst_buf_offset += frame_len;
		t->dst_buf_paddr += frame_len;
		t->dst_buf_size -= frame_len;
		t->cmd = CMD_ENCODE_IDR;
		encoder_loop(ctx);
		return;

	} else {
		// no src_buf when encoding SPS or PPS
		if (t->hcodec_status > ENCODER_PICTURE_DONE) {
			v4l2_m2m_buf_done(t->src_buf, VB2_BUF_STATE_DONE);
			t->src_buf = NULL;
		}

		vb2_set_plane_payload(&t->dst_buf->vb2_buf, 0, full_len);

		is_last = ctx->m2m_ctx->is_draining &&
			v4l2_m2m_num_src_bufs_ready(ctx->m2m_ctx) == 0;
		if (is_last) {
			v4l2_m2m_last_buffer_done(ctx->m2m_ctx, t->dst_buf);
		} else {
			v4l2_m2m_buf_done(t->dst_buf, VB2_BUF_STATE_DONE);
		}
		t->dst_buf = NULL;
		t->dst_buf_size = 0;
		t->dst_buf_paddr = 0;
		t->dst_buf_offset = 0;

		// Increment frame_number
		if (t->hcodec_status > ENCODER_PICTURE_DONE) {
			// TODO H264_ENC_SVC
#if 0 //#ifdef H264_ENC_SVC
			/* only update when there is reference frame */
			if (wq->pic.enable_svc == 0 || wq->pic.non_ref_cnt == 0) {
				wq->pic.frame_number++;
				enc_pr(LOG_INFO, "Increase frame_num to %d\n",
						wq->pic.frame_number);
			}
#endif
			ctx->init_params.frame_number++;
		}

		v4l2_m2m_job_finish(ctx->m2m_ctx->m2m_dev, ctx->m2m_ctx);
		return;
	}
}

static void encoder_abort(struct encoder_h264_ctx *ctx) {
	struct encoder_task_ctx *t = &ctx->task;

	job_err(ctx->job,
			"encoder_abort: hcodec status:%d, mb info: 0x%x, dct status: 0x%x, vlc status: 0x%x, me status: 0x%x, risc pc:0x%x, debug:0x%x\n",
			amlvenc_hcodec_status(false),
			amlvenc_hcodec_mb_info(),
			amlvenc_hcodec_qdct_status(),
			amlvenc_hcodec_vlc_status(),
			amlvenc_hcodec_me_status(),
			amlvenc_hcodec_mpc_risc(),
			amlvenc_hcodec_debug());

	amvenc_stop();

	v4l2_m2m_mark_stopped(ctx->m2m_ctx);

	if(t->src_buf) {
		v4l2_m2m_buf_done(t->src_buf, VB2_BUF_STATE_ERROR);
		t->src_buf = NULL;
	}

	if(t->dst_buf) {
		v4l2_m2m_buf_done(t->dst_buf, VB2_BUF_STATE_ERROR);
		t->dst_buf = NULL;
		t->dst_buf_size = 0;
		t->dst_buf_paddr = 0;
		t->dst_buf_offset = 0;
	}

	meson_vcodec_event_eos(ctx->session);

	if (t->src_buf || t->dst_buf) {
		v4l2_m2m_job_finish(ctx->m2m_ctx->m2m_dev, ctx->m2m_ctx);
	}
}

static void encoder_start(struct encoder_h264_ctx *ctx) {
	struct encoder_task_ctx *t = &ctx->task;
	bool is_idr;

	// Ensure there are buffers to work on
	t->src_buf = v4l2_m2m_next_src_buf(ctx->m2m_ctx);
	t->dst_buf = v4l2_m2m_next_dst_buf(ctx->m2m_ctx);
	if (!t->src_buf || !t->dst_buf) {
		job_trace(ctx->job, "no buffers available");
		v4l2_m2m_job_finish(ctx->m2m_ctx->m2m_dev, ctx->m2m_ctx);
		t->src_buf = NULL;
		t->dst_buf = NULL;
		return;
	}

	/*
	 * When to encode the SPS/PPS header:
	 *      - Always before the first frame
	 *      _ Before any IDR if repeat_header
	 *
	 * How to encode the SPS / PPS header:
	 * 1. CMD_ENCODE_SEQUENCE
	 *      - It never generates a buffer
	 *      - encoder_loop to CMD_ENCODE_PICTURE
	 * 2. CMD_ENCODE_PICTURE
	 *      a. if join_header
	 *      - IDR is appended to the header
	 *      - encoder_loop to CMD_ENCODE_IDR
	 *      b. if !join_header
	 *      - Generates a buffer for the header
	 *      - IDR by the next encoder_start
	 */

	// set encoder cmd
	// first frame
	if (ctx->enc_start_count == 0) {
		is_idr = true;
		t->cmd = CMD_ENCODE_SEQUENCE;

	// subsequent frames
	} else {
		is_idr =
			(ctx->init_params.frame_number % t->gop_size == 0) ||
			(t->src_buf->flags & V4L2_BUF_FLAG_KEYFRAME);

		if (is_idr && t->repeat_header && ctx->enc_start_count != 2) {
			t->cmd = CMD_ENCODE_SEQUENCE;
		} else if (is_idr) {
			t->cmd = CMD_ENCODE_IDR;
		} else {
			t->cmd = CMD_ENCODE_NON_IDR;
		}
	}

	// increment idr_pic_id
	if ( t->cmd == CMD_ENCODE_SEQUENCE ||
		(t->cmd == CMD_ENCODE_IDR && !t->repeat_header && ctx->enc_start_count != 2)) {
		ctx->init_params.idr_pic_id = (ctx->init_params.idr_pic_id + 1) % 65536;
		ctx->init_params.frame_number = 0;
	} else

	job_trace(ctx->job, "idr=%d, cmd=%d", is_idr, t->cmd);

	// set buffers
	if (t->cmd == CMD_ENCODE_SEQUENCE && !t->join_header) {
		t->src_buf = NULL;
	} else {
		v4l2_m2m_src_buf_remove_by_buf(ctx->m2m_ctx, t->src_buf);
	}
	v4l2_m2m_dst_buf_remove_by_buf(ctx->m2m_ctx, t->dst_buf);
	t->dst_buf_paddr = vb2_dma_contig_plane_dma_addr(&t->dst_buf->vb2_buf, 0);
	t->dst_buf_size = vb2_plane_size(&t->dst_buf->vb2_buf, 0);
	t->dst_buf_offset = 0;

	// start encoder loop
	t->needs_2pass = ctx->rc_ctx.params.rate_control && is_idr;
	t->is_2nd_pass = false;
	t->is_retry = false;
	encoder_loop(ctx);
}

static void encoder_loop(struct encoder_h264_ctx *ctx) {
	struct encoder_task_ctx *t = &ctx->task;
	int ret;

	t->reset_encoder = t->cmd != CMD_ENCODE_PICTURE;

	if (t->reset_encoder) {
		// amvenc_stop
		amvenc_stop();

		// amvenc_reset
		amvenc_reset();
	}

	encoder_config_qp(ctx);

	// Configure encoding parameters
	ret = encoder_configure(ctx);
	if (ret) {
		if (t->src_buf)
			v4l2_m2m_buf_done(t->src_buf, VB2_BUF_STATE_ERROR);
		if (t->dst_buf)
			v4l2_m2m_buf_done(t->dst_buf, VB2_BUF_STATE_ERROR);
		t->src_buf = NULL;
		t->dst_buf = NULL;
		t->dst_buf_size = 0;
		t->dst_buf_paddr = 0;
		t->dst_buf_offset = 0;
		v4l2_m2m_job_finish(ctx->m2m_ctx->m2m_dev, ctx->m2m_ctx);
		return;
	}

	job_trace(ctx->job, "id=%d, cmd=%d, idr=%d, 2nd_pass=%d, idr_pic_id=%d, frame_number=%d, ts=%llu",
			ctx->enc_start_count, t->cmd,
			t->cmd == CMD_ENCODE_IDR, t->is_2nd_pass,
			ctx->init_params.idr_pic_id,
			ctx->init_params.frame_number,
			t->src_buf ? t->src_buf->vb2_buf.timestamp : 0l);

	// update stats
	ctx->enc_start_count++;

	// amlvenc_hcodec_cmd
	reinit_completion(&ctx->encoder_completion);
	amlvenc_hcodec_cmd(t->cmd);

	if (t->reset_encoder) {
		// Start encoding
		amvenc_start();
	}

	unsigned long timeout;
	timeout = wait_for_completion_timeout(&ctx->encoder_completion, msecs_to_jiffies(ENCODER_TIMEOUT_MS));

	if (timeout) {
		switch (t->hcodec_status) {
			case ENCODER_SEQUENCE_DONE:
			case ENCODER_PICTURE_DONE:
			case ENCODER_FRAME_DONE:
				encoder_done(ctx);
				break;
			default:
				job_warn(ctx->job, "Invalid hcodec_status %d\n", t->hcodec_status);
				encoder_abort(ctx);
				break;
		}
	} else {
		job_err(ctx->job, "encoder timeout\n");
		encoder_abort(ctx);
	}
}

static irqreturn_t hcodec_isr(int irq, void *data)
{
	const struct meson_codec_job *job = data;
	struct encoder_h264_ctx *ctx = job->priv;
	struct encoder_task_ctx *t = &ctx->task;

	// read encoder status and clear interrupt
	enum amlvenc_hcodec_status status = amlvenc_hcodec_status(true);

	switch (status) {
		case ENCODER_SEQUENCE_DONE:
		case ENCODER_PICTURE_DONE:
		case ENCODER_FRAME_DONE:
			t->hcodec_status = status;
			complete(&ctx->encoder_completion);
			return IRQ_HANDLED;
		default:
			return IRQ_NONE;
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

static int encoder_h264_prepare(struct meson_codec_job *job) {
	struct meson_vcodec_session *session = job->session;
	struct encoder_h264_ctx	*ctx;
	int ret;

	ctx = kcalloc(1, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
	ctx->job = job;
	ctx->session = session;
	ctx->m2m_ctx = session->m2m_ctx;
	init_completion(&ctx->encoder_completion);
	job->priv = ctx;

	ctx->init_params.init_qppicture = INITIAL_QP;
	ctx->init_params.idr_pic_id = 1;
	ctx->init_params.frame_number = 0;
	ctx->me_params = amlvenc_h264_me_defaults;
	amlvenc_h264_init_me(&ctx->me_params);

	u32 width = V4L2_FMT_WIDTH(job->src_fmt);
	u32 height = V4L2_FMT_HEIGHT(job->src_fmt);
	u32 canvas_width = ALIGN(width, 32);
	u32 canvas_height = ALIGN(height, 16);
	u32 mb_width = (width + 15) >> 4;
	u32 mb_height = (height + 15) >> 4;

	// dct: 0x800000 1920x1088x4
	ctx->buffers[BUF_QDCT].size = canvas_width * canvas_height * 4;
	// assit: 0xc0000
	ctx->buffers[BUF_ASSIST].size = 0xc0000;
	// cbr_info: 0x2000
	ctx->buffers[BUF_CBR_INFO].size = sizeof(struct cbr_info_buffer);
	// dump_info: 0xa0000 (1920x1088/256)x80
	ctx->buffers[BUF_DUMP_INFO].size = mb_width * mb_height * MB_INFO_SIZE;
	// dec0_y: 0x300000 canvas_width, canvas_height
	ctx->buffers[BUF_DBLK_Y].size = canvas_width * canvas_height;	
	// dec0_y: 0x300000 canvas_width, canvas_height / 2
	ctx->buffers[BUF_DBLK_UV].size = canvas_width * canvas_height / 2; 
	// dec1_y: 0x300000 canvas_width, canvas_height
	ctx->buffers[BUF_REF_Y].size = canvas_width * canvas_height; 
	// dec1_y: 0x300000 canvas_width, canvas_height / 2
	ctx->buffers[BUF_REF_UV].size = canvas_width * canvas_height / 2; 

	ctx->encoder_spec = find_encoder_spec(V4L2_FMT_PIXFMT(job->src_fmt));
	if (!ctx->encoder_spec) {
		job_err(job, "Failed to find spec of %.4s\n", V4L2_FMT_FOURCC(job->src_fmt));
		ret = -EINVAL;
		goto free_ctx;
	}

	ret = buffers_alloc(session->core->dev, ctx->buffers, MAX_BUFFERS);
	if (ret) {
		job_err(job, "Failed to allocate buffer\n");
		goto free_ctx;
	}

	struct amlvenc_h264_rc_params rc_params;
	rc_params.target_bitrate = meson_vcodec_g_ctrl(ctx->session, V4L2_CID(BITRATE));
	rc_params.frame_rate_num = ctx->session->timeperframe.denominator;
	rc_params.frame_rate_den = ctx->session->timeperframe.numerator;
	//rc_params.vbv_buffer_size;
	rc_params.mb_width = mb_width;
	rc_params.mb_height = mb_height;
	rc_params.initial_qp = INITIAL_QP;
	rc_params.rate_control = meson_vcodec_g_ctrl(ctx->session, V4L2_CID(FRAME_RC_ENABLE));
	ret = amlvenc_h264_rc_init(
			&ctx->rc_ctx,
			&rc_params,
			ctx->buffers[BUF_CBR_INFO].vaddr,
			ctx->buffers[BUF_DUMP_INFO].vaddr);
	if (ret) {
		job_err(job, "Failed to init rate control context\n");
		goto free_buffers;
	}

	/* alloc canvases */
	ret = canvas_alloc(session->core, &ctx->canvases[0][0], MAX_CANVAS_TYPE * MAX_NUM_CANVAS);
	if (ret) {
		job_err(job, "Failed to allocate canvases\n");
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
		job_err(job, "Failed to request HCODEC irq\n");
		goto free_canvas;
	}

	ctx->task.gop_size = meson_vcodec_g_ctrl(ctx->session, V4L2_CID(GOP_SIZE));
	ctx->task.repeat_header = meson_vcodec_g_ctrl(ctx->session, V4L2_CID(REPEAT_SEQ_HEADER));
	ctx->task.join_header = meson_vcodec_g_ctrl(ctx->session, V4L2_CID(HEADER_MODE));
	ctx->task.min_qp = meson_vcodec_g_ctrl(ctx->session, V4L2_CID(H264_MIN_QP));
	ctx->task.max_qp = meson_vcodec_g_ctrl(ctx->session, V4L2_CID(H264_MAX_QP));

	job_info(ctx->job, "Encoding parameters: min_qp=%d, max_qp=%d, bitrate=%d, rc=%d, frame_rate=%d/%d, bytes/frame=%d, gop=%d, join_header=%d, repeat_header=%d\n",
			ctx->task.min_qp,
			ctx->task.max_qp,
			ctx->rc_ctx.params.target_bitrate,
			ctx->rc_ctx.params.rate_control,
			ctx->rc_ctx.params.frame_rate_num,
			ctx->rc_ctx.params.frame_rate_den,
			ctx->rc_ctx.state.avg_bits_per_frame >> 3,
			ctx->task.gop_size,
			ctx->task.join_header,
			ctx->task.repeat_header);

	ctx->conf_params.qp_mode = 1;
	ctx->conf_params.encoder_width = V4L2_FMT_WIDTH(ctx->job->src_fmt);
	ctx->conf_params.encoder_height = V4L2_FMT_HEIGHT(ctx->job->src_fmt);
	ctx->conf_params.i4_weight = I4MB_WEIGHT_OFFSET;
	ctx->conf_params.i16_weight = I16MB_WEIGHT_OFFSET;
	ctx->conf_params.me_weight = ME_WEIGHT_OFFSET;
	ctx->conf_params.cbr_ddr_start_addr = ctx->buffers[BUF_CBR_INFO].paddr;
	ctx->conf_params.cbr_start_tbl_id = START_TABLE_ID;
	ctx->conf_params.cbr_short_shift = CBR_SHORT_SHIFT;
	ctx->conf_params.cbr_long_mb_num = CBR_LONG_MB_NUM;
	ctx->conf_params.cbr_long_th = CBR_LONG_THRESH;
	ctx->conf_params.cbr_block_w = ctx->rc_ctx.block_width;
	ctx->conf_params.cbr_block_h = ctx->rc_ctx.block_height;
	ctx->conf_params.dump_ddr_start_addr = ctx->buffers[BUF_DUMP_INFO].paddr;
	ctx->conf_params.qtable = &ctx->qtable;
	ctx->conf_params.me = &ctx->me_params;

	return 0;

canvas_config_failed:
	job_err(job, "Failed to configure buffer canvases\n");
	goto free_canvas;

//free_irq:
	free_irq(session->core->irqs[IRQ_HCODEC], (void *)job);
free_canvas:
	canvas_free(session->core, &ctx->canvases[0][0], MAX_CANVAS_TYPE * MAX_NUM_CANVAS);
free_buffers:
	amlvenc_h264_rc_free(&ctx->rc_ctx);
	buffers_free(session->core->dev, ctx->buffers, MAX_BUFFERS);
free_ctx:
	kfree(ctx);
	job->priv = NULL;
	return ret;
}

static int encoder_h264_init(struct meson_codec_dev *codec) {
	struct meson_vcodec_core *core = codec->core;
	int ret;

	/* hcodec_clk */
	ret = meson_vcodec_clk_prepare(core, CLK_HCODEC, 667 * MHz);
	if (ret) {
		dev_err(core->dev, "Failed to enable HCODEC clock\n");
		return ret;
	}

	/* hcodec_rst */
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SC2) {
		if (!core->resets[RESET_HCODEC]) {
			dev_err(core->dev, "Failed to get HCODEC reset\n");
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
		dev_err(core->dev, "Failed to power on HCODEC\n");
		goto disable_clk;
	}

	usleep_range(10, 20);

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
		dev_err(core->dev, "Failed to load firmware\n");
		goto disable_hcodec;
	}

	/* request irq */
	if (!core->irqs[IRQ_HCODEC]) {
		dev_err(core->dev, "Failed to get HCODEC irq\n");
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
	struct encoder_h264_ctx *ctx = job->priv;

	v4l2_m2m_update_start_streaming_state(ctx->m2m_ctx, vq);

	return 0;
}

static int encoder_h264_queue(struct meson_codec_job *job, struct vb2_v4l2_buffer *vb) {
	struct encoder_h264_ctx *ctx = job->priv;

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

static void encoder_h264_run(struct meson_codec_job *job) {
	struct encoder_h264_ctx *ctx = job->priv;

	encoder_start(ctx);
}

static void encoder_h264_abort(struct meson_codec_job *job) {
	struct encoder_h264_ctx *ctx = job->priv;

	// v4l2-m2m may call abort even if stopped
	// and job finished earlier with error state
	// let finish the job once again to prevent
	// v4l2-m2m hanging forever
	if (v4l2_m2m_has_stopped(ctx->m2m_ctx)) {
		v4l2_m2m_job_finish(ctx->m2m_ctx->m2m_dev, ctx->m2m_ctx);
	}
}

static int encoder_h264_stop(struct meson_codec_job *job, struct vb2_queue *vq) {
	struct encoder_h264_ctx *ctx = job->priv;

	v4l2_m2m_update_stop_streaming_state(ctx->m2m_ctx, vq);

	if (V4L2_TYPE_IS_OUTPUT(vq->type) &&
			v4l2_m2m_has_stopped(ctx->m2m_ctx))
		meson_vcodec_event_eos(ctx->session);

	return 0;
}

static void encoder_h264_unprepare(struct meson_codec_job *job) {
	struct encoder_h264_ctx *ctx = job->priv;

	free_irq(job->session->core->irqs[IRQ_HCODEC], (void *)job);

	canvas_free(job->session->core, &ctx->canvases[0][0], MAX_CANVAS_TYPE * MAX_NUM_CANVAS);
	amlvenc_h264_rc_free(&ctx->rc_ctx);
	buffers_free(job->session->core->dev, ctx->buffers, MAX_BUFFERS);

	kfree(ctx);
	job->priv = NULL;
}

static void encoder_h264_release(struct meson_codec_dev *codec) {

	amlvenc_dos_hcodec_memory(false);
	amlvenc_dos_hcodec_gateclk(false);
	meson_vcodec_pwrc_off(codec->core, PWRC_HCODEC);
	meson_vcodec_clk_unprepare(codec->core, CLK_HCODEC);
}

static int h264_encoder_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct meson_codec_job *job = (struct meson_codec_job *)ctrl->priv;
	struct encoder_h264_ctx *ctx = job->priv;
	job_dbg(job, "FORCE_KEY_FRAME");

	/* ctx may have not been created yet since first frame has not been queued yet */
	if (ctx) {
		ctx->is_next_idr = true;
	}
	return 0;
}

static const struct v4l2_ctrl_ops h264_encoder_ctrl_ops = {
	.s_ctrl = h264_encoder_s_ctrl,
};

static const struct v4l2_ctrl_config h264_encoder_ctrls[] = {
//	V4L2_CTRL(B_FRAMES, 0, 16, 1, 2 ),
	V4L2_CTRL(GOP_SIZE, 1, 300, 1, 30 ),
	V4L2_CTRL(BITRATE, 100000, 100000000, 100000, 2000000 ),
	V4L2_CTRL(FRAME_RC_ENABLE, 0, 1, 1, 1 ),
	V4L2_CTRL(HEADER_MODE, 0, 1, 0, 0 ),
	V4L2_CTRL(REPEAT_SEQ_HEADER, 0, 1, 1, 0 ),
	V4L2_CTRL_OPS(FORCE_KEY_FRAME, &h264_encoder_ctrl_ops, 0, 0, 0, 0 ),
	V4L2_CTRL(H264_MIN_QP, 0, 51, 1, 10 ),
	V4L2_CTRL(H264_MAX_QP, 0, 51, 1, 51 ),
//	V4L2_CTRL(H264_PROFILE, 0, 4, 1, 0 ),
};

static const struct meson_codec_ops h264_encoder_ops = {
	.init = &encoder_h264_init,
	.prepare = &encoder_h264_prepare,
	.start = &encoder_h264_start,
	.queue = &encoder_h264_queue,
	.run = &encoder_h264_run,
	.abort = &encoder_h264_abort,
	.stop = &encoder_h264_stop,
	.unprepare = &encoder_h264_unprepare,
	.release = &encoder_h264_release,
};

const struct meson_codec_spec h264_encoder = {
	.type = H264_ENCODER,
	.name = "h264_encoder",
	.ops = &h264_encoder_ops,
	.ctrls = h264_encoder_ctrls,
	.num_ctrls = ARRAY_SIZE(h264_encoder_ctrls),
};

