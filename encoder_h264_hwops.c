#include <linux/io.h>

#include "meson-platforms.h"
#include "encoder_h264_hwops.h"
#include "hcodec_regs.h"
#include "dos_regs.h"


#define IE_PIPPELINE_BLOCK_SHIFT 0
#define IE_PIPPELINE_BLOCK_MASK 0x1f
#define ME_PIXEL_MODE_SHIFT 5
#define ME_PIXEL_MODE_MASK 0x3

/********************************************
 *  AV Scratch Register Re-Define
 ****************************************** *
 */
#define ENCODER_STATUS HCODEC_HENC_SCRATCH_0
#define MEM_OFFSET_REG HCODEC_HENC_SCRATCH_1
#define DEBUG_REG HCODEC_HENC_SCRATCH_2
#define IDR_PIC_ID HCODEC_HENC_SCRATCH_5
#define FRAME_NUMBER HCODEC_HENC_SCRATCH_6
#define PIC_ORDER_CNT_LSB HCODEC_HENC_SCRATCH_7
#define LOG2_MAX_PIC_ORDER_CNT_LSB HCODEC_HENC_SCRATCH_8
#define LOG2_MAX_FRAME_NUM HCODEC_HENC_SCRATCH_9
#define ANC0_BUFFER_ID HCODEC_HENC_SCRATCH_A
#define QPPICTURE HCODEC_HENC_SCRATCH_B
#define IE_ME_MB_TYPE HCODEC_HENC_SCRATCH_D
/* bit 0-4, IE_PIPPELINE_BLOCK
 * bit 5    me half pixel in m8
 *		disable i4x4 in gxbb
 * bit 6    me step2 sub pixel in m8
 *		disable i16x16 in gxbb
 */
#define IE_ME_MODE HCODEC_HENC_SCRATCH_E
#define IE_REF_SEL HCODEC_HENC_SCRATCH_F
/* [31:0] NUM_ROWS_PER_SLICE_P */
/* [15:0] NUM_ROWS_PER_SLICE_I */
#define FIXED_SLICE_CFG HCODEC_HENC_SCRATCH_L
/* For GX */
#define INFO_DUMP_START_ADDR HCODEC_HENC_SCRATCH_I
/* For CBR */
#define H264_ENC_CBR_TABLE_ADDR HCODEC_HENC_SCRATCH_3
#define H264_ENC_CBR_MB_SIZE_ADDR HCODEC_HENC_SCRATCH_4
/* Bytes(Float) * 256 */
#define H264_ENC_CBR_CTL HCODEC_HENC_SCRATCH_G
/* [31:28] : init qp table idx */
/* [27:24] : short_term adjust shift */
/* [23:16] : Long_term MB_Number between adjust, */
/* [15:0] Long_term adjust threshold(Bytes) */
#define H264_ENC_CBR_TARGET_SIZE HCODEC_HENC_SCRATCH_H
/* Bytes(Float) * 256 */
#define H264_ENC_CBR_PREV_BYTES HCODEC_HENC_SCRATCH_J
#define H264_ENC_CBR_REGION_SIZE HCODEC_HENC_SCRATCH_J
/* for SVC */
#define H264_ENC_SVC_PIC_TYPE HCODEC_HENC_SCRATCH_K

/* define for PIC  header */
#define ENC_SLC_REF 0x8410
#define ENC_SLC_NON_REF 0x8010

/* --------------------------------------------------- */
/* ENCODER_STATUS define */
/* --------------------------------------------------- */
#define ENCODER_IDLE 0
#define ENCODER_SEQUENCE 1
#define ENCODER_PICTURE 2
#define ENCODER_IDR 3
#define ENCODER_NON_IDR 4
#define ENCODER_MB_HEADER 5
#define ENCODER_MB_DATA 6

#define ENCODER_SEQUENCE_DONE 7
#define ENCODER_PICTURE_DONE 8
#define ENCODER_IDR_DONE 9
#define ENCODER_NON_IDR_DONE 10
#define ENCODER_MB_HEADER_DONE 11
#define ENCODER_MB_DATA_DONE 12

#define ENCODER_NON_IDR_INTRA 13
#define ENCODER_NON_IDR_INTER 14

#define ENCODER_ERROR 0xff

/********************************************
 * defines for H.264 mb_type
 *******************************************
 */
#define HENC_MB_Type_PBSKIP 0x0
#define HENC_MB_Type_PSKIP 0x0
#define HENC_MB_Type_BSKIP_DIRECT 0x0
#define HENC_MB_Type_P16x16 0x1
#define HENC_MB_Type_P16x8 0x2
#define HENC_MB_Type_P8x16 0x3
#define HENC_MB_Type_SMB8x8 0x4
#define HENC_MB_Type_SMB8x4 0x5
#define HENC_MB_Type_SMB4x8 0x6
#define HENC_MB_Type_SMB4x4 0x7
#define HENC_MB_Type_P8x8 0x8
#define HENC_MB_Type_I4MB 0x9
#define HENC_MB_Type_I16MB 0xa
#define HENC_MB_Type_IBLOCK 0xb
#define HENC_MB_Type_SI4MB 0xc
#define HENC_MB_Type_I8MB 0xd
#define HENC_MB_Type_IPCM 0xe
#define HENC_MB_Type_AUTO 0xf

#define HENC_MB_CBP_AUTO 0xff
#define HENC_SKIP_RUN_AUTO 0xffff

/* for temp */
#define HCODEC_MFDIN_REGC_MBLP (HCODEC_MFDIN_REGB_AMPC + 0x1)
#define HCODEC_MFDIN_REG0D (HCODEC_MFDIN_REGB_AMPC + 0x2)
#define HCODEC_MFDIN_REG0E (HCODEC_MFDIN_REGB_AMPC + 0x3)
#define HCODEC_MFDIN_REG0F (HCODEC_MFDIN_REGB_AMPC + 0x4)
#define HCODEC_MFDIN_REG10 (HCODEC_MFDIN_REGB_AMPC + 0x5)
#define HCODEC_MFDIN_REG11 (HCODEC_MFDIN_REGB_AMPC + 0x6)
#define HCODEC_MFDIN_REG12 (HCODEC_MFDIN_REGB_AMPC + 0x7)
#define HCODEC_MFDIN_REG13 (HCODEC_MFDIN_REGB_AMPC + 0x8)
#define HCODEC_MFDIN_REG14 (HCODEC_MFDIN_REGB_AMPC + 0x9)
#define HCODEC_MFDIN_REG15 (HCODEC_MFDIN_REGB_AMPC + 0xa)
#define HCODEC_MFDIN_REG16 (HCODEC_MFDIN_REGB_AMPC + 0xb)

