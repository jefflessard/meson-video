/*
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 * Copyright (C) 2024 Jean-François Lessard
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef __AML_VENC_H264_H__
#define __AML_VENC_H264_H__

#include <linux/types.h>

#ifdef CONFIG_AMLOGIC_MEDIA_MODULE
#define AML_FRAME_SINK
#define H264_ENC_CBR
#define MULTI_SLICE_MC
#else
#define H264_ENC_CBR
#endif

#define IE_PIPELINE_BLOCK_SHIFT 0
#define IE_PIPELINE_BLOCK_MASK  0x1f
#define ME_PIXEL_MODE_SHIFT 5
#define ME_PIXEL_MODE_MASK  0x3

#ifdef H264_ENC_CBR
#define CBR_TABLE_SIZE  0x800
#define CBR_SHORT_SHIFT 12 /* same as disable */
#define CBR_LONG_MB_NUM 2
#define START_TABLE_ID 8
#define CBR_LONG_THRESH 4
#endif

/* me weight offset should not very small, it used by v1 me module. */
/* the min real sad for me is 16 by hardware. */
#define ME_WEIGHT_OFFSET 0x520
#define I4MB_WEIGHT_OFFSET 0x655
#define I16MB_WEIGHT_OFFSET 0x560
#define V3_IE_F_ZERO_SAD_I16 (I16MB_WEIGHT_OFFSET + 0x10)
#define V3_IE_F_ZERO_SAD_I4 (I4MB_WEIGHT_OFFSET + 0x20)
#define V3_ME_F_ZERO_SAD (ME_WEIGHT_OFFSET + 0x10)

#define QTABLE_SIZE 8

/* --------------------------------------------------- */
/* ENCODER_STATUS define */
/* --------------------------------------------------- */
#ifdef AML_FRAME_SINK
enum amlvenc_hcodec_status {
	ENCODER_IDLE = 0,
	ENCODER_SEQUENCE = 1,
	ENCODER_PICTURE = 2,
	ENCODER_IDR = 3,
	ENCODER_NON_IDR = 4,
	ENCODER_MB_HEADER = 5,
	ENCODER_MB_DATA = 6,

	ENCODER_SEQUENCE_DONE = 7,
	ENCODER_PICTURE_DONE = 8,
	ENCODER_IDR_DONE = 9,
	ENCODER_NON_IDR_DONE = 10,
	ENCODER_MB_HEADER_DONE = 11,
	ENCODER_MB_DATA_DONE = 12,

	ENCODER_NON_IDR_INTRA = 13,
	ENCODER_NON_IDR_INTER = 14,

	ENCODER_ERROR = 0xff,
};
#else
enum amlvenc_hcodec_status {
	ENCODER_IDLE = 0,
	ENCODER_SEQUENCE = 1,
	ENCODER_PICTURE = 2,
	ENCODER_IDR = 3,
	ENCODER_NON_IDR = 4,

	ENCODER_SEQUENCE_DONE = 7,
	ENCODER_PICTURE_DONE = 8,
	ENCODER_FRAME_DONE = 9,
	/* there is no distinction between IDR and NON_IDR done */
};
#endif

enum amlvenc_hcodec_cmd {
	CMD_ENCODE_IDLE      =  0,
	CMD_ENCODE_SEQUENCE  =  1,
	CMD_ENCODE_PICTURE   =  2,
	CMD_ENCODE_IDR       =  3, 
	CMD_ENCODE_NON_IDR   =  4,
#ifdef AML_FRAME_SINK
	CMD_ENCODE_MB_HEADER =  5,
	CMD_ENCODE_MB_DATA   =  6,
#endif
};

/********************************************
 * defines for H.264 mb_type
 *******************************************
 */
enum amlvenc_henc_mb_type {
    HENC_MB_Type_PBSKIP = 0x0,
    HENC_MB_Type_PSKIP = 0x0,
    HENC_MB_Type_BSKIP_DIRECT = 0x0,
    HENC_MB_Type_P16x16 = 0x1,
    HENC_MB_Type_P16x8 = 0x2,
    HENC_MB_Type_P8x16 = 0x3,
    HENC_MB_Type_SMB8x8 = 0x4,
    HENC_MB_Type_SMB8x4 = 0x5,
    HENC_MB_Type_SMB4x8 = 0x6,
    HENC_MB_Type_SMB4x4 = 0x7,
    HENC_MB_Type_P8x8 = 0x8,
    HENC_MB_Type_I4MB = 0x9,
    HENC_MB_Type_I16MB = 0xa,
    HENC_MB_Type_IBLOCK = 0xb,
    HENC_MB_Type_SI4MB = 0xc,
    HENC_MB_Type_I8MB = 0xd,
    HENC_MB_Type_IPCM = 0xe,
    HENC_MB_Type_AUTO = 0xf,

