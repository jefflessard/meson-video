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
#include "amlvdec_h264_dpb.h"

#include "amlogic.h"
//#include "register.h"
#include "regs/dos_regs.h"
#include "regs/amrisc_regs.h"
#include "regs/vdec_regs.h"

#define MHz (1000000)
#define DECODER_TIMEOUT_MS 5000
#define MB_MV_SIZE 96
#define VLD_PADDING_SIZE 1024

#define CLEAR_VREG_MASK(r, mask)  WRITE_VREG(r, READ_VREG(r) & ~(mask))
#define SET_VREG_MASK(r, mask)    WRITE_VREG(r, READ_VREG(r) | (mask))

#define MC_OFFSET_HEADER    0x0000
#define MC_OFFSET_DATA      0x1000
#define MC_OFFSET_MMCO      0x2000
#define MC_OFFSET_LIST      0x3000
#define MC_OFFSET_SLICE     0x4000
#define MC_OFFSET_MAIN      0x5000

#define MC_MAX_SIZE         (16 * SZ_4K)
#define MC_AMRISC_SIZE      (SZ_4K)
#define MC_TOTAL_SIZE       ((20 + 16) * SZ_1K)
#define MC_SWAP_SIZE        (4 * SZ_1K)
#define LMEM_SIZE           PAGE_SIZE
#define AUX_PREFIX_SIZE     (16 * SZ_1K)
#define AUX_SUFFIX_SIZE     (0)
#define AUX_SIZE            AUX_PREFIX_SIZE + AUX_SUFFIX_SIZE
#define WORKSPACE_SIZE      0x200000
#define WORKSPACE_OFFSET    (64 * SZ_1K)

#define VDEC_FIFO_ALIGN     8

#define ASSIST_MBOX1_CLR_REG VDEC_ASSIST_MBOX1_CLR_REG
#define ASSIST_MBOX1_MASK    VDEC_ASSIST_MBOX1_MASK
#define ASSIST_MBOX1_IRQ_REG VDEC_ASSIST_MBOX1_IRQ_REG

/* unmapped                 AV_SCRATCH_0 */
	/* (max_reference_size << 24) |
	 *     (dpb.mDPB.size << 16) |
	 *     (dpb.mDPB.size << 8) */
/* unmapped                 AV_SCRATCH_1 */
	/* param1 / seq_info2
	 * mb_width = param1 & 0xff;
	 * mb_total = (param1 >> 8) & 0xffff;
	 * mb_mv_byte = (seq_info2 & 0x80000000) ? 24 : 96;*/
	/* reserved_byte*8
	 *  reused to store the reserved bit_cnt for one packet multi-frame */
#define INIT_FLAG_REG       AV_SCRATCH_2
	/* init_flag 0-1 */
	/* param2 / seq_info
	 * frame_mbs_only_flag = (seq_info >> 15) & 0x01;
	 * chroma_format_idc = (seq_info >> 13) & 0x03; */
	/* bit 15: frame_mbs_only_flag
	 * bit 13-14: chroma_format_idc */
#define HEAD_PADDING_REG    AV_SCRATCH_3
	/* set to 0 */
#define H264_DECODE_MODE    AV_SCRATCH_4
	#define DECODE_MODE_SINGLE            0x0
	#define DECODE_MODE_MULTI_FRAMEBASE   0x1
	#define DECODE_MODE_MULTI_STREAMBASE  0x2
	#define DECODE_MODE_MULTI_DVBAL       0x3
	#define DECODE_MODE_MULTI_DVENL       0x4
#define H264_DECODE_SEQINFO	AV_SCRATCH_5
	/* set to seq_info2 */
/* unmapped                 AV_SCRATCH_6 */
	/* param3 */
	/* @AV_SCRATCH_6.31-16 = (left << 8 | right ) << 1
	 * @AV_SCRATCH_6.15-0  = (top << 8 | bottom ) << (2 - frame_mbs_only_flag) */
#define UCODE_WATCHDOG_REG  AV_SCRATCH_7
	/* (max_reference_size << 24) |
	 *      (dpb.mDPB.size << 16) |
	 *      (dpb.mDPB.size << 8) */
/* unmapped                 AV_SCRATCH_8 */
	/* workspace paddr + workspace offset */
#define ERROR_STATUS_REG	AV_SCRATCH_9
	/* set to 0 */
#define NAL_SEARCH_CTL		AV_SCRATCH_9
	/* NAL_SEARCH_CTL: bit 0, enable itu_t35
	 * NAL_SEARCH_CTL: bit 1, enable mmu
	 * NAL_SEARCH_CTL: bit 2, detect frame_mbs_only_flag whether switch resolution
	 * NAL_SEARCH_CTL: bit 7-14,level_idc
	 * NAL_SEARCH_CTL: bit 15,bitstream_restriction_flag */
#define RPM_CMD_REG         AV_SCRATCH_A
	/* related to SEND_PARAM_WITH_REG */
/* unmapped                 AV_SCRATCH_B */
	/* param4 / seq_info3
	   level_idc = param4 & 0xff
	   max_reference_size = (param4 >> 8) & 0xff */
#define H264_AUX_ADR        AV_SCRATCH_C
	/* aux_phy_addr */
/* unmapped                 AV_SCRATCH_D */
#define H264_DECODE_SIZE	AV_SCRATCH_E
	/* decode_size */
/* unmapped                 AV_SCRATCH_F */
	/* ((error_recovery_mode_in & 0x1) << 4) |
	 *    ((UCODE_IP_ONLY_PARAM & 0x1) << 6) */
/* unmapped                 AV_SCRATCH_G */
	/* mc_dma_handle */
#define H264_AUX_DATA_SIZE  AV_SCRATCH_H
	/* ((prefix_aux_size >> 4) << 16) |
	 *        (suffix_aux_size >> 4) */
#define FRAME_COUNTER_REG   AV_SCRATCH_I
	/* decode_pic_count */
#define DPB_STATUS_REG      AV_SCRATCH_J
	#define H264_SLICE_HEAD_DONE         0x01
	#define H264_PIC_DATA_DONE           0x02
	/*#define H264_SPS_DONE              0x03*/
	/*#define H264_PPS_DONE              0x04*/
	/*#define H264_SLICE_DATA_DONE       0x05*/
	/*#define H264_DATA_END              0x06*/
	#define H264_CONFIG_REQUEST          0x11
	#define H264_DATA_REQUEST            0x12
	#define H264_WRRSP_REQUEST           0x13
	#define H264_WRRSP_DONE              0x14
	#define H264_DECODE_BUFEMPTY         0x20
	#define H264_DECODE_TIMEOUT          0x21
	#define H264_SEARCH_BUFEMPTY         0x22
	#define H264_DECODE_OVER_SIZE        0x23
	#define VIDEO_SIGNAL_LOW             0x26
	#define VIDEO_SIGNAL_HIGH            0x27
	#define H264_FIND_NEXT_PIC_NAL       0x50
	#define H264_FIND_NEXT_DVEL_NAL      0x51
	#define H264_AUX_DATA_READY          0x52
	#define H264_SEI_DATA_READY          0x53
	#define H264_SEI_DATA_DONE           0x54
	#define H264_STATE_SEARCH_AFTER_SPS  0x80
	#define H264_STATE_SEARCH_AFTER_PPS  0x81
	#define H264_STATE_PARSE_SLICE_HEAD  0x82
	#define H264_STATE_SEARCH_HEAD       0x83
	#define H264_ACTION_SEARCH_HEAD      0xf0
	#define H264_ACTION_DECODE_SLICE     0xf1
	#define H264_ACTION_CONFIG_DONE      0xf2
	#define H264_ACTION_DECODE_NEWPIC    0xf3
	#define H264_ACTION_DECODE_START     0xff
/* unmapped                 AV_SCRATCH_K */
	/* udebug_flag:
	 *     bit 0, enable ucode print
	 *     bit 1, enable ucode detail print
	 *     bit 3, disable ucode watchdog
	 *     bit [31:16] not 0, pos to dump lmem
	 *                 bit 2, pop bits to lmem
	 *     bit [11:8], pre-pop bits for alignment (when bit 2 is 1) */
