#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>

#include "amlvenc_h264_info.h"

#define DIVSCALE 16
#define DIVSCALE_SQRT 8
#define DIV_SCALED(num, den) (uint32_t)((((uint64_t) num << DIVSCALE) / den) >> DIVSCALE)

#define CBR_MIN_BLOCK_SIZE 4

#define CBR_RC_MIN 0x0000
#define CBR_RC_MAX 0xffff
#define CBR_RC_P_COEFF 400
#define CBR_RC_I_COEFF  25
#define CBR_RC_D_COEFF  50
#define CBR_RC_IDR_RATIO 4

#define CBR_QP_ADJ_INTERVAL 1
#define CBR_QP_INC_THRESHOLD 12
#define CBR_QP_DEC_THRESHOLD 10
#define CBR_QP_MIN 8
#define CBR_QP_MAX 43


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

	ctx->state.avg_bits_per_frame = DIV_SCALED(params->target_bitrate * params->frame_rate_den, params->frame_rate_num);
	ctx->state.target_frame_bits = ctx->state.avg_bits_per_frame;
	ctx->state.current_qp = params->initial_qp;
	ctx->state.fullness = 0;
	ctx->state.total_bits = 0;
	ctx->state.last_error = 0;
	ctx->state.error_sum = 0;
	ctx->state.rc_frame_count = 0;
	ctx->state.qp_frame_count = 0;

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
		memset(block_mb_size, CBR_RC_MIN, ctx->block_width_n * ctx->block_height_n);
	} else {
		memset(block_mb_size, CBR_RC_MAX, ctx->block_width_n * ctx->block_height_n);
	}
}


/* rate control */

static void amlvenc_h264_rc_adjust_qp(struct amlvenc_h264_rc_ctx *ctx) {
	int32_t avg_bits_per_frame = ctx->state.avg_bits_per_frame;
	uint32_t fps_num = ctx->params.frame_rate_num;
	uint32_t fps_den = ctx->params.frame_rate_den;
	uint32_t fps = DIV_ROUND_CLOSEST(fps_num, fps_den);

	ctx->state.qp_frame_count++;

	if (ctx->state.qp_frame_count >= fps * CBR_QP_ADJ_INTERVAL) {
		int64_t mabr = ctx->state.total_bits / ctx->state.rc_frame_count;
		int64_t deviation = 100 * (mabr - avg_bits_per_frame) / avg_bits_per_frame;

		uint8_t old_qp = ctx->state.current_qp;
		int qp_change = 0;
		if (deviation > CBR_QP_INC_THRESHOLD) {
			qp_change = min(deviation / CBR_QP_INC_THRESHOLD, 6);
		} else if (deviation < -CBR_QP_DEC_THRESHOLD) {
			qp_change = max(deviation / CBR_QP_DEC_THRESHOLD, -4);
		}

		ctx->state.current_qp = clamp(ctx->state.current_qp + qp_change, CBR_QP_MIN, CBR_QP_MAX);

		if (ctx->state.current_qp != old_qp) {
			ctx->state.qp_frame_count = 0;
			ctx->state.error_sum = 0;
			// adjust fullness ?

			pr_debug("h264_encoder: CBR qp adjusted: deviation=%lld, prev=%d, qp=%d (%d)\n",
					deviation,
					old_qp,
					ctx->state.current_qp,
					qp_change);
		}
	}
}

static void amlvenc_h264_rc_adjust_rate(struct amlvenc_h264_rc_ctx *ctx, uint32_t frame_bits) {
	int32_t target_bitrate = ctx->params.target_bitrate;
	int32_t avg_bits_per_frame = ctx->state.avg_bits_per_frame;
	uint32_t fps_num = ctx->params.frame_rate_num;
	uint32_t fps_den = ctx->params.frame_rate_den;
	uint32_t fps = DIV_ROUND_CLOSEST(fps_num, fps_den);
	int64_t mabr, error;
	int64_t p_term, i_term, d_term;
	int32_t target_bits, adjustment;

	// Update moving average
	ctx->state.total_bits += frame_bits; 
	if (ctx->state.rc_frame_count >= fps) {
		ctx->state.total_bits -= ctx->state.total_bits / fps;
	}
	ctx->state.rc_frame_count = umin(ctx->state.rc_frame_count + 1, fps);
	mabr = ctx->state.total_bits / ctx->state.rc_frame_count;

	// Update buffer fullness
	ctx->state.fullness += target_bitrate / fps - frame_bits;
	ctx->state.fullness = clamp(ctx->state.fullness, target_bitrate, target_bitrate);

	// Calculate error and PID control
	error = avg_bits_per_frame - mabr;
	ctx->state.error_sum += error;
	p_term = CBR_RC_P_COEFF * error / 1000;
	i_term = CBR_RC_I_COEFF * ctx->state.error_sum / 1000;
	d_term = CBR_RC_D_COEFF * (error - ctx->state.last_error) / 1000;
	ctx->state.last_error = error;

	adjustment = p_term + i_term + d_term;
	target_bits = avg_bits_per_frame + adjustment - ctx->state.fullness / fps;
	target_bits = clamp(target_bits, avg_bits_per_frame / 2, avg_bits_per_frame * 2);

	pr_debug("h264_encoder: CBR target frame size: last=%d, mabr=%lld, target=%d (%d)\n",
			frame_bits >> 3,
			mabr >> 3,
			target_bits >> 3,
			ctx->state.avg_bits_per_frame >> 3);

	ctx->state.target_frame_bits = target_bits;
}

void amlvenc_h264_rc_frame_prepare(struct amlvenc_h264_rc_ctx *ctx, bool is_idr)
{
	if (!ctx->params.rate_control)
		return;

	if (is_idr)
		ctx->state.target_frame_bits *= CBR_RC_IDR_RATIO;
}

bool amlvenc_h264_rc_frame_done(struct amlvenc_h264_rc_ctx *ctx, uint32_t frame_bits)
{
	if (!ctx->params.rate_control)
		return false;
		
	amlvenc_h264_rc_adjust_rate(ctx, frame_bits);
	amlvenc_h264_rc_adjust_qp(ctx);
	return false;
}
