#ifndef __AML_VENC_H264_BUFFERS_H__
#define __AML_VENC_H264_BUFFERS_H__

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>

#include "amlvenc_h264.h"

#define MB_INFO_SIZE 80  // Size per macroblock

#define CBR_BLOCK_COUNT 16
#define CBR_LOG2_BLOCK_MB_SIZE 8  /* table size = 2^8 = 256 */
#define CBR_BLOCK_MB_SIZE (1 << CBR_LOG2_BLOCK_MB_SIZE)
#define CBR_TBL_BLOCK_PADDING ( \
		128 - sizeof(qp_table_t) \
		- 8 * sizeof(uint16_t)) // offsets
#define CBR_BUFFER_PADDING ( \
		0x2000 \
		- sizeof(struct amlvenc_h264_cbr_tbl_block) * CBR_BLOCK_COUNT \
	   	- sizeof(uint16_t) * CBR_BLOCK_MB_SIZE)
#define CBR_TBL_BLOCK_END_MARKER1 0x55aa
#define CBR_TBL_BLOCK_END_MARKER2 0xaa55

// Structure for each block in the CBR buffer
struct amlvenc_h264_cbr_tbl_block {
	qp_table_t qp_table;
	uint16_t i4mb_weight_offset;
	uint16_t i16mb_weight_offset;
	uint16_t me_weight_offset;
	uint16_t ie_f_zero_sad_i4;
	uint16_t ie_f_zero_sad_i16;
	uint16_t me_f_zero_sad;
	uint16_t end_marker1; /* 0x55aa */
	uint16_t end_marker2; /* 0xaa55 */
	uint8_t padding[CBR_TBL_BLOCK_PADDING];  // Padding to 128 bytes
};
static_assert(sizeof(struct amlvenc_h264_cbr_tbl_block) == 128, "amlvenc_h264_cbr_tbl_block must be exactly 128 bytes");

// Full CBR buffer layout
struct amlvenc_h264_cbr_info {
	struct amlvenc_h264_cbr_tbl_block blocks[CBR_BLOCK_COUNT];  // 16 x 128 bytes
	uint16_t block_mb_size[CBR_BLOCK_MB_SIZE]; // 0x800 offset
};
static_assert(sizeof(struct amlvenc_h264_cbr_info) % 16 == 0, "amlvenc_h264_cbr_info size must be a multiple of 16 bytes");
static_assert(offsetof(struct amlvenc_h264_cbr_info, block_mb_size) == CBR_TABLE_SIZE, "amlvenc_h264_cbr_info block_mb_size must be at offset CBR_TABLE_SIZE");


#if 0
// used types
enum HENC_MB_Type {
	HENC_MB_Type_I4MB,
	HENC_MB_Type_I16MB,
	HENC_MB_Type_P16x16,
	HENC_MB_Type_P16x8,
	HENC_MB_Type_P8x16,
	HENC_MB_Type_P8x8,
	HENC_MB_Type_PSKIP,
	HENC_MB_Type_AUTO
};
#endif