#define LMEM_DUMP_ADR       AV_SCRATCH_L
	/* lmem_phy_addr */
#define DEBUG_REG1          AV_SCRATCH_M
	/* debug_tag, set to 0 */
#define DEBUG_REG2          AV_SCRATCH_N
	/* udebug_pause_val, set to 0 */
#define H264_DECODE_INFO    M4_CONTROL_REG /* 0xc29 */
	/* set to (1<<13) */
#define MBY_MBX             MB_MOTION_MODE /*0xc07*/
	/* decode_mb_count = 
	*      ((mby_mbx & 0xff) * mb_width +
	*      (((mby_mbx >> 8) & 0xff) + 1)) */

#define H264_BUFFER_INFO_INDEX    PMV3_X /* 0xc24 */
#define H264_BUFFER_INFO_DATA   PMV2_X  /* 0xc22 */
#define H264_CURRENT_POC_IDX_RESET LAST_SLICE_MV_ADDR /* 0xc30 */
#define H264_CURRENT_POC          LAST_MVY /* 0xc32 shared with conceal MV */

#define H264_CO_MB_WR_ADDR        VLD_C38 /* 0xc38 */
/* bit 31:30 -- L1[0] picture coding structure,
 *	00 - top field,	01 - bottom field,
 *	10 - frame, 11 - mbaff frame
 *   bit 29 - L1[0] top/bot for B field picture , 0 - top, 1 - bot
 *   bit 28:0 h264_co_mb_mem_rd_addr[31:3]
 *	-- only used for B Picture Direct mode [2:0] will set to 3'b000
 */
#define H264_CO_MB_RD_ADDR        VLD_C39 /* 0xc39 */

/* bit 15 -- flush co_mb_data to DDR -- W-Only
 *   bit 14 -- h264_co_mb_mem_wr_addr write Enable -- W-Only
 *   bit 13 -- h264_co_mb_info_wr_ptr write Enable -- W-Only
 *   bit 9 -- soft_reset -- W-Only
 *   bit 8 -- upgent
 *   bit 7:2 -- h264_co_mb_mem_wr_addr
 *   bit 1:0 -- h264_co_mb_info_wr_ptr
 */
#define H264_CO_MB_RW_CTL         VLD_C3D /* 0xc3d */
#define DCAC_DDR_BYTE64_CTL                   0x0e1d

#define LMEM_TO_DPB_INDEX(idx) ((idx / 4) * 4 + (3 - (idx % 4)))
#define PROFILE_IDC_MMCO      LMEM_TO_DPB_INDEX(0XE7)
#define LMEM_GET(ctx, idx) (((u16 *) ctx->buffers[BUF_LMEM].vaddr)[idx])


enum decoder_h264_buffers : u8 {
	BUF_FW,
	BUF_WORKSPACE,
	BUF_LMEM,
	BUF_AUX,
	BUF_REF,
	MAX_INTERNAL_BUFFERS,
};

struct decoder_task_ctx {
	struct vb2_v4l2_buffer *src_buf;
	u32 slice_count;
	dpb_entry_t *dpb_entry;
};

struct decoder_h264_dpb_buffer {
	struct vb2_v4l2_buffer *dst_buf;
	uint32_t info0;
	uint32_t info1;
	uint32_t info2;
};

struct decoder_h264_ctx {
	struct meson_codec_job *job;
	struct meson_vcodec_session *session;
	struct v4l2_m2m_ctx *m2m_ctx;
	struct completion decoder_completion;

	struct decoder_task_ctx task;
	struct meson_vcodec_buffer buffers[MAX_INTERNAL_BUFFERS];
	struct amlvdec_h264_lmem *lmem;
	dpb_buffer_t dpb;

	u8 canvases[MAX_DPB_FRAMES][MAX_NUM_CANVAS];
	struct decoder_h264_dpb_buffer dpb_bufs[MAX_DPB_FRAMES];

	u32 pic_width;
	u32 pic_height;
	u32 mb_width;
	u32 mb_height;
	u32 mb_total;

	u32 cmd;
	u32 status;
	u32 error;

	u32 decode_pic_count;
	bool init_flag;
};

/* decoder forward declarations */
static void decoder_start(struct decoder_h264_ctx *ctx);
static void decoder_loop(struct decoder_h264_ctx *ctx);
static void decoder_done(struct decoder_h264_ctx *ctx);
static void decoder_abort(struct decoder_h264_ctx *ctx);

static void amvdec_start(struct decoder_h264_ctx *ctx)
{
	job_trace(ctx->job);

	/* #if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6 */
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_M6) {
		READ_VREG(DOS_SW_RESET0);
		READ_VREG(DOS_SW_RESET0);
		READ_VREG(DOS_SW_RESET0);

		WRITE_VREG(DOS_SW_RESET0, (1 << 12) | (1 << 11));
		WRITE_VREG(DOS_SW_RESET0, 0);

		READ_VREG(DOS_SW_RESET0);
		READ_VREG(DOS_SW_RESET0);
		READ_VREG(DOS_SW_RESET0);
#ifdef TODO
	} else {
		/* #else */
		/* additional cbus dummy register reading for timing control */
		READ_RESET_REG(RESET0_REGISTER);
		READ_RESET_REG(RESET0_REGISTER);
		READ_RESET_REG(RESET0_REGISTER);
		READ_RESET_REG(RESET0_REGISTER);

		WRITE_RESET_REG(RESET0_REGISTER, RESET_VCPU | RESET_CCPU);

		READ_RESET_REG(RESET0_REGISTER);
		READ_RESET_REG(RESET0_REGISTER);
		READ_RESET_REG(RESET0_REGISTER);
#endif
	}
	/* #endif */

	WRITE_VREG(MPSR, 0x0001);
}

static void amvdec_stop(struct decoder_h264_ctx *ctx)
{
	ulong timeout = jiffies + HZ/10;

	WRITE_VREG(MPSR, 0);
	WRITE_VREG(CPSR, 0);

	while (READ_VREG(IMEM_DMA_CTRL) & 0x8000) {
		if (time_after(jiffies, timeout))
			break;
	}

	timeout = jiffies + HZ/10;
	while (READ_VREG(LMEM_DMA_CTRL) & 0x8000) {
		if (time_after(jiffies, timeout))
			break;
	}

	/* #if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6 */
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_M6) {
		READ_VREG(DOS_SW_RESET0);
		READ_VREG(DOS_SW_RESET0);
		READ_VREG(DOS_SW_RESET0);

		WRITE_VREG(DOS_SW_RESET0, (1 << 12) | (1 << 11));
		WRITE_VREG(DOS_SW_RESET0, 0);

		READ_VREG(DOS_SW_RESET0);
		READ_VREG(DOS_SW_RESET0);
		READ_VREG(DOS_SW_RESET0);
#ifdef TODO
	} else {
		/* #else */
		WRITE_RESET_REG(RESET0_REGISTER, RESET_VCPU | RESET_CCPU);

		/* additional cbus dummy register reading for timing control */
		READ_RESET_REG(RESET0_REGISTER);
		READ_RESET_REG(RESET0_REGISTER);
		READ_RESET_REG(RESET0_REGISTER);
		READ_RESET_REG(RESET0_REGISTER);
#endif
	}
	/* #endif */
} 

void vdec_reset_core(struct decoder_h264_ctx *ctx)
{
	job_trace(ctx->job);
#ifdef TODO
	if ((get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T7) ||
		(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T3) ||
		(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_S5)) {
		/* t7 no dmc req for vdec only */
		vdec_dbus_ctrl(0);
	} else {
		dec_dmc_port_ctrl(0, VDEC_INPUT_TARGET_VLD);
	}
#endif
	/*
	 * 2: assist
	 * 3: vld_reset
	 * 4: vld_part_reset
	 * 5: vfifo reset
	 * 6: iqidct
	 * 7: mc
	 * 8: dblk
	 * 9: pic_dc
	 * 10: psc
	 * 11: mcpu
	 * 12: ccpu
	 * 13: ddr
	 * 14: afifo
	 */
	WRITE_VREG(DOS_SW_RESET0, (1<<3)|(1<<4)|(1<<5)|(1<<7)|(1<<8)|(1<<9));
	WRITE_VREG(DOS_SW_RESET0, 0);

#ifdef TODO
	if ((get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T7) ||
		(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T3) ||
		(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_S5))
		vdec_dbus_ctrl(1);
	else
		dec_dmc_port_ctrl(1, VDEC_INPUT_TARGET_VLD);
#endif
}