/* y tnr */
static unsigned int y_tnr_mc_en = 1;
static unsigned int y_tnr_txt_mode;
static unsigned int y_tnr_mot_sad_margin = 1;
static unsigned int y_tnr_mot_cortxt_rate = 1;
static unsigned int y_tnr_mot_distxt_ofst = 5;
static unsigned int y_tnr_mot_distxt_rate = 4;
static unsigned int y_tnr_mot_dismot_ofst = 4;
static unsigned int y_tnr_mot_frcsad_lock = 8;
static unsigned int y_tnr_mot2alp_frc_gain = 10;
static unsigned int y_tnr_mot2alp_nrm_gain = 216;
static unsigned int y_tnr_mot2alp_dis_gain = 128;
static unsigned int y_tnr_mot2alp_dis_ofst = 32;
static unsigned int y_tnr_alpha_min = 32;
static unsigned int y_tnr_alpha_max = 63;
static unsigned int y_tnr_deghost_os;
/* c tnr */
static unsigned int c_tnr_mc_en = 1;
static unsigned int c_tnr_txt_mode;
static unsigned int c_tnr_mot_sad_margin = 1;
static unsigned int c_tnr_mot_cortxt_rate = 1;
static unsigned int c_tnr_mot_distxt_ofst = 5;
static unsigned int c_tnr_mot_distxt_rate = 4;
static unsigned int c_tnr_mot_dismot_ofst = 4;
static unsigned int c_tnr_mot_frcsad_lock = 8;
static unsigned int c_tnr_mot2alp_frc_gain = 10;
static unsigned int c_tnr_mot2alp_nrm_gain = 216;
static unsigned int c_tnr_mot2alp_dis_gain = 128;
static unsigned int c_tnr_mot2alp_dis_ofst = 32;
static unsigned int c_tnr_alpha_min = 32;
static unsigned int c_tnr_alpha_max = 63;
static unsigned int c_tnr_deghost_os;
/* y snr */
static unsigned int y_snr_err_norm = 1;
static unsigned int y_snr_gau_bld_core = 1;
static int y_snr_gau_bld_ofst = -1;
static unsigned int y_snr_gau_bld_rate = 48;
static unsigned int y_snr_gau_alp0_min;
static unsigned int y_snr_gau_alp0_max = 63;
static unsigned int y_bld_beta2alp_rate = 16;
static unsigned int y_bld_beta_min;
static unsigned int y_bld_beta_max = 63;
/* c snr */
static unsigned int c_snr_err_norm = 1;
static unsigned int c_snr_gau_bld_core = 1;
static int c_snr_gau_bld_ofst = -1;
static unsigned int c_snr_gau_bld_rate = 48;
static unsigned int c_snr_gau_alp0_min;
static unsigned int c_snr_gau_alp0_max = 63;
static unsigned int c_bld_beta2alp_rate = 16;
static unsigned int c_bld_beta_min;
static unsigned int c_bld_beta_max = 63;

#define ADV_MV_LARGE_16x8 1
#define ADV_MV_LARGE_8x16 1
#define ADV_MV_LARGE_16x16 1

/* me weight offset should not very small, it used by v1 me module. */
/* the min real sad for me is 16 by hardware. */
#define ME_WEIGHT_OFFSET 0x520
#define I4MB_WEIGHT_OFFSET 0x655
#define I16MB_WEIGHT_OFFSET 0x560

#define ADV_MV_16x16_WEIGHT 0x080
#define ADV_MV_16_8_WEIGHT 0x0e0
#define ADV_MV_8x8_WEIGHT 0x240
#define ADV_MV_4x4x4_WEIGHT 0x3000

#define IE_SAD_SHIFT_I16 0x001
#define IE_SAD_SHIFT_I4 0x001
#define ME_SAD_SHIFT_INTER 0x001

#define STEP_2_SKIP_SAD 0
#define STEP_1_SKIP_SAD 0
#define STEP_0_SKIP_SAD 0
#define STEP_2_SKIP_WEIGHT 0
#define STEP_1_SKIP_WEIGHT 0
#define STEP_0_SKIP_WEIGHT 0

#define ME_SAD_RANGE_0 0x1 /* 0x0 */
#define ME_SAD_RANGE_1 0x0
#define ME_SAD_RANGE_2 0x0
#define ME_SAD_RANGE_3 0x0

/* use 0 for v3, 0x18 for v2 */
#define ME_MV_PRE_WEIGHT_0 0x18
/* use 0 for v3, 0x18 for v2 */
#define ME_MV_PRE_WEIGHT_1 0x18
#define ME_MV_PRE_WEIGHT_2 0x0
#define ME_MV_PRE_WEIGHT_3 0x0

/* use 0 for v3, 0x18 for v2 */
#define ME_MV_STEP_WEIGHT_0 0x18
/* use 0 for v3, 0x18 for v2 */
#define ME_MV_STEP_WEIGHT_1 0x18
#define ME_MV_STEP_WEIGHT_2 0x0
#define ME_MV_STEP_WEIGHT_3 0x0

#define ME_SAD_ENOUGH_0_DATA 0x00
#define ME_SAD_ENOUGH_1_DATA 0x04
#define ME_SAD_ENOUGH_2_DATA 0x11
#define ADV_MV_8x8_ENOUGH_DATA 0x20

/* V4_COLOR_BLOCK_FIX */
#define V3_FORCE_SKIP_SAD_0 0x10
/* 4 Blocks */
#define V3_FORCE_SKIP_SAD_1 0x60
/* 16 Blocks + V3_SKIP_WEIGHT_2 */
#define V3_FORCE_SKIP_SAD_2 0x250
/* almost disable it -- use t_lac_coeff_2 output to F_ZERO is better */
#define V3_ME_F_ZERO_SAD (ME_WEIGHT_OFFSET + 0x10)

#define V3_IE_F_ZERO_SAD_I16 (I16MB_WEIGHT_OFFSET + 0x10)
#define V3_IE_F_ZERO_SAD_I4 (I4MB_WEIGHT_OFFSET + 0x20)

#define V3_SKIP_WEIGHT_0 0x10
/* 4 Blocks  8 separate search sad can be very low */
#define V3_SKIP_WEIGHT_1 0x8 /* (4 * ME_MV_STEP_WEIGHT_1 + 0x100) */
#define V3_SKIP_WEIGHT_2 0x3

#define V3_LEVEL_1_F_SKIP_MAX_SAD 0x0
#define V3_LEVEL_1_SKIP_MAX_SAD 0x6

#define I4_ipred_weight_most 0x18
#define I4_ipred_weight_else 0x28

#define C_ipred_weight_V 0x04
#define C_ipred_weight_H 0x08
#define C_ipred_weight_DC 0x0c

#define I16_ipred_weight_V 0x04
#define I16_ipred_weight_H 0x08
#define I16_ipred_weight_DC 0x0c

/* 0x00 same as disable */
#define v3_left_small_max_ie_sad 0x00
#define v3_left_small_max_me_sad 0x40

#define v5_use_small_diff_cnt 0
#define v5_simple_mb_inter_all_en 1
#define v5_simple_mb_inter_8x8_en 1
#define v5_simple_mb_inter_16_8_en 1
#define v5_simple_mb_inter_16x16_en 1
#define v5_simple_mb_intra_en 1
#define v5_simple_mb_C_en 0
#define v5_simple_mb_Y_en 1
#define v5_small_diff_Y 0x10
#define v5_small_diff_C 0x18
/* shift 8-bits, 2, 1, 0, -1, -2, -3, -4 */
#define v5_simple_dq_setting 0x43210fed
#define v5_simple_me_weight_setting 0

#ifdef H264_ENC_CBR
#define CBR_TABLE_SIZE 0x800
#define CBR_SHORT_SHIFT 12 /* same as disable */
#define CBR_LONG_MB_NUM 2
#define START_TABLE_ID 8
#define CBR_LONG_THRESH 4
#endif

void avc_configure_quantization_tables(
		u8 quant_tbl_i4[8], u8 quant_tbl_i16[8], u8 quant_tbl_me[8])
{
	WRITE_HREG(HCODEC_Q_QUANT_CONTROL,
			(0 << 23) |	   /* quant_table_addr */
			(1 << 22)); /* quant_table_addr_update */

	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_i4[0]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_i4[1]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_i4[2]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_i4[3]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_i4[4]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_i4[5]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_i4[6]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_i4[7]);

	WRITE_HREG(HCODEC_Q_QUANT_CONTROL,
			(8 << 23) |	   /* quant_table_addr */
			(1 << 22)); /* quant_table_addr_update */

	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_i16[0]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_i16[1]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_i16[2]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_i16[3]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_i16[4]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_i16[5]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_i16[6]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_i16[7]);

	WRITE_HREG(HCODEC_Q_QUANT_CONTROL,
			(16 << 23) |	   /* quant_table_addr */
			(1 << 22)); /* quant_table_addr_update */

	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_me[0]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_me[1]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_me[2]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_me[3]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_me[4]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_me[5]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_me[6]);
	WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
			quant_tbl_me[7]);
}

