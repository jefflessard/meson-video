#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>

#include "amlvenc_h264_info.h"

#define RC_MAX 0xffff
#define RC_MIN 0x0000
#define CBR_MIN_BLOCK_SIZE 4
#define DIVSCALE 16
#define DIVSCALE_SQRT 8
#define US_IN_S  1000000
#define MS_IN_S  1000
#define US_IN_MS 1000
#define RC_IDR_SCALER_RATIO 4
#define RC_QP_MIN 8
#define RC_QP_MAX 43

#define DIV_SCALED(num, den) (uint32_t)((((uint64_t) num << DIVSCALE) / den) >> DIVSCALE)

static void amlvenc_h264_rc_calculate_cbr_blocks(struct amlvenc_h264_rc_ctx *ctx) {
	uint32_t block_width_num, block_height_num;
	uint32_t block_width, block_height;
	uint32_t block_width_n, block_height_n;

	// Calculate block numbers
	block_height_num = int_sqrt(((uint64_t) CBR_BLOCK_MB_SIZE * ctx->params.mb_height << DIVSCALE)/ ctx->params.mb_width) >> DIVSCALE_SQRT;
	block_width_num = int_sqrt(((uint64_t) CBR_BLOCK_MB_SIZE * ctx->params.mb_width << DIVSCALE) / ctx->params.mb_height) >> DIVSCALE_SQRT;

	// Ensure a minimum number of blocks
	block_height_num = umax(block_height_num, ctx->params.mb_height / 32);
	block_width_num = umax(block_width_num, ctx->params.mb_width / 32);

	// Calculate block dimensions
	block_width = DIV_ROUND_UP(ctx->params.mb_width, block_width_num);
	block_height = DIV_ROUND_UP(ctx->params.mb_height, block_height_num);

	// Ensure minimum block size
	block_width = umax(block_width, CBR_MIN_BLOCK_SIZE);
	block_height = umax(block_height, CBR_MIN_BLOCK_SIZE);

	// Calculate number of blocks in each dimension
	block_width_n = DIV_ROUND_UP(ctx->params.mb_width, block_width);
	block_height_n = DIV_ROUND_UP(ctx->params.mb_height, block_height);

	pr_debug("h264_encoder: mb_w=%d (n=%d), mb_h=%d (n=%d)", ctx->params.mb_width, block_width_num, ctx->params.mb_height, block_height_num);
	pr_debug("h264_encoder: block_w=%d (n=%d), block_h=%d (n=%d)", block_width, block_width_n, block_height, block_height_n);

	ctx->block_width      = block_width;
	ctx->block_height     = block_height;
	ctx->block_width_n    = block_width_n;
	ctx->block_height_n   = block_height_n;
}

int amlvenc_h264_rc_init(struct amlvenc_h264_rc_ctx *ctx, const struct amlvenc_h264_rc_params *params, void *cbr_info, void* mb_info)
{
	if (params->mb_width == 0 || params->mb_height == 0) {
		pr_warn("h264_encoder: Invalid MB size mb_width=%d, mb_height=%d", params->mb_width, params->mb_height);
		return -EINVAL;
	}

	ctx->params = *params;
	ctx->cbr_info = (struct cbr_info_buffer *) cbr_info;
	ctx->mb_info = (struct h264_mb_info *) mb_info;

	amlvenc_h264_rc_calculate_cbr_blocks(ctx);

	ctx->block_sad_size = kcalloc(ctx->block_width_n * ctx->block_height_n, sizeof(*ctx->block_sad_size), GFP_KERNEL);
	if (!ctx->block_sad_size) {
		return -ENOMEM;
	}

	ctx->state.fullness = params->target_bitrate / 2;
	ctx->state.avg_bits_per_frame = DIV_SCALED(params->target_bitrate * params->frame_rate_den, params->frame_rate_num);
	ctx->state.target_frame_bits = ctx->state.avg_bits_per_frame;
	ctx->state.current_qp = params->initial_qp;
	ctx->state.frame_duration_ms = DIV_SCALED(MS_IN_S * ctx->params.frame_rate_den, ctx->params.frame_rate_num);
	ctx->state.last_timestamp_us = 0;

	return 0;
}

