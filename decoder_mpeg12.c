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

#include "amlvdec_vdec.h"
#include "amlvdec_mpeg_dpb.h"
#include "decoder_common.h"


#define MREG_CC_ADDR    AV_SCRATCH_0
/* AV_SCRATCH_1
	bit [0-9]: temporal_reference(poc)
*/
#define MREG_REF0        AV_SCRATCH_2
#define MREG_REF1        AV_SCRATCH_3
/* protocol registers */
#define MREG_SEQ_INFO       AV_SCRATCH_4
	#define SEQINFO_EXT_AVAILABLE   0x80000000
	#define SEQINFO_PROG            0x00010000

#define MREG_PIC_INFO       AV_SCRATCH_5
	#define PICINFO_ERROR       0x80000000
	#define PICINFO_TYPE_MASK   0x00030000
	#define PICINFO_TYPE_I      0x00000000
	#define PICINFO_TYPE_P      0x00010000
	#define PICINFO_TYPE_B      0x00020000
	#define PICINFO_PROG        0x8000
	#define PICINFO_RPT_FIRST   0x4000
	#define PICINFO_TOP_FIRST   0x2000
	#define PICINFO_FRAME       0x1000

#define MREG_PIC_WIDTH      AV_SCRATCH_6
#define MREG_PIC_HEIGHT     AV_SCRATCH_7
#define MREG_INPUT          AV_SCRATCH_8  /*input_type*/
#define MREG_BUFFEROUT      AV_SCRATCH_9  /*FROM_AMRISC_REG*/
	#define MPEG12_PIC_DONE     1
	#define MPEG12_DATA_EMPTY   2
	#define MPEG12_SEQ_END      3
	#define MPEG12_DATA_REQUEST 4

#define MREG_CMD            AV_SCRATCH_A
#define MREG_CO_MV_START    AV_SCRATCH_B
#define MREG_ERROR_COUNT    AV_SCRATCH_C
#define MREG_FRAME_OFFSET   AV_SCRATCH_D
#define MREG_WAIT_BUFFER    AV_SCRATCH_E
#define MREG_FATAL_ERROR    AV_SCRATCH_F
/* AV_SCRATCH_G */
	#define MPEG12_V4L2_INFO_NOTIFY 1

/* AV_SCRATCH_H */
/* AV_SCRATCH_I */
/* AV_SCRATCH_J */
	#define MPEG12_USERDATA_DONE 0x8000

#define DECODE_STOP_POS         AV_SCRATCH_K
/* AV_SCRATCH_L */
/* AV_SCRATCH_M */
/* AV_SCRATCH_N */


#define AUX_BUF_ALIGN(adr) ((adr + 0xf) & (~0xf))

#define GET_SLICE_TYPE(type)  ("IPB##"[((type&PICINFO_TYPE_MASK)>>16)&0x3])

#define TOP_FIELD            0x1000
#define BOTTOM_FIELD         0x2000
#define FRAME_PICTURE        0x3000
#define FRAME_PICTURE_MASK   0x3000

#define CCBUF_SIZE      	(5*SZ_1K)

#define VF_POOL_SIZE        64
#define DECODE_BUFFER_NUM_MAX 16
#define DECODE_BUFFER_NUM_DEF 8
#define MAX_BMMU_BUFFER_NUM (DECODE_BUFFER_NUM_MAX + 1)

#define PUT_INTERVAL        (HZ/100)
#define WORKSPACE_SIZE		(4*SZ_64K) /*swap&ccbuf&matirx&MV*/
#define CTX_LMEM_SWAP_OFFSET    0
#define CTX_CCBUF_OFFSET        0x800
#define CTX_QUANT_MATRIX_OFFSET (CTX_CCBUF_OFFSET + 5*1024)
#define CTX_CO_MV_OFFSET        (CTX_QUANT_MATRIX_OFFSET + 1*1024)
#define CTX_DECBUF_OFFSET       (CTX_CO_MV_OFFSET + 0x11000)

#define DEFAULT_MEM_SIZE	(32*SZ_1M)
#define INVALID_IDX 		(-1)  /* Invalid buffer index.*/

#define MPEG2_ERROR_FRAME_DISPLAY 0
#define MPEG2_ERROR_FRAME_DROP 1

#define MAX_SIZE_2K (1920 * 1088)

#define INTERLACE_SEQ_ALWAYS

#define DEC_RESULT_NONE     0
#define DEC_RESULT_DONE     1
#define DEC_RESULT_AGAIN    2
#define DEC_RESULT_ERROR    3
#define DEC_RESULT_FORCE_EXIT 4
#define DEC_RESULT_EOS 5
#define DEC_RESULT_GET_DATA         6
#define DEC_RESULT_GET_DATA_RETRY   7
#define DEC_RESULT_UNFINISH         11