void avc_configure_output_buffer(u32 bitstreamStart, u32 bitstreamEnd)
{
	WRITE_HREG(HCODEC_VLC_VB_MEM_CTL,
			((1 << 31) | (0x3f << 24) |
			 (0x20 << 16) | (2 << 0)));
	WRITE_HREG(HCODEC_VLC_VB_START_PTR,
			bitstreamStart);
	WRITE_HREG(HCODEC_VLC_VB_WR_PTR,
			bitstreamStart);
	WRITE_HREG(HCODEC_VLC_VB_SW_RD_PTR,
			bitstreamStart);
	WRITE_HREG(HCODEC_VLC_VB_END_PTR,
			bitstreamEnd);
	WRITE_HREG(HCODEC_VLC_VB_CONTROL, 1);
	WRITE_HREG(HCODEC_VLC_VB_CONTROL,
			((0 << 14) | (7 << 3) |
			 (1 << 1) | (0 << 0)));
}

void avc_configure_input_buffer(u32 dct_buff_start_addr, u32 dct_buff_end_addr)
{
	WRITE_HREG(HCODEC_QDCT_MB_START_PTR, dct_buff_start_addr);
	WRITE_HREG(HCODEC_QDCT_MB_END_PTR, dct_buff_end_addr);
	WRITE_HREG(HCODEC_QDCT_MB_WR_PTR, dct_buff_start_addr);
	WRITE_HREG(HCODEC_QDCT_MB_RD_PTR, dct_buff_start_addr);
	WRITE_HREG(HCODEC_QDCT_MB_BUFF, 0);
}

void avc_configure_ref_buffer(int canvas)
{
	WRITE_HREG(HCODEC_ANC0_CANVAS_ADDR, canvas);
	WRITE_HREG(HCODEC_VLC_HCMD_CONFIG, 0);
}

void avc_configure_assist_buffer(u32 assit_buffer_offset)
{
	WRITE_HREG(MEM_OFFSET_REG, assit_buffer_offset);
}

void avc_configure_deblock_buffer(int canvas)
{
	WRITE_HREG(HCODEC_REC_CANVAS_ADDR, canvas);
	WRITE_HREG(HCODEC_DBKR_CANVAS_ADDR, canvas);
	WRITE_HREG(HCODEC_DBKW_CANVAS_ADDR, canvas);
}

void avc_configure_encoder_params(
		u32 idr_pic_id, u32 frame_number, u32 pic_order_cnt_lsb,
		u32 log2_max_frame_num, u32 log2_max_pic_order_cnt_lsb,
		u32 init_qppicture, bool idr)
{
	WRITE_HREG(HCODEC_VLC_TOTAL_BYTES, 0);
	WRITE_HREG(HCODEC_VLC_CONFIG, 0x07);
	WRITE_HREG(HCODEC_VLC_INT_CONTROL, 0);

	WRITE_HREG(HCODEC_ASSIST_AMR1_INT0, 0x15);
	WRITE_HREG(HCODEC_ASSIST_AMR1_INT1, 0x8);
	WRITE_HREG(HCODEC_ASSIST_AMR1_INT3, 0x14);

	WRITE_HREG(IDR_PIC_ID, idr_pic_id);
	WRITE_HREG(FRAME_NUMBER,
			(idr == true) ? 0 : frame_number);
	WRITE_HREG(PIC_ORDER_CNT_LSB,
			(idr == true) ? 0 : pic_order_cnt_lsb);

	WRITE_HREG(LOG2_MAX_PIC_ORDER_CNT_LSB, log2_max_pic_order_cnt_lsb);
	WRITE_HREG(LOG2_MAX_FRAME_NUM, log2_max_frame_num);
	WRITE_HREG(ANC0_BUFFER_ID, 0);
	WRITE_HREG(QPPICTURE, init_qppicture);
}

// TODO: Needs review avc_init_ie_me_parameter
void avc_configure_ie_me(u32 quant)
{
	// u32 ie_me_mb_type;

	u32 ie_cur_ref_sel = 0;
	u32 ie_pippeline_block = 12;
	u32 ie_me_mode = (ie_pippeline_block & IE_PIPPELINE_BLOCK_MASK) << IE_PIPPELINE_BLOCK_SHIFT;

	WRITE_HREG(IE_ME_MODE, ie_me_mode);
	WRITE_HREG(IE_REF_SEL, ie_cur_ref_sel);

#if 0 // TODO 
	if (quant == 0) { // Assuming 0 indicates IDR frame
		ie_me_mb_type = HENC_MB_Type_I4MB;
	} else {
		ie_me_mb_type = (HENC_SKIP_RUN_AUTO << 16) |
			(HENC_MB_Type_AUTO << 4) |
			(HENC_MB_Type_AUTO << 0);
	}
	WRITE_HREG(IE_ME_MB_TYPE, ie_me_mb_type);
#endif
}

void dos_reset(u32 dos_sw_reset1)
{
	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_SC2) { // && use_reset_control ?
		hcodec_hw_reset();
	} else {
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		WRITE_VREG(DOS_SW_RESET1, dos_sw_reset1);
		WRITE_VREG(DOS_SW_RESET1, 0);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
		READ_VREG(DOS_SW_RESET1);
	}
}

void avc_encoder_reset(void)
{
	dos_reset(
			(1 << 2) | (1 << 6) |
			(1 << 7) | (1 << 8) |
			(1 << 14) | (1 << 16) |
			(1 << 17));
}

void avc_encoder_start(void)
{
	dos_reset(
			(1 << 12) | (1 << 11));

	WRITE_HREG(HCODEC_MPSR, 0x0001);
}

void avc_encoder_stop(void)
{
	ulong timeout = jiffies + HZ;

	WRITE_HREG(HCODEC_MPSR, 0);
	WRITE_HREG(HCODEC_CPSR, 0);

	while (READ_HREG(HCODEC_IMEM_DMA_CTRL) & 0x8000)
	{
		if (time_after(jiffies, timeout))
			break;
	}

	dos_reset(
			(1 << 12) | (1 << 11) |
			(1 << 2) | (1 << 6) |
			(1 << 7) | (1 << 8) |
			(1 << 14) | (1 << 16) |
			(1 << 17));
}