    HENC_MB_CBP_AUTO = 0xff,
    HENC_SKIP_RUN_AUTO = 0xffff,
};


#define QP_ROWS 8
#define QP_COLS 4

typedef struct __attribute__((packed)) {
	u8 i4_qp[QP_ROWS][QP_COLS];  /* intra 4x4 */
	u8 i16_qp[QP_ROWS][QP_COLS]; /* intra 16x16 */
	u8 me_qp[QP_ROWS][QP_COLS];  /* inter 16x16 */
} qp_table_t;

#ifdef H264_ENC_CBR
#define CBR_BLOCK_COUNT 16
#define CBR_LOG2_BLOCK_MB_SIZE 8  /* table size = 2^8 = 256 */
#define CBR_BLOCK_MB_SIZE (1 << CBR_LOG2_BLOCK_MB_SIZE)
#define CBR_TBL_BLOCK_PADDING ( \
		128 - sizeof(qp_table_t) \
		- sizeof(struct weight_offsets) \
		- sizeof(u32))
#define CBR_BUFFER_PADDING ( \
		0x2000 \
		- sizeof(struct cbr_tbl_block) * CBR_BLOCK_COUNT \
	   	- sizeof(u16) * CBR_BLOCK_MB_SIZE)

struct __attribute__((packed)) weight_offsets {
	u16 i4mb_weight_offset;
	u16 i16mb_weight_offset;
	u16 me_weight_offset;
	u16 ie_f_zero_sad_i4;
	u16 ie_f_zero_sad_i16;
	u16 me_f_zero_sad;
};

// Structure for each block in the CBR buffer
struct __attribute__((packed)) cbr_tbl_block {
	qp_table_t qp_table;
	struct weight_offsets offsets;
	u32 end_marker; // end marker 0x55aaaa55
	u8 padding[CBR_TBL_BLOCK_PADDING];  // Padding to 128 bytes
};

// Full CBR buffer layout
struct __attribute__((packed)) cbr_info_buffer {
	struct cbr_tbl_block blocks[CBR_BLOCK_COUNT];  // 16 x 128 bytes
	u16 block_mb_size[CBR_BLOCK_MB_SIZE]; // 0x800 offset
	u8 reserved[CBR_BUFFER_PADDING]; // Unused/reserved space to 0x2000
};
#endif

/**
 * struct amlvenc_h264_qtable_params - Quantization table prameters of H.264 encoder
 * @quant_tbl_i4: Quantization table for I4 mode
 * @quant_tbl_i16: Quantization table for I16 mode
 * @quant_tbl_me: Quantization table for motion estimation
 */
struct amlvenc_h264_qtable_params {
    u32 *quant_tbl_i4;
    u32 *quant_tbl_i16;
    u32 *quant_tbl_me;
};

// Struct for amlvenc_h264_init_encoder parameters
struct amlvenc_h264_init_encoder_params {
    bool idr;
    u32 idr_pic_id;
    u32 init_qppicture;
    u32 frame_number;
};

/**
 * struct amlvenc_h264_me_params - Motion estimation parameters
 * @me_mv_merge_ctl: Motion vector merge control
 * @me_step0_close_mv: Step 0 close motion vector
 * @me_sad_enough_01: SAD enough value for steps 0 and 1
 * @me_sad_enough_23: SAD enough value for steps 2 and 3
 * @me_f_skip_sad: SAD value for frame skip
 * @me_f_skip_weight: Weight for frame skip
 * @me_mv_weight_01: Motion vector weight for steps 0 and 1
 * @me_mv_weight_23: Motion vector weight for steps 2 and 3
 * @me_sad_range_inc: SAD range increment
 */