static inline void dump_registers(struct decoder_h264_ctx *ctx) {
#define MB_TOTAL_BIT	8
#define MB_TOTAL_MASK	GENMASK(15, 0)
#define MB_WIDTH_MASK	GENMASK(7, 0)
#define MAX_REF_BIT	24
#define MAX_REF_MASK	GENMASK(6, 0)
	struct meson_codec_dev *codec = ctx->job->codec;
	struct meson_vcodec_core *core = codec->core;

	/* param1 */
	u32 param1 = READ_VREG(AV_SCRATCH_1);
	u32 mb_width = param1 & 0xff;
	u32 mb_total = (param1 >> 8) & 0xffff;
	u32 mb_height = 0;
	if (!mb_width && mb_total) /*for 4k2k*/
		mb_width = 256;
	if (mb_width)
		mb_height = mb_total/mb_width;
	u32 frame_width = mb_width << 4;
	u32 frame_height = mb_height << 4;

#if 0
	/* param2 */
	u32 param2 = READ_VREG(AV_SCRATCH_2);
	bool frame_mbs_only_flag = (param2 >> 15) & 0x01;
	u32 chroma_format_idc = (param2 >> 13) & 0x03;

	/* param3 */
	u32 param3 = READ_VREG(AV_SCRATCH_6);
	u32 crop_bottom = (param3 & 0xff) >> (2 - frame_mbs_only_flag);
	u32 crop_right = ((param3 >> 16) & 0xff) >> 1;
	frame_width -= crop_right;
	frame_height -= crop_bottom;
#endif

	/* param4 */
	u32 param4 = READ_VREG(AV_SCRATCH_B);
	u32 level_idc = param4 & 0xff;
	u32 max_ref = (param4 >> 8) & 0xff;

	job_dbg(ctx->job, "max_ref: %u/%u, level=%u, frame: %ux%u\n",
		max_ref, ctx->lmem->params.max_reference_frame_num,
		level_idc,
		frame_width, frame_height);

	job_dbg(ctx->job,
			"DPB_STATUS_REG 0x%x, "
			"ERROR_STATUS_REG 0x%x, "
			"VFIFO lvl=0x%x cur=0x%x rp=0x%x wp=0x%x, "
			"BCNT 0x%x, "
			"MBY_MBX 0x%x, "
			"FC %u, "
			"POC %u, "
			"P1 0x%x (mbw %d mbt %d), "
			"P2 0x%x, "
			"P3 0x%x, "
			"P4 0x%x\n",
			READ_VREG(DPB_STATUS_REG),
			READ_VREG(ERROR_STATUS_REG),
			READ_VREG(VLD_MEM_VIFIFO_LEVEL),
			READ_VREG(VLD_MEM_VIFIFO_CURR_PTR),
			READ_VREG(VLD_MEM_VIFIFO_RP),
			READ_VREG(VLD_MEM_VIFIFO_WP),
			READ_VREG(VIFF_BIT_CNT) >> 3,
			READ_VREG(MBY_MBX),
			READ_VREG(FRAME_COUNTER_REG),
			READ_VREG(H264_CURRENT_POC),
			READ_VREG(AV_SCRATCH_1),
			READ_VREG(AV_SCRATCH_1) & 0xff,
			(READ_VREG(AV_SCRATCH_1) >> 8) & 0xffff,
			READ_VREG(AV_SCRATCH_2),
			READ_VREG(AV_SCRATCH_6),
			READ_VREG(AV_SCRATCH_B));

#if 0
	job_dbg(ctx->job,
			"MBOX1: clr=%d, mask=%d, irq=%d\n",
			READ_VREG(ASSIST_MBOX1_CLR_REG),
			READ_VREG(ASSIST_MBOX1_MASK),
			READ_VREG(ASSIST_MBOX1_IRQ_REG));
#endif

#if 0
	job_dbg(ctx->job,
			"DPB: cropl=%d, cropb=%d, mbh=%d/%d/%d\n",
			ctx->lmem->params.frame_crop_left_offset,
			ctx->lmem->params.frame_crop_bottom_offset,
			ctx->lmem->params.total_mb_height,
			ctx->lmem->params.mb_height, mb_height);
#endif
}

static int config_decode_canvas(struct decoder_h264_ctx *ctx, dpb_entry_t *entry)
{
	int canvas;
	int ret;

	struct decoder_h264_dpb_buffer *dpb_buf = entry->buffer;

	job_trace(ctx->job, "canvas=%d, vb2=%d", entry->buffer_index, dpb_buf->dst_buf->vb2_buf.index);

	ret = meson_vcodec_config_vb2_canvas(
		   ctx->job->codec,
		   ctx->job->dst_fmt,
		   &dpb_buf->dst_buf->vb2_buf,
		   ctx->canvases[entry->buffer_index]);
	if (ret < 0) {
		job_err(ctx->job, "Failed to configure vb2 canvas");
		return ret;
	}
	canvas = ret;

	WRITE_VREG(ANC0_CANVAS_ADDR + entry->buffer_index, canvas);

	return 0;
}

static void vdec_input(dma_addr_t buf_paddr, u32 buf_size, u32 data_len) {
/* begin vdec_prepare_input */
	/* full reset to HW input */
	WRITE_VREG(VLD_MEM_VIFIFO_CONTROL, 0);
	/* reset VLD fifo for all vdec */
	WRITE_VREG(DOS_SW_RESET0, (1<<5) | (1<<4) | (1<<3));
	WRITE_VREG(DOS_SW_RESET0, 0);

	WRITE_VREG(POWER_CTL_VLD, 1 << 4);

	WRITE_VREG(VLD_MEM_VIFIFO_START_PTR, buf_paddr);
	WRITE_VREG(VLD_MEM_VIFIFO_END_PTR, buf_paddr + buf_size - 8);
	WRITE_VREG(VLD_MEM_VIFIFO_CURR_PTR, buf_paddr);

	WRITE_VREG(VLD_MEM_VIFIFO_CONTROL, 1);
	WRITE_VREG(VLD_MEM_VIFIFO_CONTROL, 0);

	/* set to manual mode */
	WRITE_VREG(VLD_MEM_VIFIFO_BUF_CNTL, 2);

	WRITE_VREG(VLD_MEM_VIFIFO_RP, buf_paddr);
#if 1
	u32 dummy = data_len + VLD_PADDING_SIZE;
	if (dummy >= buf_size)
		dummy -= buf_size;
	WRITE_VREG(VLD_MEM_VIFIFO_WP,
			round_down(buf_paddr + dummy, VDEC_FIFO_ALIGN));
#else
	WRITE_VREG(VLD_MEM_VIFIFO_WP, buf_paddr + data_len - 8);
#endif

	WRITE_VREG(VLD_MEM_VIFIFO_BUF_CNTL, 3);
	WRITE_VREG(VLD_MEM_VIFIFO_BUF_CNTL, 2);


#define MEM_VFIFO_BUF_400        BIT(23)
#define MEM_VFIFO_BUF_200        BIT(22)
#define MEM_VFIFO_BUF_100        BIT(21)
#define MEM_VFIFO_BUF_80         BIT(20)
#define MEM_VFIFO_BUF_40         BIT(19)
#define MEM_VFIFO_BUF_20         BIT(18)
#define MEM_VFIFO_16DW           BIT(17)
#define MEM_VFIFO_8DW            BIT(16)
#define MEM_VFIFO_USE_LEVEL      BIT(10)
#define MEM_VFIFO_NO_PARSER      (BIT(3)|BIT(4)|BIT(5)) // (7<<3)
#define MEM_VFIFO_CTRL_EMPTY_EN	 BIT(2)
#define MEM_VFIFO_CTRL_FILL_EN	 BIT(1)
#define MEM_VFIFO_CTRL_INIT      BIT(0)

	// https://github.com/codesnake/uboot-amlogic/blob/master/arch/arm/include/asm/arch-m8/dos_cbus_register.h
	WRITE_VREG(VLD_MEM_VIFIFO_CONTROL,
			MEM_VFIFO_BUF_200 | MEM_VFIFO_16DW |
			MEM_VFIFO_USE_LEVEL |
			MEM_VFIFO_NO_PARSER);
/* end vdec_prepare_input */
}