void avc_configure_mfdin(
		u32 input, u8 iformat, u8 oformat,
		u32 picsize_x, u32 picsize_y,
		u8 r2y_en, u8 nr, u8 ifmt_extra)
{
	u8 dsample_en; /* Downsample Enable */
	u8 interp_en;  /* Interpolation Enable */
	u8 y_size;	   /* 0:16 Pixels for y direction pickup; 1:8 pixels */
	u8 r2y_mode;   /* RGB2YUV Mode, range(0~3) */
	/* mfdin_reg3_canv[25:24];
	 *  // bytes per pixel in x direction for index0, 0:half 1:1 2:2 3:3
	 */
	u8 canv_idx0_bppx;
	/* mfdin_reg3_canv[27:26];
	 *  // bytes per pixel in x direction for index1-2, 0:half 1:1 2:2 3:3
	 */
	u8 canv_idx1_bppx;
	/* mfdin_reg3_canv[29:28];
	 *  // bytes per pixel in y direction for index0, 0:half 1:1 2:2 3:3
	 */
	u8 canv_idx0_bppy;
	/* mfdin_reg3_canv[31:30];
	 *  // bytes per pixel in y direction for index1-2, 0:half 1:1 2:2 3:3
	 */
	u8 canv_idx1_bppy;
	u8 ifmt444, ifmt422, ifmt420, linear_bytes4p;
	u8 nr_enable;
	u8 cfg_y_snr_en;
	u8 cfg_y_tnr_en;
	u8 cfg_c_snr_en;
	u8 cfg_c_tnr_en;
	u32 linear_bytesperline;
	s32 reg_offset;
	bool linear_enable = false;
	bool format_err = false;

	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_TXL)
	{
		if ((iformat == 7) && (ifmt_extra > 2))
			format_err = true;
	}
	else if (iformat == 7)
		format_err = true;

	if (format_err)
	{
		pr_err(
				"mfdin format err, iformat:%d, ifmt_extra:%d\n",
				iformat, ifmt_extra);
		return;
	}
	if (iformat != 7)
		ifmt_extra = 0;

	ifmt444 = ((iformat == 1) || (iformat == 5) || (iformat == 8) ||
			(iformat == 9) || (iformat == 12))
		? 1
		: 0;
	if (iformat == 7 && ifmt_extra == 1)
		ifmt444 = 1;
	ifmt422 = ((iformat == 0) || (iformat == 10)) ? 1 : 0;
	if (iformat == 7 && ifmt_extra != 1)
		ifmt422 = 1;
	ifmt420 = ((iformat == 2) || (iformat == 3) || (iformat == 4) ||
			(iformat == 11))
		? 1
		: 0;
	dsample_en = ((ifmt444 && (oformat != 2)) ||
			(ifmt422 && (oformat == 0)))
		? 1
		: 0;
	interp_en = ((ifmt422 && (oformat == 2)) ||
			(ifmt420 && (oformat != 0)))
		? 1
		: 0;
	y_size = (oformat != 0) ? 1 : 0;
	if (iformat == 12)
		y_size = 0;
	r2y_mode = (r2y_en == 1) ? 1 : 0; /* Fixed to 1 (TODO) */
	canv_idx0_bppx = (iformat == 1) ? 3 : (iformat == 0) ? 2
		: 1;
	canv_idx1_bppx = (iformat == 4) ? 0 : 1;
	canv_idx0_bppy = 1;
	canv_idx1_bppy = (iformat == 5) ? 1 : 0;

	if ((iformat == 8) || (iformat == 9) || (iformat == 12))
		linear_bytes4p = 3;
	else if (iformat == 10)
		linear_bytes4p = 2;
	else if (iformat == 11)
		linear_bytes4p = 1;
	else
		linear_bytes4p = 0;
	if (iformat == 12)
		linear_bytesperline = picsize_x * 4;
	else
		linear_bytesperline = picsize_x * linear_bytes4p;

	if (iformat < 8)
		linear_enable = false;
	else
		linear_enable = true;

	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_GXBB)
	{
		reg_offset = -8;
		/* nr_mode: 0:Disabled 1:SNR Only 2:TNR Only 3:3DNR */
		nr_enable = (nr) ? 1 : 0;
		cfg_y_snr_en = ((nr == 1) || (nr == 3)) ? 1 : 0;
		cfg_y_tnr_en = ((nr == 2) || (nr == 3)) ? 1 : 0;
		cfg_c_snr_en = cfg_y_snr_en;
		/* cfg_c_tnr_en = cfg_y_tnr_en; */
		cfg_c_tnr_en = 0;

		/* NR For Y */
		WRITE_HREG((HCODEC_MFDIN_REG0D + reg_offset),
				((cfg_y_snr_en << 0) |
				 (y_snr_err_norm << 1) |
				 (y_snr_gau_bld_core << 2) |
				 (((y_snr_gau_bld_ofst) & 0xff) << 6) |
				 (y_snr_gau_bld_rate << 14) |
				 (y_snr_gau_alp0_min << 20) |
				 (y_snr_gau_alp0_max << 26)));
		WRITE_HREG((HCODEC_MFDIN_REG0E + reg_offset),
				((cfg_y_tnr_en << 0) |
				 (y_tnr_mc_en << 1) |
				 (y_tnr_txt_mode << 2) |
				 (y_tnr_mot_sad_margin << 3) |
				 (y_tnr_alpha_min << 7) |
				 (y_tnr_alpha_max << 13) |
				 (y_tnr_deghost_os << 19)));
		WRITE_HREG((HCODEC_MFDIN_REG0F + reg_offset),
				((y_tnr_mot_cortxt_rate << 0) |
				 (y_tnr_mot_distxt_ofst << 8) |
				 (y_tnr_mot_distxt_rate << 4) |
				 (y_tnr_mot_dismot_ofst << 16) |
				 (y_tnr_mot_frcsad_lock << 24)));
		WRITE_HREG((HCODEC_MFDIN_REG10 + reg_offset),
				((y_tnr_mot2alp_frc_gain << 0) |
				 (y_tnr_mot2alp_nrm_gain << 8) |
				 (y_tnr_mot2alp_dis_gain << 16) |
				 (y_tnr_mot2alp_dis_ofst << 24)));
		WRITE_HREG((HCODEC_MFDIN_REG11 + reg_offset),
				((y_bld_beta2alp_rate << 0) |
				 (y_bld_beta_min << 8) |
				 (y_bld_beta_max << 14)));

		/* NR For C */
		WRITE_HREG((HCODEC_MFDIN_REG12 + reg_offset),
				((cfg_y_snr_en << 0) |
				 (c_snr_err_norm << 1) |
				 (c_snr_gau_bld_core << 2) |
				 (((c_snr_gau_bld_ofst) & 0xff) << 6) |
				 (c_snr_gau_bld_rate << 14) |
				 (c_snr_gau_alp0_min << 20) |
				 (c_snr_gau_alp0_max << 26)));

		WRITE_HREG((HCODEC_MFDIN_REG13 + reg_offset),
				((cfg_c_tnr_en << 0) |
				 (c_tnr_mc_en << 1) |
				 (c_tnr_txt_mode << 2) |
				 (c_tnr_mot_sad_margin << 3) |
				 (c_tnr_alpha_min << 7) |
				 (c_tnr_alpha_max << 13) |
				 (c_tnr_deghost_os << 19)));
		WRITE_HREG((HCODEC_MFDIN_REG14 + reg_offset),
				((c_tnr_mot_cortxt_rate << 0) |
				 (c_tnr_mot_distxt_ofst << 8) |
				 (c_tnr_mot_distxt_rate << 4) |
				 (c_tnr_mot_dismot_ofst << 16) |
				 (c_tnr_mot_frcsad_lock << 24)));
		WRITE_HREG((HCODEC_MFDIN_REG15 + reg_offset),
				((c_tnr_mot2alp_frc_gain << 0) |
				 (c_tnr_mot2alp_nrm_gain << 8) |
				 (c_tnr_mot2alp_dis_gain << 16) |
				 (c_tnr_mot2alp_dis_ofst << 24)));

		WRITE_HREG((HCODEC_MFDIN_REG16 + reg_offset),
				((c_bld_beta2alp_rate << 0) |
				 (c_bld_beta_min << 8) |
				 (c_bld_beta_max << 14)));

		WRITE_HREG((HCODEC_MFDIN_REG1_CTRL + reg_offset),
				(iformat << 0) | (oformat << 4) |
				(dsample_en << 6) | (y_size << 8) |
				(interp_en << 9) | (r2y_en << 12) |
				(r2y_mode << 13) | (ifmt_extra << 16) |
				(nr_enable << 19));

		if (get_cpu_type() >= MESON_CPU_MAJOR_ID_SC2)
		{
			WRITE_HREG((HCODEC_MFDIN_REG8_DMBL + reg_offset),
					(picsize_x << 16) | (picsize_y << 0));
		}
		else
		{
			WRITE_HREG((HCODEC_MFDIN_REG8_DMBL + reg_offset),
					(picsize_x << 14) | (picsize_y << 0));
		}
	}
	else
	{
		reg_offset = 0;
		WRITE_HREG((HCODEC_MFDIN_REG1_CTRL + reg_offset),
				(iformat << 0) | (oformat << 4) |
				(dsample_en << 6) | (y_size << 8) |
				(interp_en << 9) | (r2y_en << 12) |
				(r2y_mode << 13));

		WRITE_HREG((HCODEC_MFDIN_REG8_DMBL + reg_offset),
				(picsize_x << 12) | (picsize_y << 0));
	}

	if (linear_enable == false)
	{
		WRITE_HREG((HCODEC_MFDIN_REG3_CANV + reg_offset),
				(input & 0xffffff) |
				(canv_idx1_bppy << 30) |
				(canv_idx0_bppy << 28) |
				(canv_idx1_bppx << 26) |
				(canv_idx0_bppx << 24));
		WRITE_HREG((HCODEC_MFDIN_REG4_LNR0 + reg_offset),
				(0 << 16) | (0 << 0));
		WRITE_HREG((HCODEC_MFDIN_REG5_LNR1 + reg_offset), 0);
	}
	else
	{
		WRITE_HREG((HCODEC_MFDIN_REG3_CANV + reg_offset),
				(canv_idx1_bppy << 30) |
				(canv_idx0_bppy << 28) |
				(canv_idx1_bppx << 26) |
				(canv_idx0_bppx << 24));
		WRITE_HREG((HCODEC_MFDIN_REG4_LNR0 + reg_offset),
				(linear_bytes4p << 16) | (linear_bytesperline << 0));
		WRITE_HREG((HCODEC_MFDIN_REG5_LNR1 + reg_offset), input);
	}

	if (iformat == 12)
		WRITE_HREG((HCODEC_MFDIN_REG9_ENDN + reg_offset),
				(2 << 0) | (1 << 3) | (0 << 6) |
				(3 << 9) | (6 << 12) | (5 << 15) |
				(4 << 18) | (7 << 21));
	else
		WRITE_HREG((HCODEC_MFDIN_REG9_ENDN + reg_offset),
				(7 << 0) | (6 << 3) | (5 << 6) |
				(4 << 9) | (3 << 12) | (2 << 15) |
				(1 << 18) | (0 << 21));
}

