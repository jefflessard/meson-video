#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>

#include "amlvenc_h264_info.h"

#define RC_MAX 0xffff
#define RC_MIN 0x0000
#define CBR_MIN_BLOCK_SIZE 4
#define DIVSCALE 16
#define DIVSCALE_SQRT 8
//#define FIXED_RC RC_MIN
//#define FIXED_RC_QP
#define IDR_SCALER_RATIO 4
#define BUFFER_LOW_THRESHOLD 10
#define BUFFER_HIGH_THRESHOLD 150
#define QP_MIN 8
#define QP_MAX_LOW_BITRATE 38
#define QP_MAX_HIGH_BITRATE 32
#define MAX_QP_COUNT_THRESHOLD 40
#define BITRATE_SCALE_THRESHOLD 5000000

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

	//ctx->block_width_num  = block_width_num;
	//ctx->block_height_num = block_height_num;
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

	ctx->state.target_bitrate = params->initial_target_bitrate;
	ctx->state.buffer_fullness = params->initial_target_bitrate / 2;
	ctx->state.bits_per_frame = DIV_SCALED(params->initial_target_bitrate * params->frame_rate_den, params->frame_rate_num);
	ctx->state.current_qp = params->initial_qp;
	ctx->state.encoded_frames = 0;
	ctx->state.last_timestamp = 0;
	ctx->state.max_qp_count = 0;
#if 0
	ctx->state.me_weight = 0;
#endif

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

void amlvenc_h264_rc_update_cbr_mb_sizes(struct amlvenc_h264_rc_ctx *ctx) {
	uint32_t fps_num = ctx->params.frame_rate_num;
	uint32_t fps_den = ctx->params.frame_rate_den;
	uint32_t bps = ctx->state.target_bitrate;
	uint16_t *block_mb_size = ctx->cbr_info->block_mb_size;
	uint32_t total_frame_size = 0;
	uint32_t total_sad_size = ctx->f_sad_count;
	uint32_t block_size = ctx->block_width * ctx->block_height;
	bool rate_control = ctx->params.rate_control;

	uint64_t s_target_frame_size = ((uint64_t) bps * fps_den << (5 + DIVSCALE)) / fps_num; // << 5 = * 32, for 32 bits per block pixel
	uint64_t s_target_block_size = s_target_frame_size / block_size;

	for (int x = 0; x < ctx->block_height_n; x++) {
		for (int y = 0; y < ctx->block_width_n; y++) {
			uint32_t offset = x * ctx->block_width_n + y;
#ifdef FIXED_RC
			block_mb_size[offset] = FIXED_RC;
#else
			if (rate_control) {
				if (total_sad_size > 0) {
					uint32_t block_sad_size = ctx->block_sad_size[offset];
					uint64_t block_rc_value = (s_target_block_size * block_sad_size / total_sad_size) >> DIVSCALE;

					block_mb_size[offset] = (uint16_t) min_t(uint64_t, block_rc_value, 0xffff);
				} else {
					block_mb_size[offset] = RC_MAX;
				}
			} else {
				block_mb_size[offset] = RC_MAX;
			}
			total_frame_size += block_mb_size[offset] * block_size;
#endif
		}
	}

	pr_debug("h264_encoder: target:%llu, current:%d, diff:%llu",
			s_target_frame_size >> (8 + DIVSCALE),
			total_frame_size >> 8,
			((s_target_frame_size >> DIVSCALE) - total_frame_size) >> 8);
}


/* rate control */

static void amlvenc_h264_rc_scale_bitrate(struct amlvenc_h264_rc_ctx *ctx)
{
	if (ctx->state.target_bitrate >= BITRATE_SCALE_THRESHOLD)
		return;

	uint32_t ratio = 100;
	uint32_t int_frame_rate = ctx->params.frame_rate_num / ctx->params.frame_rate_den; 
	if (int_frame_rate < 10)
		ratio = 130;
	else if (int_frame_rate < 15)
		ratio = 120;
	else if (int_frame_rate < 20)
		ratio = 110;

	ctx->state.bits_per_frame = (uint32_t)(ctx->state.bits_per_frame * ratio / 100);
	ctx->state.target_bitrate = DIV_SCALED(ctx->state.bits_per_frame * ctx->params.frame_rate_num, ctx->params.frame_rate_den);
}

static void amlvenc_h264_rc_update_buffer(struct amlvenc_h264_rc_ctx *ctx, uint64_t timestamp)
{
	uint32_t frame_duration = DIV_SCALED(1000000 * ctx->params.frame_rate_den, ctx->params.frame_rate_num); // in microseconds
	uint32_t time_elapsed = timestamp - ctx->state.last_timestamp;
	uint32_t bits_to_remove = DIV_SCALED(ctx->state.bits_per_frame * time_elapsed, frame_duration);

	if (ctx->state.buffer_fullness >= bits_to_remove)
		ctx->state.buffer_fullness -= bits_to_remove;
	else
		ctx->state.buffer_fullness = 0;

	ctx->state.last_timestamp = timestamp;
}

