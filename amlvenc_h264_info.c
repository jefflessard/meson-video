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

static void calculate_cbr_blocks(struct h264_buffer_info *info) {
	u32 block_width_num, block_height_num;
	u32 block_width, block_height;
	u32 block_width_n, block_height_n;

	// Calculate block numbers
	block_height_num = int_sqrt(((uint64_t) CBR_BLOCK_MB_SIZE * info->mb_height << DIVSCALE)/ info->mb_width) >> DIVSCALE_SQRT;
	block_width_num = int_sqrt(((uint64_t) CBR_BLOCK_MB_SIZE * info->mb_width << DIVSCALE) / info->mb_height) >> DIVSCALE_SQRT;

	// Ensure a minimum number of blocks
	block_height_num = umax(block_height_num, info->mb_height / 32);
	block_width_num = umax(block_width_num, info->mb_width / 32);

	// Calculate block dimensions
	block_width = DIV_ROUND_UP(info->mb_width, block_width_num);
	block_height = DIV_ROUND_UP(info->mb_height, block_height_num);

	// Ensure minimum block size
	block_width = umax(block_width, CBR_MIN_BLOCK_SIZE);
	block_height = umax(block_height, CBR_MIN_BLOCK_SIZE);

	// Calculate number of blocks in each dimension
	block_width_n = DIV_ROUND_UP(info->mb_width, block_width);
	block_height_n = DIV_ROUND_UP(info->mb_height, block_height);

	pr_debug("h264_encoder: mb_w=%d (n=%d), mb_h=%d (n=%d)", info->mb_width, block_width_num, info->mb_height, block_height_num);
	pr_debug("h264_encoder: block_w=%d (n=%d), block_h=%d (n=%d)", block_width, block_width_n, block_height, block_height_n);

	//	info->block_width_num  = block_width_num;
	//	info->block_height_num = block_height_num;
	info->block_width      = block_width;
	info->block_height     = block_height;
	info->block_width_n    = block_width_n;
	info->block_height_n   = block_height_n;
}

int init_h264_buffer_info(struct h264_buffer_info *info, void *cbr_info, void* mb_info, uint32_t mb_width, uint32_t mb_height)
{
	if (mb_width == 0 || mb_height == 0) {
		pr_warn("h264_encoder: Invalid MB size mb_width=%d, mb_height=%d", mb_width, mb_height);
		return -EINVAL;
	}

	info->cbr_info = (struct cbr_info_buffer *) cbr_info;
	info->mb_info = (struct h264_mb_info *) mb_info;
	info->mb_width = mb_width;
	info->mb_height = mb_height;

	calculate_cbr_blocks(info);

	info->block_sad_size = kcalloc(info->block_width_n * info->block_height_n, sizeof(*info->block_sad_size), GFP_KERNEL);
	if (!info->block_sad_size) {
		return -ENOMEM;
	}

	return 0;
}

int update_sad_stats(struct h264_buffer_info *info, struct amlvenc_h264_configure_encoder_params *p)
{
	if (!info || !info->block_sad_size) {
		pr_err("h264_encoder: Invalid input parameters\n");
		return -EINVAL;
	}

	uint32_t mb_width = info->mb_width;
	uint32_t mb_height = info->mb_height;
	uint32_t block_width = info->block_width;
	uint32_t block_height = info->block_height;
	uint32_t block_width_n = info->block_width_n;
	uint32_t block_height_n = info->block_height_n;
	uint32_t total_blocks = block_width_n * block_height_n;

	// Initialize stats
	uint32_t invalid_count = 0;
	info->f_sad_count = 0;
	memset(info->block_sad_size, 0, total_blocks * sizeof(*info->block_sad_size));

	for (uint32_t y = 0; y < mb_height; y++) {
		for (uint32_t x = 0; x < mb_width; x++) {
			struct h264_mb_info *mb = get_mb_info(info, x, y);
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
			info->f_sad_count += final_sad;

			// Update block_sad_size
			uint32_t block_x = x / block_width;
			uint32_t block_y = y / block_height;
			uint32_t block_index = block_y * block_width_n + block_x;
			info->block_sad_size[block_index] += final_sad;
		}
	}

	pr_debug("h264_encoder: %d total SAD, %d unknown MB type", info->f_sad_count, invalid_count);
	return 0;
}

void hexdump_mb_info(struct h264_buffer_info *info) {
	for(int x = 0; x < info->mb_width; x++) {	
		for(int y = 0; y < info->mb_width; y++) {
			struct h264_mb_info *mb_info = get_mb_info(info, x, y);
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

void convert_table_to_risc(struct cbr_info_buffer *cbr_info)
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

void update_cbr_mb_sizes(struct h264_buffer_info *info, bool rate_control, uint32_t bps, uint32_t fps_num, uint32_t fps_den) {
	uint16_t *block_mb_size = info->cbr_info->block_mb_size;
	u32 total_frame_size = 0;
	u32 total_sad_size = info->f_sad_count;
	u32 block_size = info->block_width * info->block_height;

	u64 s_target_frame_size = ((u64) bps * fps_den << (5 + DIVSCALE)) / fps_num; // << 5 = * 32, for 32 bits per block pixel
	u64 s_target_block_size = s_target_frame_size / block_size;

	for (int x = 0; x < info->block_height_n; x++) {
		for (int y = 0; y < info->block_width_n; y++) {
			u32 offset = x * info->block_width_n + y;
#ifdef FIXED_RC
			block_mb_size[offset] = FIXED_RC;
#else
			if (rate_control) {
				if (total_sad_size > 0) {
					u32 block_sad_size = info->block_sad_size[offset];
					u64 block_rc_value = (s_target_block_size * block_sad_size / total_sad_size) >> DIVSCALE;

					block_mb_size[offset] = (u16) min_t(u64, block_rc_value, 0xffff);
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