static void vdec_h264_input(struct decoder_h264_ctx *ctx) {
	struct decoder_task_ctx *t = &ctx->task;
	struct vb2_buffer *vb2 = &t->src_buf->vb2_buf;
	dma_addr_t buf_paddr = vb2_dma_contig_plane_dma_addr(vb2, 0);
	u32 buf_size = vb2_plane_size(vb2, 0);
	u32 data_len = vb2_get_plane_payload(vb2, 0);

	vdec_input(buf_paddr, buf_size, data_len);

	WRITE_VREG(POWER_CTL_VLD,
		(0 << 10) | (1 << 9) | (1 << 6) | (1 << 4));

	WRITE_VREG(H264_DECODE_INFO, (1<<13));
	WRITE_VREG(H264_DECODE_SIZE, data_len);
	WRITE_VREG(VIFF_BIT_CNT, data_len << 3);

	/* vdec_enable_input */
	SET_VREG_MASK(VLD_MEM_VIFIFO_CONTROL, (1<<2) | (1<<1));

	job_trace(ctx->job, "src_buf vb2: index=%d, size=0x%x, start=0x%llx, end=0x%llx, bytes=0x%x\n",
			vb2->index,
			buf_size,
			buf_paddr,
			buf_paddr + data_len - 1,
			data_len);
	job_trace(ctx->job, "data=%*ph",
			(int) umin(data_len, 64),
			vb2_plane_vaddr(vb2, 0));
}

static void vdec_h264_init(struct decoder_h264_ctx *ctx) {
	/* amrisc
	WRITE_VREG(0x301, 0);
	WRITE_VREG(0x31b, 0);
	WRITE_VREG(0x31d, 0);
	*/

	/* swap: stream or restore
	WRITE_VREG(VLD_MEM_SWAP_ADDR, 0);
	WRITE_VREG(VLD_MEM_SWAP_CTL, 0);
	*/

	/* vdec_config_vld_reg: stream
       reset VLD before setting all pointers
	WRITE_VREG(VLD_MEM_VIFIFO_WRAP_COUNT, 0);
	*/

/* begin buffers */
	WRITE_VREG(AV_SCRATCH_G, ctx->buffers[BUF_FW].paddr);
	WRITE_VREG(AV_SCRATCH_8, ctx->buffers[BUF_WORKSPACE].paddr + WORKSPACE_OFFSET);
	WRITE_VREG(LMEM_DUMP_ADR, ctx->buffers[BUF_LMEM].paddr);
	/* config_aux_buf */
	WRITE_VREG(H264_AUX_ADR, ctx->buffers[BUF_AUX].paddr);
	WRITE_VREG(H264_AUX_DATA_SIZE,
		((AUX_PREFIX_SIZE >> 4) << 16) |
		(AUX_SUFFIX_SIZE >> 4));
/* end buffers */

	WRITE_VREG(AV_SCRATCH_1, 0);

#if 0
	WRITE_VREG(AV_SCRATCH_1, 0);
	WRITE_VREG(AV_SCRATCH_7, 0);
	WRITE_VREG(AV_SCRATCH_9, 0);
	WRITE_VREG(AV_SCRATCH_K, 0);
	WRITE_VREG(H264_DECODE_SEQINFO, 0);
	WRITE_VREG(IQIDCT_CONTROL, 0);
	/* h264_vdec_dw_cfg
	WRITE_VREG(MDEC_DOUBLEW_CFG3, 0);
	WRITE_VREG(MDEC_DOUBLEW_CFG4, 0);
	WRITE_VREG(MDEC_DOUBLEW_CFG5, 0);
	*/
	/* set_mmu_config extif_addr
	WRITE_VREG(MDEC_EXTIF_CFG0, 0);
	*//*
	WRITE_VREG(MDEC_EXTIF_CFG1, 0);
	WRITE_VREG(MDEC_EXTIF_CFG2, 0);
	*/
#endif

/* begin vh264_hw_ctx_restore */

	/* disable PSCALE for hardware sharing */
	WRITE_VREG(PSCALE_CTRL, 0);

	/* clear mailbox interrupt */
	WRITE_VREG(ASSIST_MBOX1_CLR_REG, 1);
	/* enable mailbox interrupt */
	WRITE_VREG(ASSIST_MBOX1_MASK, 1);
	/* buffer empty
	WRITE_VREG(ASSIST_MBOX1_IRQ_REG, 1);
	*/

	switch (V4L2_FMT_PIXFMT(ctx->job->dst_fmt)) {
		case V4L2_FMT(NV21):
		case V4L2_FMT(NV21M):
			SET_VREG_MASK(MDEC_PIC_DC_CTRL, 1<<17);
			/* cbcr_merge_swap_en */
			SET_VREG_MASK(MDEC_PIC_DC_CTRL, 1<<16);
			break;
		case V4L2_FMT(NV12):
		case V4L2_FMT(NV12M):
			SET_VREG_MASK(MDEC_PIC_DC_CTRL, 1<<17);
			CLEAR_VREG_MASK(MDEC_PIC_DC_CTRL, 1 << 16);
			break;
		case V4L2_FMT(YUV420):
		case V4L2_FMT(YUV420M):
			CLEAR_VREG_MASK(MDEC_PIC_DC_CTRL, 1<<17);
			CLEAR_VREG_MASK(MDEC_PIC_DC_CTRL, 1 << 16);
			break;
		default:
			job_err(ctx->job, "Invalid format: %.4s\n", V4L2_FMT_FOURCC(ctx->job->dst_fmt));
			break;
	}

	SET_VREG_MASK(MDEC_PIC_DC_CTRL, 0xbf << 24);
	CLEAR_VREG_MASK(MDEC_PIC_DC_CTRL, 0xbf << 24);

	CLEAR_VREG_MASK(MDEC_PIC_DC_CTRL, 1 << 31);

	/*comfig vdec:h264:mdec to use hevc mcr/mcrcc/decomp*/
	CLEAR_VREG_MASK(MDEC_PIC_DC_MUX_CTRL, 1 << 31);
	WRITE_VREG(MDEC_EXTIF_CFG1, 0);

	WRITE_VREG(MDEC_PIC_DC_THRESH, 0x404038aa);

	// TODO MDEC_DOUBLEW_CFG0

#define error_recovery_mode_in 1
#define UCODE_IP_ONLY_PARAM 0
	WRITE_VREG(AV_SCRATCH_F,
			(READ_VREG(AV_SCRATCH_F) & 0xffffffc3) |
			(UCODE_IP_ONLY_PARAM << 6) |
			(error_recovery_mode_in << 4));

	CLEAR_VREG_MASK(AV_SCRATCH_F, 1 << 6);

	if (ctx->init_flag == 0) {
		WRITE_VREG(DPB_STATUS_REG, 0);
	} else {
		/* will trigger a new frame number on H264_ACTION_DECODE_NEWPIC */
		WRITE_VREG(DPB_STATUS_REG, H264_ACTION_DECODE_START);
	}

	WRITE_VREG(FRAME_COUNTER_REG, ctx->decode_pic_count);

	WRITE_VREG(DEBUG_REG1, 0);
	WRITE_VREG(DEBUG_REG2, 0);

#if 0
	WRITE_VREG(IQIDCT_CONTROL, 0x200);
	/*Because CSD data is not found at playback start,
	  the IQIDCT_CONTROL register is not saved,
	  the initialized value 0x200 of IQIDCT_CONTROL is set*/
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_GXL) {
		if (hw->init_flag && (hw->reg_iqidct_control_init_flag == 0))
			WRITE_VREG(IQIDCT_CONTROL, 0x200);
	}