#define DEC_DECODE_TIMEOUT         0x21
#define DECODE_ID(hw) (hw_to_vdec(hw)->id)

#define USERDATA_FIFO_NUM    256
#define MAX_FREE_USERDATA_NODES		5

#define MAX_UD_RECORDS	5

static const u32 frame_rate_tab[16] = {
	96000 / 30, 96000000 / 23976, 96000 / 24, 96000 / 25,
	9600000 / 2997, 96000 / 30, 96000 / 50, 9600000 / 5994,
	96000 / 60,
	/* > 8 reserved, use 24 */
	96000 / 24, 96000 / 24, 96000 / 24, 96000 / 24,
	96000 / 24, 96000 / 24, 96000 / 24
};

typedef enum {
	PIC_TYPE_I = 0x0,
	PIC_TYPE_P = 0x1,
	PIC_TYPE_B = 0x2
} picinfo_type;

typedef struct {
	u32 reserved1       : 12; // Reserved bits
	bool is_frame       : 1;  // PICINFO_FRAME (Bit 12)
	bool is_top_first   : 1;  // PICINFO_TOP_FIRST (Bit 13)
	bool is_rpt_first   : 1;  // PICINFO_RPT_FIRST (Bit 14)
	bool is_progressive : 1;  // PICINFO_PROG (Bit 15)
	picinfo_type type   : 2;  // PICINFO_TYPE (Bits 16-17)
	u32 reserved2       : 14; // Reserved bits
	bool is_error       : 1;  // PICINFO_ERROR (Bit 31)
} picinfo_t;

enum decoder_mpeg_buffers : u8 {
	BUF_WORKSPACE,
	BUF_USERDATA,
	MAX_INTERNAL_BUFFERS,
};

enum decoder_mpeg_canvas : u8 {
	CANVAS_CURRENT_FRAME,
	CANVAS_FORWARD_REF,
	CANVAS_BACKWARD_REF,
	MAX_DECODE_CANVAS,
};

struct decoder_task_ctx {
	struct vb2_v4l2_buffer *src_buf;
	struct vb2_v4l2_buffer *dst_buf;
	dpb_picture_t *dpb_pic;
};

struct decoder_mpeg_ctx {
	struct meson_codec_job *job;
	struct meson_vcodec_session *session;
	struct v4l2_m2m_ctx *m2m_ctx;
	struct completion decoder_completion;

	struct decoder_task_ctx task;
	struct meson_vcodec_buffer buffers[MAX_INTERNAL_BUFFERS];

	u8 canvases[MAX_DECODE_CANVAS][MAX_NUM_CANVAS];

	dpb_buffer_t dpb;

	u64 ts_step;
	u64 last_ts;

	u32 pic_width;
	u32 pic_height;
	u32 mb_width;
	u32 mb_height;
	u32 mb_total;

	u32 status;

	u32 decode_pic_count;
	bool init_flag;
};

/* decoder forward declarations */
static void decoder_start(struct decoder_mpeg_ctx *ctx);
static void decoder_loop(struct decoder_mpeg_ctx *ctx);
static void decoder_done(struct decoder_mpeg_ctx *ctx);
static void decoder_abort(struct decoder_mpeg_ctx *ctx);