struct amlvenc_h264_me_params {
    u32 me_mv_merge_ctl;
    u32 me_step0_close_mv;
    u32 me_sad_enough_01;
    u32 me_sad_enough_23;
    u32 me_f_skip_sad;
    u32 me_f_skip_weight;
    u32 me_mv_weight_01;
    u32 me_mv_weight_23;
    u32 me_sad_range_inc;
};

struct amlvenc_h264_tnr_params {
    u32 mc_en;
    u32 txt_mode;
    u32 mot_sad_margin;
    u32 mot_cortxt_rate;
    u32 mot_distxt_ofst;
    u32 mot_distxt_rate;
    u32 mot_dismot_ofst;
    u32 mot_frcsad_lock;
    u32 mot2alp_frc_gain;
    u32 mot2alp_nrm_gain;
    u32 mot2alp_dis_gain;
    u32 mot2alp_dis_ofst;
    u32 alpha_min;
    u32 alpha_max;
    u32 deghost_os;
};

/**
 * struct amlvenc_h264_snr_params - Intra prediction and deblocking parameters
 * @err_norm:
 * @gau_bld_core:
 * @gau_bld_ofst:
 * @gau_bld_rate: SNR gaussian blur rate
 * @gau_alp0_min: Minimum SNR gaussian alpha0 value
 * @gau_alp0_max: Maximum SNR gaussian alpha0 value
 * @beta2alp_rate: Beta to alpha rate for deblocking
 * @beta_min: Minimum beta value for deblocking
 * @beta_max: Maximum beta value for deblocking
 */
struct amlvenc_h264_snr_params {
    u32 err_norm;
    u32 gau_bld_core;
    s32 gau_bld_ofst;
    u32 gau_bld_rate;
    u32 gau_alp0_min;
    u32 gau_alp0_max;
    u32 beta2alp_rate;
    u32 beta_min;
    u32 beta_max;
};

/**
 * struct amlvenc_h264_configure_encoder_params - Parameters for initializing H.264 encoder
 * @idr: IDR frame flag
 * @quant: Quantization parameter
 * @qp_mode: Quantization parameter mode
 * @encoder_width: Width of the encoded frame
 * @encoder_height: Height of the encoded frame
 * @i4_weight: I4 mode weight
 * @i16_weight: I16 mode weight
 * @me_weight: Motion estimation weight
 * @cbr_ddr_start_addr: Start address for CBR DDR
 * @cbr_start_tbl_id: Start table ID for CBR
 * @cbr_short_shift: Short shift for CBR
 * @cbr_long_mb_num: Number of long macroblocks for CBR
 * @cbr_long_th: Long threshold for CBR
 * @cbr_block_w: CBR block width
 * @cbr_block_h: CBR block height
 * @dump_ddr_start_addr: Start address for dump DDR
 * @qtable_params: Quantization tables
 * @me_params: Motion estimation parameters
 * @intra_deblock_params: Intra prediction and deblocking parameters
 */
struct amlvenc_h264_configure_encoder_params {
    bool idr;
    u32 quant;
    u32 qp_mode;
    u32 encoder_width;
    u32 encoder_height;
    u32 i4_weight;
    u32 i16_weight;
    u32 me_weight;
    u32 cbr_ddr_start_addr;
    u32 cbr_start_tbl_id;
    u32 cbr_short_shift;
    u32 cbr_long_mb_num;
    u32 cbr_long_th;
    u32 cbr_block_w;
    u32 cbr_block_h;
    u32 dump_ddr_start_addr;
    struct amlvenc_h264_qtable_params *qtable;
    struct amlvenc_h264_me_params *me;
};