#endif
	// TODO VCOP_CTRL_REG
	// TODO VLD_DECODE_CONTROL

/* end vh264_hw_ctx_restore */

#define enable_itu_t35 0
#define mmu_enable 0
	u8 level_idc = READ_VREG(AV_SCRATCH_B) && 0xff;
	WRITE_VREG(NAL_SEARCH_CTL,
			(enable_itu_t35 << 0) |
			(mmu_enable << 1) |
			(1 << 2) |
			(ctx->lmem->params.sps_flags2.bitstream_restriction_flag << 15) |
			(level_idc << 7));

	WRITE_VREG(MDEC_EXTIF_CFG2, READ_VREG(MDEC_EXTIF_CFG2) | 0x20);

#if 0
#if 0
	WRITE_VREG(AV_SCRATCH_1, 0); // reuse the register AV_SCRATCH_1 to store the reserved bit_cnt for one packet multi-frame
#else
	WRITE_VREG(AV_SCRATCH_1, decode_size << 3); // reuse the register AV_SCRATCH_1 to store the reserved bit_cnt for one packet multi-frame
#endif
#endif
	WRITE_VREG(H264_DECODE_SEQINFO, READ_VREG(AV_SCRATCH_1));
	WRITE_VREG(HEAD_PADDING_REG, 0);

/* begin config_decode_mode */
	WRITE_VREG(H264_DECODE_MODE, DECODE_MODE_MULTI_FRAMEBASE);
	WRITE_VREG(INIT_FLAG_REG, ctx->init_flag);
/* end config_decode_mode */

#if 0
#define udebug_flag 0
	WRITE_VREG(AV_SCRATCH_K, udebug_flag);
#endif

	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_G12A) {
		CLEAR_VREG_MASK(VDEC_ASSIST_MMC_CTRL1, 1 << 3);
	}
}

static void vdec_h264_configure_request(struct decoder_h264_ctx *ctx) {
	job_trace(ctx->job);

#if 0
	dump_dpb_params(&ctx->lmem->params);
	dump_dpb(&ctx->lmem->dpb);
	//dump_mmco(&ctx->lmem->mmco);
#endif

	amlvdec_h264_dpb_adjust_size(&ctx->dpb);

/* begin work DEC_RESULT_CONFIG_PARAM */
	u32 param1 = READ_VREG(AV_SCRATCH_1);
	u32 param2 = READ_VREG(AV_SCRATCH_2);
	u32 param3 = READ_VREG(AV_SCRATCH_6);
	u32 param4 = READ_VREG(AV_SCRATCH_B);

	u32 frame_width = get_picture_width(&ctx->lmem->params);
	u32 frame_height = get_picture_height(&ctx->lmem->params);
	job_dbg(ctx->job, "resolution %ux%u\n", frame_width, frame_height);
	if (ctx->pic_width != frame_width ||
		ctx->pic_height != frame_height) {
		job_info(ctx->job, "resolution changed to %ux%u\n",
				frame_width, frame_height);
		// TODO notify resolution has changed
	}

	WRITE_VREG(AV_SCRATCH_0,
			(ctx->lmem->params.max_reference_frame_num << 24) |
			(ctx->dpb.max_frames << 16) |
			(ctx->dpb.max_frames << 8));
/* end work DEC_RESULT_CONFIG_PARAM */
}

void configure_buffer_info(struct decoder_h264_ctx *ctx, dpb_entry_t *entry) {
	dpb_buffer_t *dpb = &ctx->dpb;
	struct decoder_h264_dpb_buffer *dpb_buf = entry->buffer;

	// Configure current frame buffer info
	uint32_t info0;
	switch (entry->picture_structure) {
		case MMCO_FRAME:
			info0 = 0xf480;
			break;
		case MMCO_TOP_FIELD:
			info0 = 0xf400;
			break;
		case MMCO_BOTTOM_FIELD:
			info0 = 0xf440;
			break;
		case MMCO_MBAFF_FRAME:
			info0 = 0xf4c0;
			break;
	}	

	// Set POC order flag
	if (entry->bottom_poc < entry->top_poc)
		info0 |= 0x100;

	dpb_buf->info0 = info0;
	dpb_buf->info1 = entry->top_poc;
	dpb_buf->info2 = entry->bottom_poc;

	// Write buffer info to hardware
	WRITE_VREG(H264_BUFFER_INFO_INDEX, 16);

	// Write info for all frames in DPB
	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].state == DPB_UNUSED)
			continue;

		job_trace(ctx->job, "INFO buffer index=%d\n", dpb->frames[i].buffer_index);
		dpb_buf = dpb->frames[i].buffer;

		// Update long-term reference flags
		// Set field long term flag
		if (dpb->frames[i].ref_type == DPB_LONG_TERM) {
			dpb_buf->info0 |= (1 << 4);
		} else {
			dpb_buf->info0 &= ~(1 << 4);
		}

		// Set frame long term flag
		if (dpb->frames[i].ref_type == DPB_LONG_TERM && (
			dpb->frames[i].picture_structure == MMCO_FRAME ||
			dpb->frames[i].picture_structure == MMCO_MBAFF_FRAME)) {
			dpb_buf->info0 |= (1 << 5);
		} else {
			dpb_buf->info0 &= ~(1 << 5);
		}

		// Write buffer info registers
		if (dpb->frames[i].buffer == entry->buffer) {
			WRITE_VREG(H264_BUFFER_INFO_DATA, dpb_buf->info0 | 0xf);
		} else {
			WRITE_VREG(H264_BUFFER_INFO_DATA, dpb_buf->info0);
		}

		WRITE_VREG(H264_BUFFER_INFO_DATA, dpb_buf->info1);
		WRITE_VREG(H264_BUFFER_INFO_DATA, dpb_buf->info2);
	}
}

void configure_reference_buffers(struct decoder_h264_ctx *ctx){
	dpb_entry_t **ref_list0 = ctx->dpb.ref_list0;
	int ref_list0_size = ctx->dpb.ref_list0_size;
	dpb_entry_t **ref_list1 = ctx->dpb.ref_list1;
	int ref_list1_size = ctx->dpb.ref_list1_size;
	uint32_t one_ref_cfg;
	uint32_t ref_reg_val;
	int h264_buffer_info_data_write_count;
	int i, j;

	// Configure List 0
	one_ref_cfg = 0;
	ref_reg_val = 0;
	h264_buffer_info_data_write_count = 0;
	WRITE_VREG(H264_BUFFER_INFO_INDEX, 0);
	for (i = 0; i < ref_list0_size; i += 4) {
		for (j = 0; j < 4 && (i+j) < ref_list0_size; j++) {
			dpb_entry_t *ref = ref_list0[i+j];
			uint32_t cfg;

			job_trace(ctx->job, "L0 buffer index=%d\n", ref->buffer_index);
			// Map picture structure to hardware configuration
			switch (ref->picture_structure) {
				case MMCO_TOP_FIELD:
					cfg = 0x1;
					break;
				case MMCO_BOTTOM_FIELD:
					cfg = 0x2;
					break;
				case MMCO_FRAME:
				case MMCO_MBAFF_FRAME:
				default:
					cfg = 0x3;
					break;
			}

			// Pack buffer index and structure
			one_ref_cfg = (ref->buffer_index & 0x1f) | (cfg << 5);
			ref_reg_val = (ref_reg_val << 8) | one_ref_cfg;
		}
		while (j++ < 4) {
			ref_reg_val = (ref_reg_val << 8) | one_ref_cfg;
		}
		WRITE_VREG(H264_BUFFER_INFO_DATA, ref_reg_val);
		h264_buffer_info_data_write_count++;
	}
	while (h264_buffer_info_data_write_count++ < 8) {
		ref_reg_val = (one_ref_cfg << 24) | (one_ref_cfg<<16) | (one_ref_cfg << 8) | one_ref_cfg;
		WRITE_VREG(H264_BUFFER_INFO_DATA, ref_reg_val);
	}

	// Configure List 1
	one_ref_cfg = 0;
	ref_reg_val = 0;
	WRITE_VREG(H264_BUFFER_INFO_INDEX, 8);
	for (i = 0; i < ref_list1_size; i += 4) {
		for (j = 0; j < 4 && (i+j) < ref_list1_size; j++) {
			dpb_entry_t *ref = ref_list1[i+j];
			uint32_t cfg;

			job_trace(ctx->job, "L1 buffer index=%d\n", ref->buffer_index);

			switch (ref->picture_structure) {
				case MMCO_TOP_FIELD:
					cfg = 0x1;
					break;
				case MMCO_BOTTOM_FIELD:
					cfg = 0x2;
					break;
				case MMCO_FRAME:
				case MMCO_MBAFF_FRAME:
				default:
					cfg = 0x3;
					break;
			}

			one_ref_cfg = (ref->buffer_index & 0x1f) | (cfg << 5);
			ref_reg_val = (ref_reg_val << 8) | one_ref_cfg;
		}
		while (j++ < 4) {
			ref_reg_val = (ref_reg_val << 8) | one_ref_cfg;
		}
		WRITE_VREG(H264_BUFFER_INFO_DATA, ref_reg_val);
	}
}