static inline void dump_registers(struct decoder_mpeg_ctx *ctx, const char *prefix) {
	u32 val = READ_VREG(MREG_PIC_INFO);
	picinfo_t *picinfo = (picinfo_t *) &val;

	job_dbg(ctx->job, "%s: "
			"MREG_CMD 0x%x, "
			"MREG_BUFFEROUT 0x%x, "
			"MREG_ERROR_COUNT 0x%x, "
			"MREG_FATAL_ERROR 0x%x, "
			"VLD_DECODE_CONTROL 0x%x, "
			"VFIFO lvl=0x%x cur=0x%x rp=0x%x wp=0x%x, "
			"BCNT 0x%x, "
			"MPEG1_2_REG 0x%x, "
			"PIC_HEAD_INFO 0x%x, "
			"MREG_FRAME_OFFSET 0x%x, "
			"MREG_SEQ_INFO 0x%x (dur=%d), "
			"MREG_PIC_INFO 0x%x (type=%d, error=%d, prog=%d, frame=%d, top=%d, rpt=%d), "
			"MREG_PIC_WIDTH %u, "
			"MREG_PIC_HEIGHT %u, "
			"POC %u (%u), "
			"J 0x%x, "
			"L 0x%x\n",
			prefix,
			READ_VREG(MREG_CMD),
			READ_VREG(MREG_BUFFEROUT),
			READ_VREG(MREG_ERROR_COUNT),
			READ_VREG(MREG_FATAL_ERROR),
			READ_VREG(VLD_DECODE_CONTROL),
			READ_VREG(VLD_MEM_VIFIFO_LEVEL),
			READ_VREG(VLD_MEM_VIFIFO_CURR_PTR),
			READ_VREG(VLD_MEM_VIFIFO_RP),
			READ_VREG(VLD_MEM_VIFIFO_WP),
			READ_VREG(VIFF_BIT_CNT) >> 3,
			READ_VREG(MPEG1_2_REG),
			READ_VREG(PIC_HEAD_INFO),
			READ_VREG(MREG_FRAME_OFFSET),
			READ_VREG(MREG_SEQ_INFO),
			frame_rate_tab[(READ_VREG(MREG_SEQ_INFO) >> 4) & 0xf],
			READ_VREG(MREG_PIC_INFO),
			picinfo->type,
			picinfo->is_error,
			picinfo->is_progressive,
			picinfo->is_frame,
			picinfo->is_top_first,
			picinfo->is_rpt_first,
			READ_VREG(MREG_PIC_WIDTH),
			READ_VREG(MREG_PIC_HEIGHT),
			READ_VREG(AV_SCRATCH_1),
			(u32)(READ_VREG(AV_SCRATCH_1) & GENMASK(9,0)),
			READ_VREG(AV_SCRATCH_J),
			READ_VREG(AV_SCRATCH_L));

#if 0
		int size = ctx->buffers[BUF_WORKSPACE].size;
		char *buf = ctx->buffers[BUF_WORKSPACE].vaddr;
		for (int i = size - 1; i>= 0; i--) {
			if (buf[i] != 0) {
				job_trace(ctx->job, "last non-zero offset: 0x%x", i);
				break;
			}
		}
#endif
}

static int config_decode_canvas(struct decoder_mpeg_ctx *ctx, enum decoder_mpeg_canvas index, void *buffer)
{
	int ret;

	struct vb2_v4l2_buffer *dst_buf = buffer;

	if (buffer == NULL) {
		job_err(ctx->job, "Failed to configure: no buffer specified");
		return -ENOENT;
	}

	job_trace(ctx->job, "canvas=%d, vb2=%d", index, dst_buf->vb2_buf.index);

	ret = meson_vcodec_config_vb2_canvas(
		   ctx->job->codec,
		   ctx->job->dst_fmt,
		   &dst_buf->vb2_buf,
		   ctx->canvases[index]);
	if (ret < 0) {
		job_err(ctx->job, "Failed to configure vb2 canvas");
		return ret;
	}

	return ret;
}

static int amlvdec_mpeg_init(struct decoder_mpeg_ctx *ctx) {

	/* begin vmpeg12_hw_ctx_restore */
	WRITE_VREG(MREG_CC_ADDR, ctx->buffers[BUF_USERDATA].paddr);
	WRITE_VREG(MREG_CO_MV_START, ctx->buffers[BUF_WORKSPACE].paddr);

	WRITE_VREG(AV_SCRATCH_L, 0);

	/* disable PSCALE for hardware sharing */
	WRITE_VREG(PSCALE_CTRL, 0);

	/* disable mpeg4 */
	WRITE_VREG(M4_CONTROL_REG, 0);
	/* clear mailbox interrupt */
	WRITE_VREG(ASSIST_MBOX1_CLR_REG, 1);
	/* clear buffer IN/OUT registers */
	WRITE_VREG(MREG_BUFFEROUT, 0);
	/* enable mailbox interrupt */
	WRITE_VREG(ASSIST_MBOX1_MASK, 1);

	/* clear error count */
	WRITE_VREG(MREG_ERROR_COUNT, 0);
	/*Use MREG_FATAL_ERROR bit1, the ucode determine
		whether to report the interruption of width and
		height information,in order to be compatible
		with the old version of ucode.
		1: Report the width and height information
		0: No Report
	  bit0:
	        1: Use cma cc buffer for new driver
	        0: use codec mm cc buffer for old driver
		*/
	WRITE_VREG(MREG_FATAL_ERROR, 3);
	/* clear wait buffer status */
	WRITE_VREG(MREG_WAIT_BUFFER, 0);

#ifdef RESTORE
	/* set to mpeg1 default */
	WRITE_VREG(MPEG1_2_REG,
			(hw->ctx_valid) ? hw->reg_mpeg1_2_reg : 0);
	/* for Mpeg1 default value */
	WRITE_VREG(PIC_HEAD_INFO,
			(hw->ctx_valid) ? hw->reg_pic_head_info : 0x380);
	/* set reference width and height */
	if ((hw->frame_width != 0) && (hw->frame_height != 0))
		WRITE_VREG(MREG_CMD,
				(hw->frame_width << 16) | hw->frame_height);
	else
		WRITE_VREG(MREG_CMD, 0);

	WRITE_VREG(MREG_PIC_WIDTH, hw->reg_pic_width);
	WRITE_VREG(MREG_PIC_HEIGHT, hw->reg_pic_height);
	WRITE_VREG(MREG_SEQ_INFO, hw->seqinfo);
	WRITE_VREG(F_CODE_REG, hw->reg_f_code_reg);
	WRITE_VREG(SLICE_VER_POS_PIC_TYPE, hw->reg_slice_ver_pos_pic_type);
	WRITE_VREG(MB_INFO, hw->reg_mb_info);
	WRITE_VREG(VCOP_CTRL_REG, hw->reg_vcop_ctrl_reg);
	WRITE_VREG(AV_SCRATCH_H, hw->reg_signal_type);

	WRITE_VREG(AV_SCRATCH_J, hw->userdata_wp_ctx);
#endif
#if 0
	/* set to mpeg1 default */
	WRITE_VREG(MPEG1_2_REG, 0);
	/* for Mpeg1 default value */
	WRITE_VREG(PIC_HEAD_INFO, 0x380);
	/* set reference width and height */
	WRITE_VREG(MREG_CMD, 0);

	WRITE_VREG(MREG_PIC_WIDTH, 0);
	WRITE_VREG(MREG_PIC_HEIGHT, 0);
	WRITE_VREG(MREG_SEQ_INFO, 0);
	WRITE_VREG(F_CODE_REG, 0);
	WRITE_VREG(SLICE_VER_POS_PIC_TYPE, 0);
	WRITE_VREG(MB_INFO, 0);
	WRITE_VREG(VCOP_CTRL_REG, 0);
	WRITE_VREG(AV_SCRATCH_H, 0);

	WRITE_VREG(AV_SCRATCH_J, 0);
#endif
	/* end vmpeg12_hw_ctx_restore */

	return 0;
}