// Struct for amlvenc_h264_configure_mdfin parameters
/*
 *
* FORMAT        IFMT, SFMT, RGB, SUBS, LBP, LBL, Wb, PL, W0, H0, W1, H1, W2, H2,  X0, Y0, X+, Y+                                     *
* YUV422S          0,    0,   -,  422,   -,   -, 16,  1,  1,  1,  -,  -,  -,  -,   2,  1,  -,  -                                     *
* YUV444S          1,    0,   -,  444,   -,   -, 24,  1,  1,  1,  -,  -,  -,  -,   3,  1,  -,  -                                     *
* RGB888S          1,    0,   1,  444,   -,   -, 24,  1,  1,  1,  -,  -,  -,  -,   3,  1,  -,  -                                     *
* NV21P            2,    0,   -,  420,   -,   -,  8,  2,  1,  1,  1, /2,  -,  -,   1,  1,  1, /2                                     *
* NV12P            3,    0,   -,  420,   -,   -,  8,  2,  1,  1,  1, /2,  -,  -,   1,  1,  1, /2                                     *
* YUV420P          4,    0,   -,  420,   -,   -,  8,  3,  1,  1, /2, /2, /2, /2,   1,  1, /2, /2                                     *
* YUV444P          5,    0,   -,  444,   -,   -,  8,  3,  1,  1,  1,  1,  1,  1,   1,  1,  1,  1                                     *
* RGB888P          5,    0,   1,  444,   -,   -,  8,  3,  1,  1,  1,  1,  1,  1,   1,  1,  1,  1                                     *
* YUV422P          6,    0,   -,  422,   -,   -,  8,  3,  1,  1, /2,  1, /2,  1,   1,  1, /2,  1                                     *
* YUV422S_12BIT    7,    0,   -,  422,   -,   -, 24,  1,  1,  1,  -,  -,  -,  -,   1,  1,  -,  -  >= AM_MESON_CPU_MAJOR_ID_TXL       *
* YUV444S_10BIT    7,    1,   -,  444,   -,   -, 30,  1,  1,  1,  -,  -,  -,  -,   1,  1,  -,  -  >= AM_MESON_CPU_MAJOR_ID_TXL       *
* YUV422S_10BIT    7,    2,   -,  422,   -,   -, 20,  1,  1,  1,  -,  -,  -,  -,   1,  1,  -,  -  >= AM_MESON_CPU_MAJOR_ID_TXL       *
* UNKNOWN_8L       8,    0,   ?,  444,   3,   3, 24,  -,  1,  1,  -,  -,  -,  -,   1,  1,  -,  -                                     *
* UNKNOWN_9L       9,    0,   ?,  444,   3,   3, 24,  -,  1,  1,  -,  -,  -,  -,   1,  1,  -,  -                                     *
* YUV422L         10,    0,   -,  422,   2,   2, 16,  -,  1,  1,  -,  -,  -,  -,   1,  1,  -,  -                                     *
* UNKNOWN_11L     11,    0,   ?,  420,   1,   1,  8,  -,  1,  1,  -,  -,  -,  -,   1,  1,  -,  -                                     *
* RGBA8888L       12,    0,   1,  444,   3,   4, 32,  -,  1,  1,  -,  -,  -,  -,   1,  1,  -,  -                                     *
*/
struct amlvenc_h264_mdfin_params {
    u32 width;
    u32 height;
	union {
    	u32 canvas;      /* combined canvas indexes */
		u32 addr;        /* paddr for linear input format (iformat >=8) */
	} input;
    enum mdfin_iformat_enum : u8 {
		MDFIN_IFMT_YUV422S        =  0,
		MDFIN_IFMT_YUV444S        =  1,
		MDFIN_IFMT_NV21P          =  2,
		MDFIN_IFMT_NV12P          =  3,
		MDFIN_IFMT_YUV420P        =  4,
		MDFIN_IFMT_YUV444P        =  5,
		MDFIN_IFMT_YUV422P        =  6, /* unconfirmed */
		MDFIN_IFMT_HIGH_BIT_DEPTH =  7, /* ifmt_extra */
		MDFIN_IFMT_UNKNOWN_8L     =  8, /* unkown 4:4:4 24 b/px linear format */
		MDFIN_IFMT_UNKOWNN_9L     =  9, /* unkown 4:4:4 24 b/px linear format */
		MDFIN_IFMT_YUV422L        = 10,
		MDFIN_IFMT_UNKOWN_11L     = 11, /* unkown 4:2:0 8 b/px linear format */
		MDFIN_IFMT_RGBA8888L      = 12,
	} iformat;           /* input raw format betweeen 0 and 12 */
    enum mdfin_ifmt_extra_enum: u8 {
		MDFIN_IFMT_EXTRA_NONE = 0,
		MDFIN_IFMT_EXTRA_YUV422S_12BIT = 0,
		MDFIN_IFMT_EXTRA_YUV444S_10BIT = 1,
		MDFIN_IFMT_EXTRA_YUV422S_10BIT = 2,
	} ifmt_extra;        /* subformat specifier for iformat = 7 */
    enum mdfin_subsampling_enum : u8 {
		MDFIN_SUBSAMPLING_420 = 0,
		MDFIN_SUBSAMPLING_422 = 1,
		MDFIN_SUBSAMPLING_444 = 2,
	} oformat;			 /* output format = 0 */
    enum mddfin_nr_mode_enum : u8 {
		MDFIN_NR_DISABLED = 0,
		MDFIN_NR_SNR      = 1,
		MDFIN_NR_TNR      = 2,
		MDFIN_NR_3D       = 3,
	} nr_mode;           /* nr_mode: 0:Disabled 1:SNR Only 2:TNR Only 3:3DNR */
	enum mdfin_r2y_mode_enum : u8 {
		MDFIN_R2Y_MODE0 = 0,
		MDFIN_R2Y_MODE1 = 1,
		MDFIN_R2Y_MODE2 = 2,
		MDFIN_R2Y_MODE3 = 3,
	} r2y_mode;          /* RGB2YUV Mode, range(0~3) */
	enum mdfin_bpp_enum : u8 {
		MDFIN_BPP_HALF = 0,
		MDFIN_BPP_1    = 1,
		MDFIN_BPP_2    = 2,
		MDFIN_BPP_3    = 3,
	} canv_idx0_bppx;    /* bytes per pixel in x direction for index0, 0:half 1:1 2:2 3:3 */
	enum mdfin_bpp_enum
	   canv_idx1_bppx;   /* bytes per pixel in x direction for index1-2, 0:half 1:1 2:2 3:3 */
	enum mdfin_bpp_enum
	   canv_idx0_bppy;   /* bytes per pixel in y direction for index0, 0:half 1:1 2:2 3:3 */
	enum mdfin_bpp_enum
	   canv_idx1_bppy;   /* bytes per pixel in y direction for index1-2, 0:half 1:1 2:2 3:3 */	
	u8 linear_bpp;       /* bytes per pixel of linear inpout format (iformat >= 8) */
	u32 linear_stride;   /* bytes per line of linear inpout format (iformat >= 8) */