void configure_colocated_buffers(struct decoder_h264_ctx *ctx, const dpb_entry_t *current_pic) {
	const uint16_t first_mb_in_slice = ctx->lmem->params.first_mb_in_slice;
	const dma_addr_t colocated_buf_start = ctx->buffers[BUF_REF].paddr;
	const int colocated_buf_size = ctx->mb_total * MB_MV_SIZE;
	const dpb_entry_t *colocated_ref = amlvdec_h264_dpb_colocated_ref(&ctx->dpb);

	/* configure co-locate buffer */
	while ((READ_VREG(H264_CO_MB_RW_CTL) >> 11) & 0x1);

	bool shift = 0;
	if (ctx->lmem->params.mode_8x8_flags & (0x4 | 0x2)) {
		shift = 2;
	}

	// Calculate write address offset based on frame/field mode
	{

		job_trace(ctx->job, "COLO_WR buffer index=%d\n", current_pic->buffer_index);

		uint32_t colocate_wr_adr = colocated_buf_start +
			((colocated_buf_size * current_pic->buffer_index) >> shift);

		uint32_t colocate_wr_offset = MB_MV_SIZE;
		if (current_pic->picture_structure != MMCO_FRAME) {
			colocate_wr_offset += MB_MV_SIZE; // Field or MBAFF mode
		}
		colocate_wr_offset >>= shift;

		// Configure write address
		WRITE_VREG(H264_CO_MB_WR_ADDR,
				colocate_wr_adr +
				(colocate_wr_offset * first_mb_in_slice));
	}

	// For B slices with colocated reference
	if (colocated_ref) {
		bool cur_colocate_ref_type;
		uint32_t colocate_rd_offset;
		const uint32_t mb_width = ctx->mb_width;
		const uint32_t mby = first_mb_in_slice / mb_width;
		const uint32_t mbx = first_mb_in_slice % mb_width;
		
		job_trace(ctx->job, "COLO_RD buffer index=%d\n", colocated_ref->buffer_index);

		switch (current_pic->picture_structure) {
			case MMCO_TOP_FIELD:
			case MMCO_BOTTOM_FIELD:
				if (colocated_ref->picture_structure == MMCO_BOTTOM_FIELD) {
					cur_colocate_ref_type = 0;
				} else {
					cur_colocate_ref_type = 1;
				}
				if (colocated_ref->picture_structure == MMCO_FRAME) {
					colocate_rd_offset = (mby << 1) * mb_width + mbx;
				} else {
					colocate_rd_offset = first_mb_in_slice << 1;
				}
				break;
			case MMCO_FRAME:
			case MMCO_MBAFF_FRAME:
				if (abs(current_pic->frame_poc - colocated_ref->top_poc) < abs(current_pic->frame_poc - colocated_ref->bottom_poc)) {
					cur_colocate_ref_type = 0;
				} else {
					cur_colocate_ref_type = 1;
				}
				if (colocated_ref->picture_structure < MMCO_FRAME) {
					if (current_pic->picture_structure == MMCO_FRAME) {
						colocate_rd_offset = (mby * mb_width + mbx) << 1;
					} else {
						colocate_rd_offset = ((mby >> 1) * mb_width + mbx) << 1;
					}
				} else if (current_pic->picture_structure == MMCO_FRAME) {
					colocate_rd_offset = first_mb_in_slice;
				} else {
					colocate_rd_offset = first_mb_in_slice << 1;
				}
				break;
		}

		uint32_t colocate_rd_adr = colocated_buf_start +
			((colocated_buf_size * colocated_ref->buffer_index) >> shift);

		colocate_rd_offset *= MB_MV_SIZE;
		colocate_rd_offset >>= shift;

		WRITE_VREG(H264_CO_MB_RD_ADDR,
				((colocate_rd_adr + colocate_rd_offset) >> 3) |
				(colocated_ref->picture_structure << 30) |
				(cur_colocate_ref_type << 29));
	}
}

static void picture_done(dpb_buffer_t *dpb, dpb_entry_t *entry) {
	struct decoder_h264_ctx *ctx = container_of(dpb, struct decoder_h264_ctx, dpb);
	struct decoder_h264_dpb_buffer *dpb_buf = entry->buffer;

	job_trace(ctx->job, "OUTPUT buffer index=%d\n", entry->buffer_index);

	if (!entry->is_gap_frame) {
		if (entry->state == DPB_INIT) {
			v4l2_m2m_buf_done(dpb_buf->dst_buf, VB2_BUF_STATE_ERROR);
		} else {
			v4l2_m2m_buf_done(dpb_buf->dst_buf, VB2_BUF_STATE_DONE);
		}
		dpb_buf->dst_buf = NULL;
	}
}

static bool vdec_h264_header_done(struct decoder_h264_ctx *ctx) {
	struct decoder_task_ctx *t = &ctx->task;

	job_trace(ctx->job);

#if 0
	dump_dpb_params(&ctx->lmem->params);
	dump_dpb(&ctx->lmem->dpb);
	//dump_mmco(&ctx->lmem->mmco);
#endif

	t->dpb_entry = amlvdec_h264_dpb_new_pic(&ctx->dpb);
	dpb_entry_t *entry = t->dpb_entry;
	if (entry == NULL) {
		return false;
	}

	// flush released pictures, if any 
	amlvdec_h264_dpb_flush_output(&ctx->dpb, false, picture_done);

	/* prepare destination buffer */

	struct decoder_h264_dpb_buffer *dpb_buf = entry->buffer;
	dpb_buf->dst_buf = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
	if (!dpb_buf->dst_buf) {
		job_err(ctx->job, "no destination buffer available");
		return false;
	}

	v4l2_m2m_buf_copy_metadata(t->src_buf, dpb_buf->dst_buf, true);

	int num_planes = V4L2_FMT_NUMPLANES(ctx->job->dst_fmt);
	struct vb2_buffer *vb2 = &dpb_buf->dst_buf->vb2_buf;
	for(int i = 0; i < num_planes; i++) {
		int size = vb2_plane_size(vb2, i);
		vb2_set_plane_payload(vb2, i, size);
	}


#if 0
	config_decode_canvas(ctx, entry);
#else
	for (int i = 0; i < ctx->dpb.max_frames; i++) {
		if (ctx->dpb.frames[i].state != DPB_UNUSED) {
			config_decode_canvas(ctx, &ctx->dpb.frames[i]);
		}
	}
#endif

	job_dbg(ctx->job, "POC: %d, %d, %d\n",
			entry->frame_poc,
			entry->top_poc,
			entry->bottom_poc);

	/* begin config_decode_buf */
	WRITE_VREG(H264_CURRENT_POC_IDX_RESET, 0);
	WRITE_VREG(H264_CURRENT_POC, entry->frame_poc);
	WRITE_VREG(H264_CURRENT_POC, entry->top_poc);
	WRITE_VREG(H264_CURRENT_POC, entry->bottom_poc);

#if 0
	WRITE_VREG(MDEC_DOUBLEW_CFG0, 0);
#endif

	WRITE_VREG(CURR_CANVAS_CTRL, entry->buffer_index << 24);
	u32 dst_canvas = READ_VREG(CURR_CANVAS_CTRL) & 0xffffff;
	WRITE_VREG(REC_CANVAS_ADDR, dst_canvas);
	WRITE_VREG(DBKR_CANVAS_ADDR, dst_canvas);
	WRITE_VREG(DBKW_CANVAS_ADDR, dst_canvas);

#if 0	
	WRITE_VREG(MDEC_DOUBLEW_CFG1, dst_canvas);
#endif

	configure_buffer_info(ctx, entry);
	configure_reference_buffers(ctx);
	configure_colocated_buffers(ctx, entry);

	/* end config_decode_buf */

#if 0
	amlvdec_h264_dpb_dump(&ctx->dpb);
#endif

	return true;
}