static int amlvdec_mpeg_configure_input(struct decoder_mpeg_ctx *ctx) {
	struct decoder_task_ctx *t = &ctx->task;
	struct vb2_buffer *vb2 = &t->src_buf->vb2_buf;
	dma_addr_t buf_paddr = vb2_dma_contig_plane_dma_addr(vb2, 0);
	u32 buf_size = vb2_plane_size(vb2, 0);
	u32 data_len = vb2_get_plane_payload(vb2, 0);

#if 0
	WRITE_VREG(MREG_CMD,
			(ctx->pic_width << 16) | ctx->pic_height);
	WRITE_VREG(MREG_PIC_WIDTH, ctx->pic_width);
	WRITE_VREG(MREG_PIC_HEIGHT, ctx->pic_height);
#endif

	amlvdec_vdec_configure_input(buf_paddr, buf_size, data_len);

	WRITE_VREG(VIFF_BIT_CNT, data_len << 3);

	static const u8 NAL_START_CODE[] = {0, 0, 1, 0};
	/* HW needs padding (NAL start) for frame ending */
	if (buf_size >= data_len + sizeof(NAL_START_CODE)) {
		u8 *tail = vb2_plane_vaddr(vb2, 0);
		tail += data_len;
		memcpy(tail, NAL_START_CODE, sizeof(NAL_START_CODE));
	} else {
		job_err(ctx->job, "Not enough space in buffer for tail NAL");
		return -ENOSPC;
	}

#define chunk_offset 0
#define ctx_valid 0
	WRITE_VREG(MREG_INPUT,
			(chunk_offset & 7) |
			(1<<7) | 
			(ctx->init_flag<<6));

	amlvdec_vdec_enable_input();

	job_trace(ctx->job, "src_buf vb2: index=%d, size=0x%x, start=0x%llx, end=0x%llx, bytes=0x%x\n",
			vb2->index,
			buf_size,
			buf_paddr,
			buf_paddr + data_len - 1,
			data_len);
#if 0
	job_trace(ctx->job, "data=%*ph",
			(int) umin(data_len, 128),
			vb2_plane_vaddr(vb2, 0));
#endif
#if 0
	print_hex_dump(KERN_DEBUG,
			"PACKET: ",
			DUMP_PREFIX_NONE,
			16, 2,
			vb2_plane_vaddr(vb2, 0),
			umin(data_len, 256),
			false);
#endif

	return 0;
}