void avc_configure_encoder_control(u32 pic_width)
{
	u32 pic_width_in_mb = (pic_width + 15) / 16;

	WRITE_HREG(HCODEC_HDEC_MC_OMEM_AUTO,
			(1 << 31) |						   /* use_omem_mb_xy */
			((pic_width_in_mb - 1) << 16)); /* omem_max_mb_x */

	WRITE_HREG(HCODEC_VLC_ADV_CONFIG,
			(0 << 10) |	  /* early_mix_mc_hcmd */
			(1 << 9) | /* update_top_left_mix */
			(1 << 8) | /* p_top_left_mix */
			(0 << 7) | /* mv_cal_mixed_type */
			(0 << 6) | /* mc_hcmd_mixed_type */
			(1 << 5) | /* use_separate_int_control */
			(1 << 4) | /* hcmd_intra_use_q_info */
			(1 << 3) | /* hcmd_left_use_prev_info */
			(1 << 2) | /* hcmd_use_q_info */
			(1 << 1) | /* use_q_delta_quant */
			(0 << 0)); /* detect_I16_from_I4 use qdct detected mb_type */

	WRITE_HREG(HCODEC_QDCT_ADV_CONFIG,
			(1 << 29) |	   /* mb_info_latch_no_I16_pred_mode */
			(1 << 28) | /* ie_dma_mbxy_use_i_pred */
			(1 << 27) | /* ie_dma_read_write_use_ip_idx */
			(1 << 26) | /* ie_start_use_top_dma_count */
			(1 << 25) | /* i_pred_top_dma_rd_mbbot */
			(1 << 24) | /* i_pred_top_dma_wr_disable */
			(0 << 23) | /* i_pred_mix */
			(1 << 22) | /* me_ab_rd_when_intra_in_p */
			(1 << 21) | /* force_mb_skip_run_when_intra */
			(0 << 20) | /* mc_out_mixed_type */
			(1 << 19) | /* ie_start_when_quant_not_full */
			(1 << 18) | /* mb_info_state_mix */
			(0 << 17) | /* mb_type_use_mix_result */
			(0 << 16) | /* me_cb_ie_read_enable */
			(0 << 15) | /* ie_cur_data_from_me */
			(1 << 14) | /* rem_per_use_table */
			(0 << 13) | /* q_latch_int_enable */
			(1 << 12) | /* q_use_table */
			(0 << 11) | /* q_start_wait */
			(1 << 10) | /* LUMA_16_LEFT_use_cur */
			(1 << 9) |  /* DC_16_LEFT_SUM_use_cur */
			(1 << 8) |  /* c_ref_ie_sel_cur */
			(0 << 7) |  /* c_ipred_perfect_mode */
			(1 << 6) |  /* ref_ie_ul_sel */
			(1 << 5) |  /* mb_type_use_ie_result */
			(1 << 4) |  /* detect_I16_from_I4 */
			(1 << 3) |  /* ie_not_wait_ref_busy */
			(1 << 2) |  /* ie_I16_enable */
			(3 << 0));  /* ie_done_sel  // fastest when waiting */
}

void avc_configure_ie_weight(u32 i16_weight, u32 i4_weight, u32 me_weight)
{
	WRITE_HREG(HCODEC_IE_WEIGHT,
			(i16_weight << 16) |
			(i4_weight << 0));
	WRITE_HREG(HCODEC_ME_WEIGHT,
			(me_weight << 0));
	WRITE_HREG(HCODEC_SAD_CONTROL_0,
			/* ie_sad_offset_I16 */
			(i16_weight << 16) |
			/* ie_sad_offset_I4 */
			(i4_weight << 0));
	WRITE_HREG(HCODEC_SAD_CONTROL_1,
			/* ie_sad_shift_I16 */
			(IE_SAD_SHIFT_I16 << 24) |
			/* ie_sad_shift_I4 */
			(IE_SAD_SHIFT_I4 << 20) |
			/* me_sad_shift_INTER */
			(ME_SAD_SHIFT_INTER << 16) |
			/* me_sad_offset_INTER */
			(me_weight << 0));
}

void avc_configure_ie_clear_sad_shift(void) {
		u32 data32 = READ_HREG(HCODEC_SAD_CONTROL_1);
		data32 = data32 & 0xffff; /* remove sad shift */
		WRITE_HREG(HCODEC_SAD_CONTROL_1, data32);
}

void avc_configure_mv_merge(u32 me_mv_merge_ctl)
{
	WRITE_HREG(HCODEC_ME_MV_MERGE_CTL, me_mv_merge_ctl);
}

void avc_configure_me_parameters(
		u32 me_step0_close_mv, u32 me_f_skip_sad,
		u32 me_f_skip_weight, u32 me_sad_range_inc,
		u32 me_sad_enough_01, u32 me_sad_enough_23,
		u32 me_mv_weight_01, u32 me_mv_weight_23
){
	WRITE_HREG(HCODEC_ME_STEP0_CLOSE_MV, me_step0_close_mv);
	WRITE_HREG(HCODEC_ME_F_SKIP_SAD, me_f_skip_sad);
	WRITE_HREG(HCODEC_ME_F_SKIP_WEIGHT, me_f_skip_weight);
	WRITE_HREG(HCODEC_ME_SAD_RANGE_INC, me_sad_range_inc);
	WRITE_HREG(HCODEC_ME_SAD_ENOUGH_01, me_sad_enough_01);
	WRITE_HREG(HCODEC_ME_MV_WEIGHT_01, me_mv_weight_01);
	WRITE_HREG(HCODEC_ME_MV_WEIGHT_23, me_mv_weight_23);
	WRITE_HREG(HCODEC_ME_SAD_ENOUGH_23, me_sad_enough_23);
}