static void amlvenc_h264_rc_calculate_qp(struct amlvenc_h264_rc_ctx *ctx)
{
	uint32_t buffer_status = (ctx->state.buffer_fullness * 100) / ctx->state.target_bitrate;
	uint8_t qp_delta = 0;

	if (buffer_status < BUFFER_LOW_THRESHOLD)
		qp_delta = 2;
	else if (buffer_status > BUFFER_HIGH_THRESHOLD)
		qp_delta = -2;

	ctx->state.current_qp += qp_delta;

	// Incorporate SAD factor
	ctx->state.current_qp += ctx->f_sad_count * 6 / (ctx->params.mb_width * ctx->params.mb_height);

	// Track max QP count
	if (ctx->state.current_qp >= QP_MAX_LOW_BITRATE - 5) {
		ctx->state.max_qp_count++;
	} else {
		ctx->state.max_qp_count = 0;
	}

#if 0
	// ME weight adjustment
	if (ctx->state.max_qp_count > MAX_QP_COUNT_THRESHOLD) {
		if ((uint32_t)(ctx->params.mb_width * ctx->params.mb_height) > 3 * ctx->f_sad_count) {
			ctx->state.me_weight = 0;
		} else {
			ctx->state.me_weight = 1;
		}
	}
#endif

	// Clamp QP
	ctx->state.current_qp = clamp(ctx->state.current_qp, QP_MIN, 
			(ctx->state.target_bitrate < 1000000) ? QP_MAX_LOW_BITRATE : QP_MAX_HIGH_BITRATE);

#if 0
	// Additional QP adjustments based on ME weight
	if (ctx->state.me_weight > 0 && ctx->state.current_qp > 35) {
		ctx->state.current_qp = 35;
	}
#endif
}

static uint32_t amlvenc_h264_rc_get_target_bits(struct amlvenc_h264_rc_ctx *ctx, bool is_idr)
{
	uint32_t target_bits = ctx->state.bits_per_frame;
	int buffer_status = (ctx->state.buffer_fullness * 100) / ctx->state.target_bitrate;

	if (buffer_status < 50)
		target_bits = (target_bits * (100 + (50 - buffer_status))) / 100;
	else if (buffer_status > 100)
		target_bits = (target_bits * (100 - (buffer_status - 100))) / 100;

	if (is_idr)
		target_bits *= IDR_SCALER_RATIO;

	return target_bits;
}

static void amlvenc_h264_rc_post_frame(struct amlvenc_h264_rc_ctx *ctx, uint32_t frame_bits)
{
	ctx->state.buffer_fullness += frame_bits;
	ctx->state.last_frame_bits = frame_bits;
	ctx->state.encoded_frames++;
}

static bool amlvenc_h264_rc_should_reencode(struct amlvenc_h264_rc_ctx *ctx, uint32_t frame_bits)
{
	// Re-encode if the frame size is significantly larger than the target
	if (ctx->state.bits_per_frame > 14 * ctx->params.mb_width * ctx->params.mb_height / 1000 &&
			frame_bits > ctx->state.target_bits * 17 / 10) {
		return true;
	}
	return false;
}

void amlvenc_h264_rc_frame_prepare(struct amlvenc_h264_rc_ctx *ctx, uint64_t timestamp, bool is_idr)
{
	if (!ctx->params.rate_control)
		return;

	amlvenc_h264_rc_scale_bitrate(ctx);
	amlvenc_h264_rc_update_buffer(ctx, timestamp);
	amlvenc_h264_rc_calculate_qp(ctx);
	ctx->state.target_bits = amlvenc_h264_rc_get_target_bits(ctx, is_idr);
}

bool amlvenc_h264_rc_frame_done(struct amlvenc_h264_rc_ctx *ctx, uint32_t frame_bits)
{
	if (!ctx->params.rate_control)
		return false;

	if (amlvenc_h264_rc_should_reencode(ctx, frame_bits)) {
		// Increase QP for re-encoding
		ctx->state.current_qp += 6 * frame_bits / ctx->state.target_bits;
		ctx->state.current_qp = clamp(ctx->state.current_qp, QP_MIN, 
				(ctx->state.target_bitrate < 1000000) ? QP_MAX_LOW_BITRATE : QP_MAX_HIGH_BITRATE);
		// The frame will be re-encoded with the new QP
		return true;
	} else {
		amlvenc_h264_rc_post_frame(ctx, frame_bits);
		return false;
	}
}