static int amlvdec_mpeg_configure_output(struct decoder_mpeg_ctx *ctx) {
	struct decoder_task_ctx *t = &ctx->task;
	int current_canvas = 0;
	int forward_canvas = 0;
	int backward_canvas = 0;

	current_canvas = config_decode_canvas(ctx, CANVAS_CURRENT_FRAME, t->dst_buf);
	if (current_canvas < 0)
		return current_canvas;

	if (ctx->dpb.forward_ref) {
		forward_canvas = config_decode_canvas(ctx, CANVAS_FORWARD_REF, ctx->dpb.forward_ref->buffer);
		if (forward_canvas < 0)
			return forward_canvas;
	} else {
		forward_canvas = 0xffffffff;
	}

	if (ctx->dpb.backward_ref) {
		backward_canvas = config_decode_canvas(ctx, CANVAS_BACKWARD_REF, ctx->dpb.backward_ref->buffer);
		if (backward_canvas < 0)
			return backward_canvas;
	} else {
		backward_canvas = 0xffffffff;
	}

	/* prepare REF0 & REF1
	   points to the past two IP buffers
	   prepare REC_CANVAS_ADDR and ANC2_CANVAS_ADDR
	   points to the output buffer*/
	WRITE_VREG(MREG_REF1, forward_canvas);
	WRITE_VREG(MREG_REF0, backward_canvas);
	WRITE_VREG(REC_CANVAS_ADDR, current_canvas);
	WRITE_VREG(ANC2_CANVAS_ADDR, current_canvas);

	switch (V4L2_FMT_PIXFMT(ctx->job->dst_fmt)) {
		case V4L2_FMT(NV12):
		case V4L2_FMT(NV12M):
			SET_VREG_MASK(MDEC_PIC_DC_CTRL, 1<<17);
			/* cbcr_merge_swap_en */
			SET_VREG_MASK(MDEC_PIC_DC_CTRL, 1<<16);
			break;
		case V4L2_FMT(NV21):
		case V4L2_FMT(NV21M):
			SET_VREG_MASK(MDEC_PIC_DC_CTRL, 1<<17);
			CLEAR_VREG_MASK(MDEC_PIC_DC_CTRL, 1 << 16);
			break;
		default:
			job_err(ctx->job, "Invalid format: %.4s\n", V4L2_FMT_FOURCC(ctx->job->dst_fmt));
			return -EINVAL;
	}

	return 0;
}

static int vdec_mpeg_header_done(struct decoder_mpeg_ctx *ctx) {
	struct decoder_task_ctx *t = &ctx->task;

	job_trace(ctx->job);

	// TODO resolution change check
	int frame_width = READ_VREG(MREG_PIC_WIDTH);
	int frame_height = READ_VREG(MREG_PIC_HEIGHT);

	job_trace(ctx->job, "info_notify: width=%d, height=%d\n",
			frame_width, frame_height);

	WRITE_VREG(AV_SCRATCH_G, 0);

	return 0;
}

static int vdec_mpeg_picture_done(struct decoder_mpeg_ctx *ctx) {
	struct decoder_task_ctx *t = &ctx->task;

	job_trace(ctx->job);

	bool is_reference;
	bool new_sequence;
	uint16_t poc;
	
	u32 val = READ_VREG(MREG_PIC_INFO);
	picinfo_t *picinfo = (picinfo_t *) &val;
	is_reference = picinfo->type != PIC_TYPE_B;
	new_sequence = picinfo->type == PIC_TYPE_I;
	poc = (u16)(READ_VREG(AV_SCRATCH_1) & GENMASK(9,0));

	t->dpb_pic = amlvdec_mpeg_dpb_new_pic(&ctx->dpb, t->dst_buf, is_reference, new_sequence, poc);
	if (!t->dpb_pic) {
		return -ENOSPC;
	}

	t->dpb_pic->data = (void *)val;
	t->dst_buf = NULL;

	amlvdec_mpeg_dpb_pic_done(&ctx->dpb, t->dpb_pic);

	return 0;
}

static void vdec_mpeg_flush_dpb(struct decoder_mpeg_ctx *ctx, bool flush_all) {
	struct decoder_task_ctx *t = &ctx->task;
	struct vb2_v4l2_buffer *dst_buf;
	dpb_picture_t *pic;

	job_trace(ctx->job);

	while ((pic = amlvdec_mpeg_dpb_next_output(&ctx->dpb, flush_all)) != NULL) {
		dst_buf = pic->buffer;
		
		if (dst_buf->vb2_buf.timestamp) {
			ctx->ts_step = dst_buf->vb2_buf.timestamp - ctx->last_ts;
			ctx->last_ts = dst_buf->vb2_buf.timestamp;
		} else {
			ctx->last_ts += ctx->ts_step;
			dst_buf->vb2_buf.timestamp = ctx->last_ts;
		}

		picinfo_t *picinfo = (picinfo_t *) &pic->data;
		job_trace(ctx->job, "OUTPUT vb2: index=%d, ts=%llu, state=%d, poc=%d, type=%d, error=%d\n",
				dst_buf->vb2_buf.index,
				dst_buf->vb2_buf.timestamp,
				pic->state,
				pic->poc,
				picinfo->type,
				picinfo->is_error);

		if (pic->state == DPB_INIT) {
			v4l2_m2m_buf_done(dst_buf, VB2_BUF_STATE_ERROR);
		} else {
			v4l2_m2m_buf_done(dst_buf, VB2_BUF_STATE_DONE);
		}
		amlvdec_mpeg_dpb_recycle_pic(&ctx->dpb, pic);
	}
}