void avc_configure_skip_control_GXL(void)
{
	WRITE_HREG(HCODEC_V3_SKIP_CONTROL,
			(1 << 31) |	   /* v3_skip_enable */
			(0 << 30) | /* v3_step_1_weight_enable */
			(1 << 28) | /* v3_mv_sad_weight_enable */
			(1 << 27) | /* v3_ipred_type_enable */
			(V3_FORCE_SKIP_SAD_1 << 12) |
			(V3_FORCE_SKIP_SAD_0 << 0));
	WRITE_HREG(HCODEC_V3_SKIP_WEIGHT,
			(V3_SKIP_WEIGHT_1 << 16) |
			(V3_SKIP_WEIGHT_0 << 0));
	WRITE_HREG(HCODEC_V3_L1_SKIP_MAX_SAD,
			(V3_LEVEL_1_F_SKIP_MAX_SAD << 16) |
			(V3_LEVEL_1_SKIP_MAX_SAD << 0));
	WRITE_HREG(HCODEC_V3_L2_SKIP_WEIGHT,
			(V3_FORCE_SKIP_SAD_2 << 16) |
			(V3_SKIP_WEIGHT_2 << 0));
}

void avc_configure_skip_control_GXTVBB(void)
{
	WRITE_HREG(HCODEC_V3_SKIP_CONTROL,
		(1 << 31) | /* v3_skip_enable */
		(0 << 30) | /* v3_step_1_weight_enable */
		(1 << 28) | /* v3_mv_sad_weight_enable */
		(1 << 27) | /* v3_ipred_type_enable */
		(0 << 12) | /* V3_FORCE_SKIP_SAD_1 */
		(0 << 0)); /* V3_FORCE_SKIP_SAD_0 */
	WRITE_HREG(HCODEC_V3_SKIP_WEIGHT,
		(V3_SKIP_WEIGHT_1 << 16) |
		(V3_SKIP_WEIGHT_0 << 0));
	WRITE_HREG(HCODEC_V3_L1_SKIP_MAX_SAD,
		(V3_LEVEL_1_F_SKIP_MAX_SAD << 16) |
		(V3_LEVEL_1_SKIP_MAX_SAD << 0));
	WRITE_HREG(HCODEC_V3_L2_SKIP_WEIGHT,
		(0 << 16) | /* V3_FORCE_SKIP_SAD_2 */
		(V3_SKIP_WEIGHT_2 << 0));
}

void avc_configure_mv_sad_table(u32 v3_mv_sad[64])
{
	int i;
	for (i = 0; i < 64; i++)
		WRITE_HREG(HCODEC_V3_MV_SAD_TABLE, v3_mv_sad[i]);
}

void avc_configure_ipred_weight(void)
{
	WRITE_HREG(HCODEC_V3_IPRED_TYPE_WEIGHT_0,
			(C_ipred_weight_H << 24) |
			(C_ipred_weight_V << 16) |
			(I4_ipred_weight_else << 8) |
			(I4_ipred_weight_most << 0));
	WRITE_HREG(HCODEC_V3_IPRED_TYPE_WEIGHT_1,
			(I16_ipred_weight_DC << 24) |
			(I16_ipred_weight_H << 16) |
			(I16_ipred_weight_V << 8) |
			(C_ipred_weight_DC << 0));
}

void avc_configure_left_small_max_sad(u32 max_me_sad, u32 max_ie_sad)
{
	WRITE_HREG(HCODEC_V3_LEFT_SMALL_MAX_SAD,
			(max_me_sad << 16) |
			(max_ie_sad << 0));
}

void avc_configure_svc_pic_type(bool enc_slc_ref)
{
	if (enc_slc_ref)
	{
		WRITE_HREG(H264_ENC_SVC_PIC_TYPE, ENC_SLC_REF);
	}
	else
	{
		WRITE_HREG(H264_ENC_SVC_PIC_TYPE, ENC_SLC_NON_REF);
	}
}

void avc_configure_fixed_slice(u32 fixed_slice_cfg, u32 rows_per_slice, u32 pic_height)
{
	if (fixed_slice_cfg)
	{
		WRITE_HREG(FIXED_SLICE_CFG, fixed_slice_cfg);
	}
	else if (rows_per_slice && rows_per_slice != (pic_height + 15) >> 4)
	{
		u32 mb_per_slice = (pic_height + 15) >> 4;
		mb_per_slice = mb_per_slice * rows_per_slice;
		WRITE_HREG(FIXED_SLICE_CFG, mb_per_slice);
	}
	else
	{
		WRITE_HREG(FIXED_SLICE_CFG, 0);
	}
}

#if 0 // TODO: no orignal usage
void avc_configure_encoding_mode(struct encode_wq_s *wq, bool is_idr) {
	u32 data32;

	data32 = READ_HREG(HCODEC_VLC_ADV_CONFIG);
	if (!is_idr) {
		// Configure for P frame
		data32 |= (1 << 10) | (1 << 7) | (1 << 6);
	} else {
		// Configure for I frame
		data32 &= ~((1 << 10) | (1 << 7) | (1 << 6));
	}
	WRITE_HREG(HCODEC_VLC_ADV_CONFIG, data32);

	data32 = READ_HREG(HCODEC_QDCT_ADV_CONFIG);
	if (!is_idr) {
		// Configure for P frame
		data32 |= (1 << 23) | (1 << 20) | (1 << 19) | (1 << 16);
	} else {
		// Configure for I frame
		data32 &= ~((1 << 23) | (1 << 20) | (1 << 19) | (1 << 16));
	}
	WRITE_HREG(HCODEC_QDCT_ADV_CONFIG, data32);

	if (is_idr) {
		WRITE_HREG(HCODEC_IE_ME_MODE, ie_me_mode);
		WRITE_HREG(HCODEC_IE_ME_MB_TYPE, HENC_MB_Type_I4MB);
	} else {
		WRITE_HREG(HCODEC_IE_ME_MODE, ie_me_mode |
				(HENC_SKIP_RUN_AUTO << 16) |
				(HENC_MB_Type_AUTO << 18) |
				(HENC_COEFF_TOKEN_AUTO << 20));
		WRITE_HREG(HCODEC_IE_ME_MB_TYPE,
				(HENC_SKIP_RUN_AUTO << 16) |
				(HENC_MB_Type_AUTO << 4) |
				(HENC_MB_Type_AUTO << 0));
	}
}
#endif

void avc_configure_cbr_settings(
		u32 ddr_start_addr, u32 ddr_addr_length,
		u16 block_w, u16 block_h, u16 long_th,
		u8 start_tbl_id, u8 short_shift, u8 long_mb_num
		) {
	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_GXTVBB)
	{
		WRITE_HREG(H264_ENC_CBR_TABLE_ADDR, ddr_start_addr);
		WRITE_HREG(H264_ENC_CBR_MB_SIZE_ADDR, ddr_start_addr + ddr_addr_length);
		WRITE_HREG(H264_ENC_CBR_CTL,
				(start_tbl_id << 28) |
				(short_shift << 24) |
				(long_mb_num << 16) |
				(long_th << 0));
		WRITE_HREG(H264_ENC_CBR_REGION_SIZE,
				(block_w << 16) |
				(block_h << 0));
	}
}