static void decoder_done(struct decoder_h264_ctx *ctx) {
	struct decoder_task_ctx *t = &ctx->task;
	
	job_trace(ctx->job);

#if 0
	dump_dpb_params(&ctx->lmem->params);
	dump_dpb(&ctx->lmem->dpb);
	//dump_mmco(&ctx->lmem->mmco);
#endif

	amvdec_stop(ctx);

	ctx->decode_pic_count++;

	v4l2_m2m_buf_done(t->src_buf, VB2_BUF_STATE_DONE);
	t->src_buf = NULL;

	// holds buffer when reference in use
	amlvdec_h264_dpb_pic_done(&ctx->dpb, t->dpb_entry);
	// flush released pictures, if any
	amlvdec_h264_dpb_flush_output(&ctx->dpb, false, picture_done);
	t->dpb_entry = NULL;

	v4l2_m2m_job_finish(ctx->m2m_ctx->m2m_dev, ctx->m2m_ctx); 
}

static void decoder_abort(struct decoder_h264_ctx *ctx) {
	struct decoder_task_ctx *t = &ctx->task;

	job_trace(ctx->job, "status=0x%x, error=0x%x\n", ctx->status, ctx->error);
	dump_registers(ctx);

	WRITE_VREG(ASSIST_MBOX1_MASK, 0);
	amvdec_stop(ctx);

	v4l2_m2m_mark_stopped(ctx->m2m_ctx);

	v4l2_m2m_buf_done(t->src_buf, VB2_BUF_STATE_ERROR);
	t->src_buf = NULL;

	// force flush remaining pictures
	amlvdec_h264_dpb_flush_output(&ctx->dpb, true, picture_done);

	v4l2_m2m_job_finish(ctx->m2m_ctx->m2m_dev, ctx->m2m_ctx);
	
	meson_vcodec_event_eos(ctx->session);
}

static void decoder_start(struct decoder_h264_ctx *ctx) {
	struct decoder_task_ctx *t = &ctx->task;

	job_trace(ctx->job);

	t->src_buf = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
	if (!t->src_buf) {
		job_err(ctx->job, "no source buffer available");
		decoder_abort(ctx);
		return;
	}

	job_trace(ctx->job, "src_buf vb2: index=%d, payload=%lu, ts=%llu",
			t->src_buf->vb2_buf.index,
			vb2_get_plane_payload(&t->src_buf->vb2_buf, 0),
			t->src_buf->vb2_buf.timestamp);

	vdec_reset_core(ctx);
	vdec_h264_init(ctx);
	vdec_h264_input(ctx);
	amvdec_start(ctx);

	ctx->cmd = H264_ACTION_SEARCH_HEAD;

	ctx->init_flag = true;
	t->slice_count = 0;

	decoder_loop(ctx);
}

static void decoder_loop(struct decoder_h264_ctx *ctx) {
	struct decoder_task_ctx *t = &ctx->task;

	reinit_completion(&ctx->decoder_completion);

	ctx->status = 0;
	ctx->error = 0;

	dump_registers(ctx);

	WRITE_VREG(DPB_STATUS_REG, ctx->cmd);

	unsigned long timeout;
	timeout = wait_for_completion_timeout(&ctx->decoder_completion, msecs_to_jiffies(DECODER_TIMEOUT_MS));
	
	job_trace(ctx->job, "cmd=%x, status=0x%x, error=0x%x\n", ctx->cmd, ctx->status, ctx->error);

	if (!timeout) {
		job_err(ctx->job, "decoder timeout\n");
		decoder_abort(ctx);
		return;
	}

	if (ctx->error) {
		job_err(ctx->job, "decoder error: 0x%x\n", ctx->error);
		decoder_abort(ctx);
		return;
	}

	switch (ctx->status) {
		/* vdec_schedule_work cases */
		case H264_SEARCH_BUFEMPTY:
		case H264_DATA_REQUEST:
		/* single frame processing, process as error cases */

		/* error cases */
		case H264_DECODE_BUFEMPTY:
		case H264_DECODE_TIMEOUT:
		case H264_DECODE_OVER_SIZE:
			job_err(ctx->job, "invalid decoder status: 0x%x\n", ctx->status);
			decoder_abort(ctx);
			return;

		/* decoder loop cases */
		case H264_CONFIG_REQUEST:
			vdec_h264_configure_request(ctx);
			ctx->cmd = H264_ACTION_CONFIG_DONE;
			decoder_loop(ctx);
			return;
		case H264_SLICE_HEAD_DONE:
			if (t->slice_count == 0) {
				if(!vdec_h264_header_done(ctx)) {
					decoder_abort(ctx);
					return;
				}
				ctx->cmd = H264_ACTION_DECODE_NEWPIC;
			} else {
				ctx->cmd = H264_ACTION_DECODE_SLICE;
			}
			t->slice_count++;
			decoder_loop(ctx);
			return;

		/* data ready cases */
		case H264_AUX_DATA_READY:
			job_info(ctx->job, "ignoring aux data");
			// TODO whats next ?
			decoder_loop(ctx);
			return;
		case H264_SEI_DATA_READY:
			job_info(ctx->job, "ignoring sei data");
			ctx->cmd = H264_SEI_DATA_DONE;
			decoder_loop(ctx);
			return;

		/* picture done cases */
		case H264_PIC_DATA_DONE:
		/* CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISIO
		case H264_FIND_NEXT_PIC_NAL:
		case H264_FIND_NEXT_DVEL_NAL: */
			decoder_done(ctx);
			return;

		/* unmanaged cases */
		default:
			job_err(ctx->job, "unkown decoder status: 0x%x\n", ctx->status);
			decoder_abort(ctx);
			return;
	}
}

static irqreturn_t vdec1_isr(int irq, void *data)
{
	const struct meson_codec_job *job = data;
	struct decoder_h264_ctx *ctx = job->priv;
	struct decoder_task_ctx *t = &ctx->task;

	WRITE_VREG(ASSIST_MBOX1_CLR_REG, 1);
	ctx->status = READ_VREG(DPB_STATUS_REG);
	ctx->error = READ_VREG(ERROR_STATUS_REG);

	job_trace(job, "status=0x%x, error=0x%x\n", ctx->status, ctx->error);

	if (ctx->status == H264_WRRSP_REQUEST) {
		WRITE_VREG(DPB_STATUS_REG, H264_WRRSP_DONE);
		return IRQ_HANDLED;
	}

	complete(&ctx->decoder_completion);

	return IRQ_HANDLED;
}

static int vdec1_load_firmware(struct meson_vcodec_core *core, dma_addr_t paddr) {
	int ret;

	meson_dos_write(core, MPSR, 0);
	meson_dos_write(core, CPSR, 0);

#if 0
	meson_dos_clear_bits(core, MDEC_PIC_DC_CTRL, BIT(31));
#endif

	meson_dos_write(core, IMEM_DMA_ADR, paddr);
	meson_dos_write(core, IMEM_DMA_COUNT, MC_AMRISC_SIZE);
	meson_dos_write(core, IMEM_DMA_CTRL, (0x8000 | (7 << 16)));

	bool is_dma_complete(void) {
		return !(meson_dos_read(core, IMEM_DMA_CTRL) & 0x8000);
	}
	return read_poll_timeout(
			is_dma_complete, ret, ret, 10000, 1000000, true);
}