struct amlvenc_h264_cbr_mb_info {
	// Offset 0-3: Used for I4x4 mode (lower 4 bits of each byte)
	uint8_t i4x4_modes_low[4];
	// Offset 4: MB CPred
	uint8_t mb_cpred;
	// Offset 5: MB type
	enum amlvenc_henc_mb_type mb_type;
	// Offset 6: MB Y coordinate
	uint8_t mb_y;
	// Offset 7: MB X coordinate
	uint8_t mb_x;
	// Offset 8-9: Intra SAD
	uint16_t intra_sad;
	// Offset 10: CBP
	uint8_t cbp;
	// Offset 11: Quant
	uint8_t quant;
	// Offset 12-15: Used for I4x4 mode (upper 4 bits of each byte) and I16 mode
	union {
		uint8_t i4x4_modes_high[4];
		struct {
			uint8_t i16_mode : 4;
			uint8_t reserved : 4;
		};
	};
	// Offset 16-17: Bits
	uint16_t bits;
	// Offset 18-21: Reserved
	uint8_t reserved1[4];
	// Offset 22-23: Inter SAD
	uint16_t inter_sad;
	// Offset 24-63: Motion vectors (10 int16_t pairs)
	int16_t mv[20];
	// Offset 64-65: Extended Intra SAD
	uint16_t intra_sad_ex;
	// Offset 66: Extended CBP
	uint8_t cbp_ex;
	// Offset 67: Extended Quant
	uint8_t quant_ex;
	// Offset 68-71: Reserved
	uint8_t reserved2[4];
	// Offset 72-73: Extended Bits
	uint16_t bits_ex;
	// Offset 74-77: Reserved
	uint8_t reserved3[4];
	// Offset 78-79: Extended Inter SAD
	uint16_t inter_sad_ex;
};
static_assert(sizeof(struct amlvenc_h264_cbr_mb_info) == MB_INFO_SIZE, "amlvenc_h264_cbr_mb_info must be exactly MB_INFO_SIZE bytes");

struct amlvenc_h264_cbr_params {
	int32_t target_bitrate;
	uint32_t frame_rate_num;
	uint32_t frame_rate_den;
	uint32_t mb_width;
	uint32_t mb_height;
	uint8_t min_qp;
	uint8_t max_qp;
	uint8_t initial_qp;
	bool rate_control;
};

struct amlvenc_h264_cbr_state {
	int64_t total_bits;
	int64_t fullness;
	int64_t last_error;
	int64_t error_sum;
	uint32_t avg_bits_per_frame;
	uint32_t target_frame_bits;
	uint8_t rc_frame_count;
	uint8_t qp_frame_count;
	uint8_t current_qp;
};

struct amlvenc_h264_cbr_ctx {
	struct amlvenc_h264_cbr_params params;
	struct amlvenc_h264_cbr_info *cbr_info;
	struct amlvenc_h264_cbr_mb_info *mb_info;
	struct amlvenc_h264_cbr_state state;

	uint32_t block_width;
	uint32_t block_height;
	uint32_t block_width_n;
	uint32_t block_height_n;

	uint32_t f_sad_count;
	uint32_t *block_sad_size;
};

struct mb_mv {
	int16_t mvx;
	int16_t mvy;
};

int amlvenc_h264_cbr_init(struct amlvenc_h264_cbr_ctx *ctx, const struct amlvenc_h264_cbr_params *params, void *cbr_info, void* mb_info);

static inline void amlvenc_h264_cbr_free(struct amlvenc_h264_cbr_ctx *ctx) {
	if (ctx) {
		kfree(ctx->block_sad_size);
	}
}

static inline struct amlvenc_h264_cbr_mb_info *amlvenc_h264_cbr_mb_info(struct amlvenc_h264_cbr_ctx *ctx, uint32_t x, uint32_t y)
{
	if (x >= ctx->params.mb_width || y >= ctx->params.mb_height)
		return NULL;
	return &ctx->mb_info[y * ctx->params.mb_width + x];
}

int amlvenc_h264_cbr_update_stats(struct amlvenc_h264_cbr_ctx *ctx, struct amlvenc_h264_configure_encoder_params *p);

void amlvenc_h264_cbr_clamp_qptbl(struct amlvenc_h264_cbr_ctx *ctx, qp_union_t *tbl);

void amlvenc_h264_cbr_update_quant_tables(struct amlvenc_h264_cbr_ctx *ctx, bool apply_rc, bool is_idr, const qp_union_t *qp_adj, const qp_union_t *qp_adj_i4i16);

void amlvenc_h264_cbr_to_risc(struct amlvenc_h264_cbr_info *cbr_info);

void amlvenc_h264_cbr_hexdump_mb_info(struct amlvenc_h264_cbr_ctx *ctx);

void amlvenc_h264_cbr_update_mb_sizes(struct amlvenc_h264_cbr_ctx *ctx, bool apply_rc);