int amlvenc_h264_rc_update_stats(struct amlvenc_h264_rc_ctx *ctx, struct amlvenc_h264_configure_encoder_params *p)
{
	if (!ctx || !ctx->block_sad_size) {
		pr_err("h264_encoder: Invalid input parameters\n");
		return -EINVAL;
	}

	uint32_t mb_width = ctx->params.mb_width;
	uint32_t mb_height = ctx->params.mb_height;
	uint32_t block_width = ctx->block_width;
	uint32_t block_height = ctx->block_height;
	uint32_t block_width_n = ctx->block_width_n;
	uint32_t block_height_n = ctx->block_height_n;
	uint32_t total_blocks = block_width_n * block_height_n;

	// Initialize stats
	uint32_t invalid_count = 0;
	ctx->f_sad_count = 0;
	memset(ctx->block_sad_size, 0, total_blocks * sizeof(*ctx->block_sad_size));

	for (uint32_t y = 0; y < mb_height; y++) {
		for (uint32_t x = 0; x < mb_width; x++) {
			struct h264_mb_info *mb = amlvenc_h264_rc_mb_info(ctx, x, y);
			if (!mb) {
				pr_err("h264_encoder: Failed to get macroblock at (%u, %u)\n", x, y);
				return -EFAULT;
			}

			int32_t final_sad;
			enum amlvenc_henc_mb_type mb_type = get_mb_type(mb);

			// Calculate final_sad based on mb_type
			switch (mb_type) {
				case HENC_MB_Type_I4MB:
					final_sad = get_mb_IntraSAD(mb) - p->i4_weight;
					break;
				case HENC_MB_Type_I16MB:
					final_sad = get_mb_IntraSAD(mb) - p->i16_weight;
					break;
				case HENC_MB_Type_P16x16:
				case HENC_MB_Type_P16x8:
				case HENC_MB_Type_P8x16:
				case HENC_MB_Type_PSKIP:
					final_sad = get_mb_InterSAD(mb) - p->me_weight;
					break;
				case HENC_MB_Type_P8x8:
					final_sad = get_mb_InterSAD_ex(mb) - p->me_weight;
					break;
				case HENC_MB_Type_AUTO:
					{
						uint16_t intra_sad = get_mb_IntraSAD(mb);
						uint16_t inter_sad = get_mb_InterSAD(mb);
						if (inter_sad < intra_sad) {
							final_sad = inter_sad - p->me_weight;
						} else {
							final_sad = intra_sad - p->i16_weight;
						}
					}
					break;
				default:
					invalid_count++;
					continue;
			}

			if (final_sad < 0)
				final_sad = 0;

			// Update f_sad_count
			ctx->f_sad_count += final_sad;

			// Update block_sad_size
			uint32_t block_x = x / block_width;
			uint32_t block_y = y / block_height;
			uint32_t block_index = block_y * block_width_n + block_x;
			ctx->block_sad_size[block_index] += final_sad;
		}
	}

	pr_debug("h264_encoder: %d total SAD, %d unknown MB type", ctx->f_sad_count, invalid_count);
	return 0;
}

void amlvenc_h264_rc_hexdump_mb_info(struct amlvenc_h264_rc_ctx *ctx) {
	for(int x = 0; x < ctx->params.mb_width; x++) {	
		for(int y = 0; y < ctx->params.mb_width; y++) {
			struct h264_mb_info *mb_info = amlvenc_h264_rc_mb_info(ctx, x, y);
			if (!mb_info) {
				pr_debug("h264_encoder: NULL MB info buffer at x=%d, y=%d", x, y);
				return;
			}
			pr_debug("h264_encoder: MB info buffer x=%d, y=%d", x, y);
			print_hex_dump(KERN_DEBUG,
					"h264_encoder: MB",
					DUMP_PREFIX_OFFSET,
					16, 4,
					mb_info,
					sizeof(*mb_info),
					false);
		}
	}
}