void avc_configure_quant_params(u32 i_pic_qp, u32 p_pic_qp)
{
	u32 i_pic_qp_c, p_pic_qp_c;

	/* Calculate I and P frame QP values */

	/* synopsys parallel_case full_case */
	switch (i_pic_qp)
	{
		case 0:
			i_pic_qp_c = 0;
			break;
		case 1:
			i_pic_qp_c = 1;
			break;
		case 2:
			i_pic_qp_c = 2;
			break;
		case 3:
			i_pic_qp_c = 3;
			break;
		case 4:
			i_pic_qp_c = 4;
			break;
		case 5:
			i_pic_qp_c = 5;
			break;
		case 6:
			i_pic_qp_c = 6;
			break;
		case 7:
			i_pic_qp_c = 7;
			break;
		case 8:
			i_pic_qp_c = 8;
			break;
		case 9:
			i_pic_qp_c = 9;
			break;
		case 10:
			i_pic_qp_c = 10;
			break;
		case 11:
			i_pic_qp_c = 11;
			break;
		case 12:
			i_pic_qp_c = 12;
			break;
		case 13:
			i_pic_qp_c = 13;
			break;
		case 14:
			i_pic_qp_c = 14;
			break;
		case 15:
			i_pic_qp_c = 15;
			break;
		case 16:
			i_pic_qp_c = 16;
			break;
		case 17:
			i_pic_qp_c = 17;
			break;
		case 18:
			i_pic_qp_c = 18;
			break;
		case 19:
			i_pic_qp_c = 19;
			break;
		case 20:
			i_pic_qp_c = 20;
			break;
		case 21:
			i_pic_qp_c = 21;
			break;
		case 22:
			i_pic_qp_c = 22;
			break;
		case 23:
			i_pic_qp_c = 23;
			break;
		case 24:
			i_pic_qp_c = 24;
			break;
		case 25:
			i_pic_qp_c = 25;
			break;
		case 26:
			i_pic_qp_c = 26;
			break;
		case 27:
			i_pic_qp_c = 27;
			break;
		case 28:
			i_pic_qp_c = 28;
			break;
		case 29:
			i_pic_qp_c = 29;
			break;
		case 30:
			i_pic_qp_c = 29;
			break;
		case 31:
			i_pic_qp_c = 30;
			break;
		case 32:
			i_pic_qp_c = 31;
			break;
		case 33:
			i_pic_qp_c = 32;
			break;
		case 34:
			i_pic_qp_c = 32;
			break;
		case 35:
			i_pic_qp_c = 33;
			break;
		case 36:
			i_pic_qp_c = 34;
			break;
		case 37:
			i_pic_qp_c = 34;
			break;
		case 38:
			i_pic_qp_c = 35;
			break;
		case 39:
			i_pic_qp_c = 35;
			break;
		case 40:
			i_pic_qp_c = 36;
			break;
		case 41:
			i_pic_qp_c = 36;
			break;
		case 42:
			i_pic_qp_c = 37;
			break;
		case 43:
			i_pic_qp_c = 37;
			break;
		case 44:
			i_pic_qp_c = 37;
			break;
		case 45:
			i_pic_qp_c = 38;
			break;
		case 46:
			i_pic_qp_c = 38;
			break;
		case 47:
			i_pic_qp_c = 38;
			break;
		case 48:
			i_pic_qp_c = 39;
			break;
		case 49:
			i_pic_qp_c = 39;
			break;
		case 50:
			i_pic_qp_c = 39;
			break;
		default:
			i_pic_qp_c = 39;
			break;
	}

	/* synopsys parallel_case full_case */
	switch (p_pic_qp)
	{
		case 0:
			p_pic_qp_c = 0;
			break;
		case 1:
			p_pic_qp_c = 1;
			break;
		case 2:
			p_pic_qp_c = 2;
			break;
		case 3:
			p_pic_qp_c = 3;
			break;
		case 4:
			p_pic_qp_c = 4;
			break;
		case 5:
			p_pic_qp_c = 5;
			break;
		case 6:
			p_pic_qp_c = 6;
			break;
		case 7:
			p_pic_qp_c = 7;
			break;
		case 8:
			p_pic_qp_c = 8;
			break;
		case 9:
			p_pic_qp_c = 9;
			break;
		case 10:
			p_pic_qp_c = 10;
			break;
		case 11:
			p_pic_qp_c = 11;
			break;
		case 12:
			p_pic_qp_c = 12;
			break;
		case 13:
			p_pic_qp_c = 13;
			break;
		case 14:
			p_pic_qp_c = 14;
			break;
		case 15:
			p_pic_qp_c = 15;
			break;
		case 16:
			p_pic_qp_c = 16;
			break;
		case 17:
			p_pic_qp_c = 17;
			break;
		case 18:
			p_pic_qp_c = 18;
			break;
		case 19:
			p_pic_qp_c = 19;
			break;
		case 20:
			p_pic_qp_c = 20;
			break;
		case 21:
			p_pic_qp_c = 21;
			break;
		case 22:
			p_pic_qp_c = 22;
			break;
		case 23:
			p_pic_qp_c = 23;
			break;
		case 24:
			p_pic_qp_c = 24;
			break;
		case 25:
			p_pic_qp_c = 25;
			break;
		case 26:
			p_pic_qp_c = 26;
			break;
		case 27:
			p_pic_qp_c = 27;
			break;
		case 28:
			p_pic_qp_c = 28;
			break;
		case 29:
			p_pic_qp_c = 29;
			break;
		case 30:
			p_pic_qp_c = 29;
			break;
		case 31:
			p_pic_qp_c = 30;
			break;
		case 32:
			p_pic_qp_c = 31;
			break;
		case 33:
			p_pic_qp_c = 32;
			break;
		case 34:
			p_pic_qp_c = 32;
			break;
		case 35:
			p_pic_qp_c = 33;
			break;
		case 36:
			p_pic_qp_c = 34;
			break;
		case 37:
			p_pic_qp_c = 34;
			break;
		case 38:
			p_pic_qp_c = 35;
			break;
		case 39:
			p_pic_qp_c = 35;
			break;
		case 40:
			p_pic_qp_c = 36;
			break;
		case 41:
			p_pic_qp_c = 36;
			break;
		case 42:
			p_pic_qp_c = 37;
			break;
		case 43:
			p_pic_qp_c = 37;
			break;
		case 44:
			p_pic_qp_c = 37;
			break;
		case 45:
			p_pic_qp_c = 38;
			break;
		case 46:
			p_pic_qp_c = 38;
			break;
		case 47:
			p_pic_qp_c = 38;
			break;
		case 48:
			p_pic_qp_c = 39;
			break;
		case 49:
			p_pic_qp_c = 39;
			break;
		case 50:
			p_pic_qp_c = 39;
			break;
		default:
			p_pic_qp_c = 39;
			break;
	}

	/* Configure quantization control registers */
	WRITE_HREG(HCODEC_QDCT_Q_QUANT_I,
			(i_pic_qp_c << 22) |
			(i_pic_qp << 16) |
			((i_pic_qp_c % 6) << 12) |
			((i_pic_qp_c / 6) << 8) |
			((i_pic_qp % 6) << 4) |
			((i_pic_qp / 6) << 0));

	WRITE_HREG(HCODEC_QDCT_Q_QUANT_P,
			(p_pic_qp_c << 22) |
			(p_pic_qp << 16) |
			((p_pic_qp_c % 6) << 12) |
			((p_pic_qp_c / 6) << 8) |
			((p_pic_qp % 6) << 4) |
			((p_pic_qp / 6) << 0));
}

void avc_configure_quant_control(u32 i_pic_qp)
{
	/* Configure VLC quantization control */
	WRITE_HREG(HCODEC_QDCT_VLC_QUANT_CTL_0,
			(0 << 19) |			  /* vlc_delta_quant_1 */
			(i_pic_qp << 13) | /* vlc_quant_1 */
			(0 << 6) |		  /* vlc_delta_quant_0 */
			(i_pic_qp << 0));  /* vlc_quant_0 */
	WRITE_HREG(HCODEC_QDCT_VLC_QUANT_CTL_1,
			(14 << 6) |	   /* vlc_max_delta_q_neg */
			(13 << 0)); /* vlc_max_delta_q_pos */
}