    const struct amlvenc_h264_snr_params *y_snr;
    const struct amlvenc_h264_snr_params *c_snr;
    const struct amlvenc_h264_tnr_params *y_tnr;
    const struct amlvenc_h264_tnr_params *c_tnr;
};

extern const struct amlvenc_h264_me_params amlvenc_h264_me_defaults;
extern const struct amlvenc_h264_snr_params amlvenc_h264_snr_defaults;
extern const struct amlvenc_h264_tnr_params amlvenc_h264_tnr_defaults;


/**
 * amlvenc_h264_init_me - Initialize motion estimation parameters for H.264 encoding
 * @p: Pointer to the motion estimation parameters
 */
void amlvenc_h264_init_me(struct amlvenc_h264_me_params *p);

/**
 * amlvenc_h264_init_encoder - Initialize H.264 encoder parameters
 * @p: Pointer to the encoder initialization parameters
 */
void amlvenc_h264_init_encoder(const struct amlvenc_h264_init_encoder_params *p);

/**
 * amlvenc_h264_init_firmware_assist_buffer - Initialize firmware assist buffer
 * @assist_buffer_offset: Offset for the assist buffer
 */
void amlvenc_h264_init_firmware_assist_buffer(u32 assist_buffer_offset);

/**
 * amlvenc_h264_init_input_dct_buffer - Initialize input DCT buffer for H.264 encoding
 * @p: Pointer to the input DCT buffer parameters
 */
void amlvenc_h264_init_input_dct_buffer(u32 dct_buff_start_addr, u32 dct_buff_end_addr);

/**
 * amlvenc_h264_init_input_reference_buffer - Initialize input reference buffer
 * @canvas: Combined canvas indexes of the reference buffers
 */
void amlvenc_h264_init_input_reference_buffer(u32 canvas);

/**
 * amlvenc_h264_init_dblk_buffer - Initialize deblocking buffer
 * @canvas: Combined canvas indexes of the deblocking buffers
 */
void amlvenc_h264_init_dblk_buffer(u32 canvas);

/**
 * amlvenc_h264_init_output_stream_buffer - Initialize output stream buffer for H.264 encoding
 * @p: Pointer to the output buffer parameters
 */
void amlvenc_h264_init_output_stream_buffer(u32 bitstreamStart, u32 bitstreamEnd);

/**
 * amlvenc_h264_init_qtable - Initialize quantization tables for H.264 encoding
 * @p: Pointer to the quantization table parameters
 */