static void decoder_start(struct decoder_mpeg_ctx *ctx) {
	struct decoder_task_ctx *t = &ctx->task;

	job_trace(ctx->job);

	t->src_buf = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
	if (!t->src_buf) {
		job_err(ctx->job, "no source buffer available");
		decoder_abort(ctx);
		return;
	}

	if (!t->dst_buf) {
		t->dst_buf = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
		if (!t->dst_buf) {
			job_err(ctx->job, "no destination buffer available");
			decoder_abort(ctx);
			return; // -ENOENT;
		}
	}

	/* set dst_buf flags */
	v4l2_m2m_buf_copy_metadata(t->src_buf, t->dst_buf, true);

	/* set destination buffer size */
	int num_planes = V4L2_FMT_NUMPLANES(ctx->job->dst_fmt);
	struct vb2_buffer *vb2 = &t->dst_buf->vb2_buf;
	for(int i = 0; i < num_planes; i++) {
		int size = vb2_plane_size(vb2, i);
		vb2_set_plane_payload(vb2, i, size);
	}

	job_trace(ctx->job, "src_buf vb2: index=%d, payload=%lu, ts=%llu",
			t->src_buf->vb2_buf.index,
			vb2_get_plane_payload(&t->src_buf->vb2_buf, 0),
			t->src_buf->vb2_buf.timestamp);

	amlvdec_vdec_reset();
	if (amlvdec_mpeg_init(ctx) ||
		amlvdec_mpeg_configure_input(ctx) ||
		amlvdec_mpeg_configure_output(ctx)) {
		decoder_abort(ctx);
	}

	amlvdec_vdec_start();

	ctx->init_flag = true;

	decoder_loop(ctx);
}

static void decoder_loop(struct decoder_mpeg_ctx *ctx) {
	struct decoder_task_ctx *t = &ctx->task;

	reinit_completion(&ctx->decoder_completion);

	unsigned long timeout;
	timeout = wait_for_completion_timeout(&ctx->decoder_completion, msecs_to_jiffies(DECODER_TIMEOUT_MS));
	
	job_trace(ctx->job, "status=0x%x\n", ctx->status);
	dump_registers(ctx, __func__);

	if (!timeout) {
		job_err(ctx->job, "decoder timeout\n");
		decoder_abort(ctx);
		return;
	}

	u32 debug_isr = READ_VREG(AV_SCRATCH_M);
	if (debug_isr) {
		job_trace(ctx->job, "debug_isr: 0x%x\n", debug_isr);
		WRITE_VREG(AV_SCRATCH_M, 0);
		decoder_loop(ctx);
		return;
	}

	bool info_notify = READ_VREG(AV_SCRATCH_G);
	if (info_notify) {
		if(vdec_mpeg_header_done(ctx)) {
			decoder_abort(ctx);
			return;
		}
		decoder_loop(ctx);
		return;
	}

	bool userdata_push = READ_VREG(AV_SCRATCH_J) & (1<<16);
	if (userdata_push) {
		job_trace(ctx->job, "userdata_push\n");
		WRITE_VREG(AV_SCRATCH_J, 0);
		decoder_loop(ctx);
		return;
	}

	switch (ctx->status) {
		case MPEG12_DATA_REQUEST:
		case MPEG12_DATA_EMPTY:
			job_err(ctx->job, "invalid decoder status: 0x%x\n", ctx->status);
			//decoder_abort(ctx);
			decoder_done(ctx);
			return;
		case MPEG12_PIC_DONE:
		case MPEG12_SEQ_END:
			if (vdec_mpeg_picture_done(ctx)) {
				decoder_abort(ctx);
				return;
			}
			decoder_done(ctx);
			return;
		default:
			job_err(ctx->job, "unkown decoder status: 0x%x\n", ctx->status);
			decoder_abort(ctx);
			return;
	}
}

static irqreturn_t vdec1_isr(int irq, void *data)
{
	const struct meson_codec_job *job = data;
	struct decoder_mpeg_ctx *ctx = job->priv;
	struct decoder_task_ctx *t = &ctx->task;

	ctx->status = READ_VREG(MREG_BUFFEROUT);

	job_trace(job, "status0x%x\n", ctx->status);

	complete(&ctx->decoder_completion);

	WRITE_VREG(ASSIST_MBOX1_CLR_REG, 1);

	return IRQ_HANDLED;
}