static int load_firmware(struct decoder_h264_ctx *ctx) {
	job_trace(ctx->job);
	struct meson_codec_dev *codec = ctx->job->codec;
	struct meson_vcodec_core *core = codec->core;
	struct meson_vcodec_buffer *fw = &ctx->buffers[BUF_FW];
	struct meson_vcodec_buffer buf;
	int ret;

	buf.size = MC_MAX_SIZE;
	ret = meson_vcodec_request_firmware(codec, &buf);
	if (ret) {
		return ret;
	};

	/*header*/
	memcpy(fw->vaddr + MC_OFFSET_HEADER, buf.vaddr + 0x4000, MC_SWAP_SIZE);
	/*data*/
	memcpy(fw->vaddr + MC_OFFSET_DATA, buf.vaddr + 0x2000, MC_SWAP_SIZE);
	/*mmco*/
	memcpy(fw->vaddr + MC_OFFSET_MMCO, buf.vaddr + 0x6000, MC_SWAP_SIZE);
	/*list*/
	memcpy(fw->vaddr + MC_OFFSET_LIST, buf.vaddr + 0x3000, MC_SWAP_SIZE);
	/*slice*/
	memcpy(fw->vaddr + MC_OFFSET_SLICE, buf.vaddr + 0x5000, MC_SWAP_SIZE);
	/*main*/
	memcpy(fw->vaddr + MC_OFFSET_MAIN, buf.vaddr, 0x2000);
	/*data*/
	memcpy(fw->vaddr + MC_OFFSET_MAIN + 0x2000, buf.vaddr + 0x2000, 0x1000);
	/*slice*/
	memcpy(fw->vaddr + MC_OFFSET_MAIN + 0x3000, buf.vaddr + 0x5000, 0x1000);

	ret = vdec1_load_firmware(core, buf.paddr);
	if (ret) {
		codec_err(codec, "Load firmware timed out (dma hang?)\n");
		goto release_firmware;
	}

release_firmware:
	meson_vcodec_release_firmware(codec, &buf);
	return ret;
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

#ifdef MDEC
	meson_dos_clear_bits(core, MDEC_PIC_DC_CTRL, BIT(31));
#endif

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

	return 0;
}

static int decoder_h264_prepare(struct meson_codec_job *job) {
	struct meson_vcodec_session *session = job->session;
	struct decoder_h264_ctx	*ctx;
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

	// TODO replace with actual parsed size and refs
	ctx->pic_width = V4L2_FMT_WIDTH(job->src_fmt);
	ctx->pic_height = V4L2_FMT_HEIGHT(job->src_fmt);
	ctx->mb_width = (ctx->pic_width + 15) >> 4;
	ctx->mb_height = (ctx->pic_height + 15) >> 4;
	ctx->mb_total = ctx->mb_width * ctx->mb_height;
	u32 ref_size = ALIGN(ctx->mb_width, 4) * ALIGN(ctx->mb_height, 4) * MB_MV_SIZE * MAX_DPB_FRAMES;

	ctx->buffers[BUF_FW].size = MC_TOTAL_SIZE;
	ctx->buffers[BUF_WORKSPACE].size = WORKSPACE_SIZE;
	ctx->buffers[BUF_LMEM].size = LMEM_SIZE;
	ctx->buffers[BUF_AUX].size = AUX_SIZE;
	ctx->buffers[BUF_REF].size = ref_size;

	ret = meson_vcodec_buffers_alloc(job->codec, ctx->buffers, MAX_INTERNAL_BUFFERS);
	if (ret) {
		job_err(job, "Failed to allocate buffer\n");
		goto free_ctx;
	}
	ctx->lmem = ctx->buffers[BUF_LMEM].vaddr;

	ret = load_firmware(ctx);
	if (ret) {
		job_err(job, "Failed to load firmware\n");
		goto free_buffers;
	}

	/* alloc canvases */
	ret = meson_vcodec_canvas_alloc(job->codec, &ctx->canvases[0][0], MAX_DPB_FRAMES * MAX_NUM_CANVAS);
	if (ret) {
		job_err(job, "Failed to allocate canvases\n");
		goto free_buffers;
	}

	amlvdec_h264_dpb_init(&ctx->dpb, ctx->lmem);
	for (i = 0; i < MAX_DPB_FRAMES; i++) {
		ctx->dpb.frames[i].buffer_index = i;
		ctx->dpb.frames[i].buffer = &ctx->dpb_bufs[i];
	}

	ret = request_irq(session->core->irqs[IRQ_MBOX1], vdec1_isr, IRQF_SHARED, "vdec1", job);
	if (ret) {
		job_err(job, "Failed to request MBOX1 irq\n");
		goto free_canvas;
	}

	return 0;

free_canvas:
	meson_vcodec_canvas_free(job->codec, &ctx->canvases[0][0], MAX_DPB_FRAMES * MAX_NUM_CANVAS);
free_buffers:
	meson_vcodec_buffers_free(job->codec, ctx->buffers, MAX_INTERNAL_BUFFERS);
free_ctx:
	kfree(ctx);
	job->priv = NULL;
	return ret;
}

static int decoder_h264_init(struct meson_codec_dev *codec) {
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

static int decoder_h264_start(struct meson_codec_job *job, struct vb2_queue *vq, u32 count) {
	struct decoder_h264_ctx *ctx = job->priv;

	v4l2_m2m_update_start_streaming_state(ctx->m2m_ctx, vq);

	return 0;
}

static int decoder_h264_queue(struct meson_codec_job *job, struct vb2_v4l2_buffer *vb) {
	struct decoder_h264_ctx *ctx = job->priv;

	if (IS_SRC_STREAM(vb->vb2_buf.type)) {
	}

	return 0;
}

static bool decoder_h264_ready(struct meson_codec_job *job) {
	return true;
}

static void decoder_h264_run(struct meson_codec_job *job) {
	struct decoder_h264_ctx *ctx = job->priv;

	decoder_start(ctx);
}

static void decoder_h264_abort(struct meson_codec_job *job) {
	struct decoder_h264_ctx *ctx = job->priv;

	// v4l2-m2m may call abort even if stopped
	// and job finished earlier with error state
	// let finish the job once again to prevent
	// v4l2-m2m hanging forever
	if (v4l2_m2m_has_stopped(ctx->m2m_ctx)) {
		v4l2_m2m_job_finish(ctx->m2m_ctx->m2m_dev, ctx->m2m_ctx);
	}
}

static int decoder_h264_stop(struct meson_codec_job *job, struct vb2_queue *vq) {
	struct decoder_h264_ctx *ctx = job->priv;

	if (!V4L2_TYPE_IS_OUTPUT(vq->type)) {
		// force flush remaining pictures
		amlvdec_h264_dpb_flush_output(&ctx->dpb, true, picture_done);
	}

	v4l2_m2m_update_stop_streaming_state(ctx->m2m_ctx, vq);

	return 0;
}

static void decoder_h264_unprepare(struct meson_codec_job *job) {
	struct decoder_h264_ctx *ctx = job->priv;

	free_irq(job->session->core->irqs[IRQ_MBOX1], (void *)job);
	meson_vcodec_canvas_free(job->codec, &ctx->canvases[0][0], MAX_DPB_FRAMES * MAX_NUM_CANVAS);
	meson_vcodec_buffers_free(job->codec, ctx->buffers, MAX_INTERNAL_BUFFERS);
	kfree(ctx);
	job->priv = NULL;
}

static void decoder_h264_release(struct meson_codec_dev *codec) {
	vdec1_release(codec->core);
}

static const struct meson_codec_ops h264_decoder_ops = {
	.init = &decoder_h264_init,
	.prepare = &decoder_h264_prepare,
	.start = &decoder_h264_start,
	.queue = &decoder_h264_queue,
	.ready = &decoder_h264_ready,
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
};