void avc_configure_ignore_config(bool ignore)
{
	if (ignore)
	{
		WRITE_HREG(HCODEC_IGNORE_CONFIG,
				(1 << 31) |	   /* ignore_lac_coeff_en */
				(1 << 26) | /* ignore_lac_coeff_else (<1) */
				(1 << 21) | /* ignore_lac_coeff_2 (<1) */
				(2 << 16) | /* ignore_lac_coeff_1 (<2) */
				(1 << 15) | /* ignore_cac_coeff_en */
				(1 << 10) | /* ignore_cac_coeff_else (<1) */
				(1 << 5) |  /* ignore_cac_coeff_2 (<1) */
				(3 << 0));  /* ignore_cac_coeff_1 (<2) */

		if (get_cpu_type() >= MESON_CPU_MAJOR_ID_GXTVBB)
			WRITE_HREG(HCODEC_IGNORE_CONFIG_2,
					(1 << 31) |	   /* ignore_t_lac_coeff_en */
					(1 << 26) | /* ignore_t_lac_coeff_else (<1) */
					(2 << 21) | /* ignore_t_lac_coeff_2 (<2) */
					(6 << 16) | /* ignore_t_lac_coeff_1 (<6) */
					(1 << 15) | /* ignore_cdc_coeff_en */
					(0 << 14) | /* ignore_t_lac_coeff_else_le_3 */
					(1 << 13) | /* ignore_t_lac_coeff_else_le_4 */
					(1 << 12) | /* ignore_cdc_only_when_empty_cac_inter */
					(1 << 11) | /* ignore_cdc_only_when_one_empty_inter */
					/* ignore_cdc_range_max_inter 0-0, 1-1, 2-2, 3-3 */
					(2 << 9) |
					/* ignore_cdc_abs_max_inter 0-1, 1-2, 2-3, 3-4 */
					(0 << 7) |
					/* ignore_cdc_only_when_empty_cac_intra */
					(1 << 5) |
					/* ignore_cdc_only_when_one_empty_intra */
					(1 << 4) |
					/* ignore_cdc_range_max_intra 0-0, 1-1, 2-2, 3-3 */
					(1 << 2) |
					/* ignore_cdc_abs_max_intra 0-1, 1-2, 2-3, 3-4 */
					(0 << 0));
		else
			WRITE_HREG(HCODEC_IGNORE_CONFIG_2,
					(1 << 31) |	   /* ignore_t_lac_coeff_en */
					(1 << 26) | /* ignore_t_lac_coeff_else (<1) */
					(1 << 21) | /* ignore_t_lac_coeff_2 (<1) */
					(5 << 16) | /* ignore_t_lac_coeff_1 (<5) */
					(0 << 0));
	}
	else
	{
		WRITE_HREG(HCODEC_IGNORE_CONFIG, 0);
		WRITE_HREG(HCODEC_IGNORE_CONFIG_2, 0);
	}
}

void avc_configure_sad_control(void)
{
	WRITE_HREG(HCODEC_SAD_CONTROL,
			(0 << 3) |	  /* ie_result_buff_enable */
			(1 << 2) | /* ie_result_buff_soft_reset */
			(0 << 1) | /* sad_enable */
			(1 << 0)); /* sad soft reset */
	WRITE_HREG(HCODEC_IE_RESULT_BUFFER, 0);

	WRITE_HREG(HCODEC_SAD_CONTROL,
			(1 << 3) |	  /* ie_result_buff_enable */
			(0 << 2) | /* ie_result_buff_soft_reset */
			(1 << 1) | /* sad_enable */
			(0 << 0)); /* sad soft reset */
}

void avc_configure_ie_control(void)
{
	WRITE_HREG(HCODEC_IE_CONTROL,
			(1 << 30) |	  /* active_ul_block */
			(0 << 1) | /* ie_enable */
			(1 << 0)); /* ie soft reset */

	WRITE_HREG(HCODEC_IE_CONTROL,
			(1 << 30) |	  /* active_ul_block */
			(0 << 1) | /* ie_enable */
			(0 << 0)); /* ie soft reset */
}

void avc_configure_me_skip_line(void)
{
	WRITE_HREG(HCODEC_ME_SKIP_LINE,
			(8 << 24) |	   /* step_3_skip_line */
			(8 << 18) | /* step_2_skip_line */
			(2 << 12) | /* step_1_skip_line */
			(0 << 6) |  /* step_0_skip_line */
			(0 << 0));
}

void avc_configure_v5_simple_mb(
		u32 qp_mode, u32 dq_setting, u32 me_weight_setting
		) {
	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_TXL)
	{
		WRITE_HREG(HCODEC_V5_SIMPLE_MB_CTL, 0);
		WRITE_HREG(HCODEC_V5_SIMPLE_MB_CTL,
				(v5_use_small_diff_cnt << 7) |
				(v5_simple_mb_inter_all_en << 6) |
				(v5_simple_mb_inter_8x8_en << 5) |
				(v5_simple_mb_inter_16_8_en << 4) |
				(v5_simple_mb_inter_16x16_en << 3) |
				(v5_simple_mb_intra_en << 2) |
				(v5_simple_mb_C_en << 1) |
				(v5_simple_mb_Y_en << 0));
		WRITE_HREG(HCODEC_V5_MB_DIFF_SUM, 0);
		WRITE_HREG(HCODEC_V5_SMALL_DIFF_CNT,
				(v5_small_diff_C << 16) |
				(v5_small_diff_Y << 0));
		if (qp_mode == 1)
		{
			WRITE_HREG(HCODEC_V5_SIMPLE_MB_DQUANT, 0);
		}
		else
		{
			WRITE_HREG(HCODEC_V5_SIMPLE_MB_DQUANT, dq_setting);
		}
		WRITE_HREG(HCODEC_V5_SIMPLE_MB_ME_WEIGHT, me_weight_setting);
		WRITE_HREG(HCODEC_QDCT_CONFIG, 1 << 0);
	}
}

#if 0
void avc_configure_canvas(struct encode_wq_s *wq) {
	u32 canvas_width, canvas_height;
	u32 start_addr = wq->mem.buf_start;

	canvas_width = ((wq->pic.encoder_width + 31) >> 5) << 5;
	canvas_height = ((wq->pic.encoder_height + 15) >> 4) << 4;

	canvas_config(ENC_CANVAS_OFFSET,
			start_addr + wq->mem.bufspec.dec0_y.buf_start,
			canvas_width, canvas_height,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
	canvas_config(1 + ENC_CANVAS_OFFSET,
			start_addr + wq->mem.bufspec.dec0_uv.buf_start,
			canvas_width, canvas_height / 2,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
	canvas_config(2 + ENC_CANVAS_OFFSET,
			start_addr + wq->mem.bufspec.dec0_uv.buf_start,
			canvas_width, canvas_height / 2,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);

	canvas_config(3 + ENC_CANVAS_OFFSET,
			start_addr + wq->mem.bufspec.dec1_y.buf_start,
			canvas_width, canvas_height,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
	canvas_config(4 + ENC_CANVAS_OFFSET,
			start_addr + wq->mem.bufspec.dec1_uv.buf_start,
			canvas_width, canvas_height / 2,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
	canvas_config(5 + ENC_CANVAS_OFFSET,
			start_addr + wq->mem.bufspec.dec1_uv.buf_start,
			canvas_width, canvas_height / 2,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
}
#endif

#if 0
void avc_convert_cbr_table(void *table, u32 len)
{
	u32 i, j;
	u16 temp;
	u16 *tbl = (u16 *)table;

	if ((len < 8) || (len % 8) || (!table))
	{
		pr_err("ConvertTable2Risc tbl %p, len %d error\n",
				table, len);
		return;
	}
	for (i = 0; i < len / 8; i++)
	{
		j = i << 2;
		temp = tbl[j];
		tbl[j] = tbl[j + 3];
		tbl[j + 3] = temp;

		temp = tbl[j + 1];
		tbl[j + 1] = tbl[j + 2];
		tbl[j + 2] = temp;
	}
}
#endif