static void decoder_done(struct decoder_mpeg_ctx *ctx) {
	struct decoder_task_ctx *t = &ctx->task;
	
	job_trace(ctx->job);

	amlvdec_vdec_stop();

	ctx->decode_pic_count++;

	v4l2_m2m_buf_done(t->src_buf, VB2_BUF_STATE_DONE);
	t->src_buf = NULL;

	vdec_mpeg_flush_dpb(ctx, false);
	t->dpb_pic = NULL;

	v4l2_m2m_job_finish(ctx->m2m_ctx->m2m_dev, ctx->m2m_ctx); 
}

static void decoder_abort(struct decoder_mpeg_ctx *ctx) {
	struct decoder_task_ctx *t = &ctx->task;

	job_trace(ctx->job, "status=0x%x\n", ctx->status);

	amlvdec_vdec_stop();

	v4l2_m2m_mark_stopped(ctx->m2m_ctx);

	if (t->src_buf) {
		v4l2_m2m_buf_done(t->src_buf, VB2_BUF_STATE_ERROR);
		t->src_buf = NULL;
	}

	if (t->dst_buf) {
		v4l2_m2m_buf_done(t->dst_buf, VB2_BUF_STATE_ERROR);
		t->dst_buf = NULL;
	}

	vdec_mpeg_flush_dpb(ctx, true);
	t->dpb_pic = NULL;
	
	v4l2_m2m_job_finish(ctx->m2m_ctx->m2m_dev, ctx->m2m_ctx);
	meson_vcodec_event_eos(ctx->session);
}

static int load_firmware(struct decoder_mpeg_ctx *ctx) {
	job_trace(ctx->job);
	struct meson_codec_dev *codec = ctx->job->codec;
	struct meson_vcodec_core *core = codec->core;
	struct meson_vcodec_buffer buf;
	int ret;

	buf.size = MC_MAX_SIZE;
	ret = meson_vcodec_request_firmware(codec, &buf);
	if (ret) {
		return ret;
	};

	ret = vdec1_load_firmware(core, buf.paddr);
	if (ret) {
		codec_err(codec, "Load firmware timed out (dma hang?)\n");
		goto release_firmware;
	}

release_firmware:
	meson_vcodec_release_firmware(codec, &buf);
	return ret;
}