void amlvenc_h264_rc_cbr_to_risc(struct cbr_info_buffer *cbr_info)
{
	uint32_t len = sizeof(*cbr_info);
	if (len < 8 || (len % 8) || !cbr_info) {
		pr_err("h264_encoder: Invalid cbr buffer\n");
		return;
	}

	uint16_t *tbl = (uint16_t *)cbr_info;
	for (uint32_t i = 0; i < len >> 3; i++) {
		uint32_t idx = i << 2;

		// Swap 1st and 2nd 16-bit values
		uint16_t temp = tbl[idx];
		tbl[idx] = tbl[idx + 1];
		tbl[idx + 1] = temp;

		// Swap 3rd and 4th 16-bit values
		temp = tbl[idx + 2];
		tbl[idx + 2] = tbl[idx + 3];
		tbl[idx + 3] = temp;
	}
}

void amlvenc_h264_rc_update_cbr_mb_sizes(struct amlvenc_h264_rc_ctx *ctx, bool apply_rc) {
	uint16_t *block_mb_size = ctx->cbr_info->block_mb_size;
	uint32_t total_sad_size = ctx->f_sad_count;

	if (apply_rc && total_sad_size > 0) {
		uint32_t block_size = ctx->block_width * ctx->block_height;
		uint32_t total_frame_size = 0;
		uint64_t s_target_block_size = ((uint64_t) ctx->state.target_frame_bits << (5 + DIVSCALE)) / block_size;

		for (int x = 0; x < ctx->block_height_n; x++) {
			for (int y = 0; y < ctx->block_width_n; y++) {
				uint32_t offset = x * ctx->block_width_n + y;
				uint32_t block_sad_size = ctx->block_sad_size[offset];
				uint64_t block_rc_value = (s_target_block_size * block_sad_size / total_sad_size) >> DIVSCALE;

				block_mb_size[offset] = (uint16_t) min_t(uint64_t, block_rc_value, 0xffff);
				total_frame_size += block_mb_size[offset] * block_size;
			}
		}
		pr_debug("h264_encoder: target:%d, current:%d, diff:%d",
				ctx->state.target_frame_bits >> (8 - 5),
				total_frame_size >> 8,
				((ctx->state.target_frame_bits << 5) - total_frame_size) >> 8);
	} else if (apply_rc) {
		memset(block_mb_size, RC_MIN, ctx->block_width_n * ctx->block_height_n);
	} else {
		memset(block_mb_size, RC_MAX, ctx->block_width_n * ctx->block_height_n);
	}
}


/* rate control */