void amlvenc_h264_init_qtable(const struct amlvenc_h264_qtable_params *p);

/**
 * amlvenc_h264_configure_ie_me - Configure IE and ME parameters for H.264 encoding
 * @ie_me_mb_type:
 */
void amlvenc_h264_configure_ie_me(enum amlvenc_henc_mb_type ie_me_mb_type);

#ifdef AML_FRAME_SINK
void amlvenc_h264_configure_fixed_slice(u32 fixed_slice_cfg, u32 rows_per_slice, u32 encoder_height);
#else
void amlvenc_h264_configure_fixed_slice(u32 encoder_height, u16 p_mb_rows, u16 i_mb_rows);
#endif

void amlvenc_h264_configure_svc_pic(bool is_slc_ref);

/**
 * amlvenc_h264_configure_mdfin - Configure MDFIN parameters for H.264 encoding
 * @p: Pointer to the MDFIN configuration parameters
 */
void amlvenc_h264_configure_mdfin(struct amlvenc_h264_mdfin_params *p);
enum mdfin_subsampling_enum amlvenc_h264_iformat_subsampling(enum mdfin_iformat_enum iformat, enum mdfin_ifmt_extra_enum ifmt_extra);

/**
 * amlvenc_h264_configure_encoder - Initialize H.264 encoder
 * @p: Pointer to the initialization parameters
 *
 * This function initializes the H.264 encoder with the provided parameters.
 * It sets up various encoder settings, including picture dimensions, quantization
 * parameters, and configures hardware registers for encoding.
 */
void amlvenc_h264_configure_encoder(const struct amlvenc_h264_configure_encoder_params *p);

/**
 * amlvenc_h264_configure_me - Initialize motion estimation parameters
 * @p: Pointer to the motion estimation parameters
 *
 * This function writes the motion estimation parameters to the hardware registers.
 */
void amlvenc_h264_configure_me(const struct amlvenc_h264_me_params *p);

void amlvenc_hcodec_encode(bool enabled);
void amlvenc_hcodec_assist_enable(void);
void amlvenc_hcodec_dma_load_firmware(dma_addr_t dma_handle, size_t size);
bool amlvenc_hcodec_dma_completed(void);
enum amlvenc_hcodec_status amlvenc_hcodec_status(bool clr_irq);
void amlvenc_hcodec_cmd(enum amlvenc_hcodec_cmd cmd);
u32 amlvenc_hcodec_mb_info(void);
u32 amlvenc_hcodec_qdct_status(void);
u32 amlvenc_hcodec_vlc_total_bytes(void);
u32 amlvenc_hcodec_vlc_status(void);
u32 amlvenc_hcodec_me_status(void);
u32 amlvenc_hcodec_mpc_risc(void);
u32 amlvenc_hcodec_debug(void);


void amlvenc_dos_sw_reset1(u32 bits);
void amlvenc_dos_hcodec_memory(bool enable);
void amlvenc_dos_hcodec_gateclk(bool enable);
void amlvenc_dos_disable_auto_gateclk(void);

#ifdef AML_FRAME_SINK
int amlvenc_h264_init_mdfin(struct amlvenc_h264_mdfin_params *p);
void amlvenc_hcodec_canvas_config(u32 index, ulong addr, u32 width, u32 height, u32 wrap, u32 blkmode);
void amlvenc_hhi_hcodec_clock_on(u8 clock_level);
void amlvenc_hhi_hcodec_clock_off(void);
void amlvenc_hcodec_power_on(u8 clocklevel);
void amlvenc_hcodec_power_off(void);
#endif

#define COMBINE_CANVAS_HELPER(index1, index2, index3, ...) \
	((index1 & 0xFF) | ((index2 & 0xFF) << 8) | ((index3 & 0xFF) << 16))

#define COMBINE_CANVAS(...) \
	COMBINE_CANVAS_HELPER(__VA_ARGS__, 0, 0, 0)

#define COMBINE_CANVAS_ARRAY(indexes, num_canvas) \
	COMBINE_CANVAS_HELPER( \
			(num_canvas > 0 ? indexes[0] : 0), \
			(num_canvas > 1 ? indexes[1] : 0), \
			(num_canvas > 2 ? indexes[2] : 0), \
		)

#endif /* __AML_VENC_H264_H__ */