static int decoder_mpeg_prepare(struct meson_codec_job *job) {
	struct meson_vcodec_session *session = job->session;
	struct decoder_mpeg_ctx	*ctx;
	int ret, i;

	job_trace(job);

	ctx = kcalloc(1, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
	ctx->job = job;
	ctx->session = session;
	ctx->m2m_ctx = session->m2m_ctx;
	init_completion(&ctx->decoder_completion);
	job->priv = ctx;

	amlvdec_mpeg_dpb_init(&ctx->dpb, MAX_DPB_FRAMES);

	// TODO replace with actual parsed size and refs
	ctx->pic_width = V4L2_FMT_WIDTH(job->src_fmt);
	ctx->pic_height = V4L2_FMT_HEIGHT(job->src_fmt);
	ctx->mb_width = (ctx->pic_width + 15) >> 4;
	ctx->mb_height = (ctx->pic_height + 15) >> 4;
	ctx->mb_total = ctx->mb_width * ctx->mb_height;
//	u32 ref_size = ALIGN(ctx->mb_width, 4) * ALIGN(ctx->mb_height, 4) * MB_MV_SIZE * MAX_DPB_FRAMES;

	ctx->buffers[BUF_WORKSPACE].size = WORKSPACE_SIZE;
	ctx->buffers[BUF_USERDATA].size = CCBUF_SIZE;

	ret = meson_vcodec_buffers_alloc(job->codec, ctx->buffers, MAX_INTERNAL_BUFFERS);
	if (ret) {
		job_err(job, "Failed to allocate buffer\n");
		goto free_ctx;
	}

	ret = load_firmware(ctx);
	if (ret) {
		job_err(job, "Failed to load firmware\n");
		goto free_buffers;
	}

	/* alloc canvases */
	ret = meson_vcodec_canvas_alloc(job->codec, &ctx->canvases[0][0], MAX_DECODE_CANVAS * MAX_NUM_CANVAS);
	if (ret) {
		job_err(job, "Failed to allocate canvases\n");
		goto free_buffers;
	}

	ret = request_irq(session->core->irqs[IRQ_MBOX1], vdec1_isr, IRQF_SHARED, "vdec1", job);
	if (ret) {
		job_err(job, "Failed to request MBOX1 irq\n");
		goto free_canvas;
	}

	return 0;

free_canvas:
	meson_vcodec_canvas_free(job->codec, &ctx->canvases[0][0], MAX_DECODE_CANVAS * MAX_NUM_CANVAS);
free_buffers:
	meson_vcodec_buffers_free(job->codec, ctx->buffers, MAX_INTERNAL_BUFFERS);
free_ctx:
	kfree(ctx);
	job->priv = NULL;
	return ret;
}

static int decoder_mpeg_init(struct meson_codec_dev *codec) {
	struct meson_vcodec_core *core = codec->core;
	int ret;

	codec_trace(codec);

	ret = vdec1_init(core);
	if (ret) {
		dev_err(core->dev, "Failed to init VDEC1\n");
		return ret;
	}

	/* request irq */
	if (!core->irqs[IRQ_MBOX1]) {
		dev_err(core->dev, "Failed to get MBOX1 irq\n");
		ret = -EINVAL;
		goto release_vdec1;
	}

	return 0;

release_vdec1:
	vdec1_release(core);
	return ret;
}

static int decoder_mpeg_start(struct meson_codec_job *job, struct vb2_queue *vq, u32 count) {
	struct decoder_mpeg_ctx *ctx = job->priv;

	v4l2_m2m_update_start_streaming_state(ctx->m2m_ctx, vq);

	return 0;
}

static int decoder_mpeg_queue(struct meson_codec_job *job, struct vb2_v4l2_buffer *vb) {
	struct decoder_mpeg_ctx *ctx = job->priv;

	if (IS_SRC_STREAM(vb->vb2_buf.type)) {
	}

	return 0;
}

static bool decoder_mpeg_ready(struct meson_codec_job *job) {
	return true;
}

static void decoder_mpeg_run(struct meson_codec_job *job) {
	struct decoder_mpeg_ctx *ctx = job->priv;

	decoder_start(ctx);
}

static void decoder_mpeg_abort(struct meson_codec_job *job) {
	struct decoder_mpeg_ctx *ctx = job->priv;

	// v4l2-m2m may call abort even if stopped
	// and job finished earlier with error state
	// let finish the job once again to prevent
	// v4l2-m2m hanging forever
	if (v4l2_m2m_has_stopped(ctx->m2m_ctx)) {
		v4l2_m2m_job_finish(ctx->m2m_ctx->m2m_dev, ctx->m2m_ctx);
	}
}

static int decoder_mpeg_stop(struct meson_codec_job *job, struct vb2_queue *vq) {
	struct decoder_mpeg_ctx *ctx = job->priv;
	struct decoder_task_ctx *t = &ctx->task;

	if (!V4L2_TYPE_IS_OUTPUT(vq->type)) {
		vdec_mpeg_flush_dpb(ctx, true);
		if (t->dst_buf) {
			v4l2_m2m_buf_done(t->dst_buf, VB2_BUF_STATE_ERROR);
			t->dst_buf = NULL;
		}
	}

	v4l2_m2m_update_stop_streaming_state(ctx->m2m_ctx, vq);

	return 0;
}

static void decoder_mpeg_unprepare(struct meson_codec_job *job) {
	struct decoder_mpeg_ctx *ctx = job->priv;

	free_irq(job->session->core->irqs[IRQ_MBOX1], (void *)job);
	meson_vcodec_canvas_free(job->codec, &ctx->canvases[0][0], MAX_DECODE_CANVAS * MAX_NUM_CANVAS);
	meson_vcodec_buffers_free(job->codec, ctx->buffers, MAX_INTERNAL_BUFFERS);
	kfree(ctx);
	job->priv = NULL;
}

static void decoder_mpeg_release(struct meson_codec_dev *codec) {
	vdec1_release(codec->core);
}

static const struct meson_codec_ops mpeg_decoder_ops = {
	.init = &decoder_mpeg_init,
	.prepare = &decoder_mpeg_prepare,
	.start = &decoder_mpeg_start,
	.queue = &decoder_mpeg_queue,
	.ready = &decoder_mpeg_ready,
	.run = &decoder_mpeg_run,
	.abort = &decoder_mpeg_abort,
	.stop = &decoder_mpeg_stop,
	.unprepare = &decoder_mpeg_unprepare,
	.release = &decoder_mpeg_release,
};

const struct meson_codec_spec mpeg2_decoder = {
	.type = MPEG2_DECODER,
	.name = "mpeg2_decoder",
	.ops = &mpeg_decoder_ops,
};

const struct meson_codec_spec mpeg1_decoder = {
	.type = MPEG1_DECODER,
	.name = "mpeg1_decoder",
	.ops = &mpeg_decoder_ops,
};