void amlvenc_h264_cbr_frame_prepare(struct amlvenc_h264_cbr_ctx *ctx, bool is_idr);

bool amlvenc_h264_cbr_frame_done(struct amlvenc_h264_cbr_ctx *ctx, uint32_t frame_bits);

// Helper macros to access fields in the raw buffer, matching the original code
#define get_mb_x(mb) ((mb)->mb_x)
#define get_mb_y(mb) ((mb)->mb_y)
#define get_mb_type(mb) ((mb)->mb_type)
#define get_mb_CPred(mb) ((mb)->mb_cpred)
#define get_mb_LPred_I16(mb) ((mb)->i16_mode)
#define get_mb_quant(mb) ((mb)->quant)
#define get_mb_cbp(mb) ((mb)->cbp)
#define get_mb_IntraSAD(mb) ((mb)->intra_sad)
#define get_mb_InterSAD(mb) ((mb)->inter_sad)
#define get_mb_bits(mb) ((mb)->bits)

#define get_mb_quant_ex(mb) ((mb)->quant_ex)
#define get_mb_cbp_ex(mb) ((mb)->cbp_ex)
#define get_mb_IntraSAD_ex(mb) ((mb)->intra_sad_ex)
#define get_mb_InterSAD_ex(mb) ((mb)->inter_sad_ex)
#define get_mb_bits_ex(mb) ((mb)->bits_ex)

// Helper function to get I4x4 modes
static inline void get_mb_LPred_I4(struct amlvenc_h264_cbr_mb_info *mb, uint8_t mode[16])
{
	for (int i = 0; i < 4; i++) {
		mode[i*2] = mb->i4x4_modes_low[i] & 0xf;
		mode[i*2 + 1] = (mb->i4x4_modes_low[i] >> 4) & 0xf;
		mode[8 + i*2] = mb->i4x4_modes_high[i] & 0xf;
		mode[8 + i*2 + 1] = (mb->i4x4_modes_high[i] >> 4) & 0xf;
	}
}

// Helper functions to get motion vectors
static inline void get_mb_mv_P16x16(struct amlvenc_h264_cbr_mb_info *mb, struct mb_mv mv[16])
{
	for (int i = 0; i < 16; i++) {
		mv[i].mvx = mb->mv[0];
		mv[i].mvy = mb->mv[1];
	}
}

static inline void get_mb_mv_P16x8(struct amlvenc_h264_cbr_mb_info *mb, struct mb_mv mv[16])
{
	for (int i = 0; i < 8; i++) {
		mv[i].mvx = mb->mv[0];
		mv[i].mvy = mb->mv[1];
		mv[i+8].mvx = mb->mv[6];
		mv[i+8].mvy = mb->mv[7];
	}
}

static inline void get_mb_mv_P8x16(struct amlvenc_h264_cbr_mb_info *mb, struct mb_mv mv[16])
{
	for (int i = 0; i < 4; i++) {
		mv[i].mvx = mv[i+8].mvx = mb->mv[0];
		mv[i].mvy = mv[i+8].mvy = mb->mv[1];
		mv[i+4].mvx = mv[i+12].mvx = mb->mv[6];
		mv[i+4].mvy = mv[i+12].mvy = mb->mv[7];
	}
}

static inline void get_mb_mv_P8x8(struct amlvenc_h264_cbr_mb_info *mb, struct mb_mv mv[16])
{
	for (int i = 0; i < 4; i++) {
		mv[i*2].mvx = mv[i*2+1].mvx = mb->mv[i*4];
		mv[i*2].mvy = mv[i*2+1].mvy = mb->mv[i*4+1];
		mv[i*2+8].mvx = mv[i*2+9].mvx = mb->mv[i*4+2];
		mv[i*2+8].mvy = mv[i*2+9].mvy = mb->mv[i*4+3];
	}
}

#endif /* __AML_VENC_H264_BUFFERS_H__ */