static void amlvenc_h264_rc_adjust_rate(struct amlvenc_h264_rc_ctx *ctx, uint64_t timestamp, bool is_idr)
{
	uint32_t target_bits = ctx->state.avg_bits_per_frame;
	uint32_t time_elapsed_ms;
	int ratio;
  
   if (ctx->state.last_timestamp_us) {	
	   if (timestamp > ctx->state.last_timestamp_us) {
		   time_elapsed_ms = (timestamp - ctx->state.last_timestamp_us) / US_IN_MS;
		   target_bits = (target_bits * time_elapsed_ms) / ctx->state.frame_duration_ms;
		   target_bits = clamp(target_bits, ctx->state.avg_bits_per_frame * 7 /10, ctx->state.avg_bits_per_frame * 13 /10);
	   }

	   if (ctx->state.fullness >= target_bits)
		   ctx->state.fullness -= target_bits;
	   else
		   ctx->state.fullness = 0;

	   ratio = ctx->state.fullness * 100 / ctx->params.target_bitrate;
	   if (ratio < 10) {
		   ctx->state.fullness = ctx->params.target_bitrate * 10 / 100;
	   }
	   ratio = clamp(ratio, 10, 150);

	   if (ratio >= 50) {
		   ratio = min((ratio - 50) * ctx->params.frame_rate_num / (ctx->params.frame_rate_den * 10), 50);
		   ratio = 100 - ratio;
	   } else {
		   ratio = 50 - ratio;
		   ratio = 100 + ratio;
	   }

   } else {
	   ratio = 100;
   }

	target_bits = ctx->state.avg_bits_per_frame * ratio / 100;

	if (is_idr)
		target_bits *= RC_IDR_SCALER_RATIO;

	ctx->state.target_frame_bits = target_bits;
	ctx->state.last_timestamp_us = timestamp;

	pr_debug("h264_encoder: CBR target frame size: ts=%llu, ratio=%d, is_idr=%d, target=%u (avg=%u)\n",
			timestamp,
			ratio,
			is_idr,
			target_bits >> 3,
			ctx->state.avg_bits_per_frame >> 3);
}

static void amlvenc_h264_rc_update_usage(struct amlvenc_h264_rc_ctx *ctx, uint32_t frame_bits)
{
	uint32_t target = ctx->state.target_frame_bits;
	int qp_offset = 0;

	if (frame_bits > target * 3) {
		qp_offset = 10;
	} else if (frame_bits > target * 2) {
		qp_offset = 6;
	} else if (frame_bits > target * 3 / 2) {
		qp_offset = 3;
	} else if (frame_bits > target * 11 / 10) {
		qp_offset = 1;
	} else if (frame_bits < target / 2) {
		qp_offset = -4;
	} else if (frame_bits < target * 2 / 3) {
		qp_offset = -2;
	} else if (frame_bits < target * 9 / 10) {
		qp_offset = -1;
	}
	
	ctx->state.current_qp = clamp(ctx->state.current_qp + qp_offset, RC_QP_MIN, RC_QP_MAX);

	ctx->state.fullness += frame_bits;

	pr_debug("h264_encoder: CBR qp adjustments: frame=%u, target=%u, qp_offset=%d, qp=%u\n",
			frame_bits >> 3,
			ctx->state.target_frame_bits >> 3,
			qp_offset,
			ctx->state.current_qp);
}

static bool amlvenc_h264_rc_should_reencode(struct amlvenc_h264_rc_ctx *ctx, uint32_t frame_bits)
{
	// Re-encode if the frame size is significantly larger than the target
	if (ctx->state.avg_bits_per_frame > 14 * ctx->params.mb_width * ctx->params.mb_height &&
			frame_bits > ctx->state.target_frame_bits * 17 / 10) {
		return true;
	}
	return false;
}

void amlvenc_h264_rc_frame_prepare(struct amlvenc_h264_rc_ctx *ctx, uint64_t timestamp, bool is_idr)
{
	if (!ctx->params.rate_control)
		return;

	amlvenc_h264_rc_adjust_rate(ctx, timestamp, is_idr);
}

bool amlvenc_h264_rc_frame_done(struct amlvenc_h264_rc_ctx *ctx, uint32_t frame_bits)
{
	if (!ctx->params.rate_control)
		return false;
#if 0
	if (amlvenc_h264_rc_should_reencode(ctx, frame_bits)) {
		// Increase QP for re-encoding
		ctx->state.current_qp += 6 * frame_bits / ctx->state.target_frame_bits;
		ctx->state.current_qp = amlvenc_h264_rc_clamp_qp(ctx, ctx->state.current_qp);
		// The frame will be re-encoded with the new QP
		return true;
	} else {
		amlvenc_h264_rc_update_usage(ctx, timestamp, frame_bits);
		return false;
	}
#else
	amlvenc_h264_rc_update_usage(ctx, frame_bits);
	return false;
#endif
}
