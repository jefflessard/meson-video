#include "amlvdec_h264_dpb.h"

static inline void amlvdec_h264_dpb_dump_pic(const char *prefix, dpb_entry_t *entry) {
	pr_debug("DPB: %s: frame=%d, idr=%d, poc=%d (%d, %d) index=%d, used=%d, ref=%d, lt=%d, gap=%d, decoded=%d, output=%d\n",
			prefix,
			entry->frame_num,
			entry->is_idr,
			entry->frame_poc,
			entry->top_poc,
			entry->bottom_poc,
			entry->buffer_index,
			entry->is_used,
			entry->is_reference,
			entry->is_long_term,
			entry->is_gap_frame,
			entry->is_decoded,
			entry->is_output);
}

static void amlvdec_h264_dpb_decode_poc(const struct amlvdec_h264_lmem *lmem, poc_state_t *poc, dpb_entry_t *entry)
{
	const struct amlvdec_h264_rpm *params = &lmem->params;
	const struct amlvdec_h264_dpb *dpb = &lmem->dpb;
	const struct amlvdec_h264_mmco *mmco = &lmem->mmco;

	const bool field_pic_flag =
		entry->picture_structure == MMCO_TOP_FIELD ||
		entry->picture_structure == MMCO_BOTTOM_FIELD;
	const bool bottom_field_flag =
		entry->picture_structure == MMCO_BOTTOM_FIELD;
	const uint16_t pic_order_cnt_lsb = dpb->pic_order_cnt_lsb;
	const int frame_num = dpb->frame_num;
	const int delta_pic_order_cnt_bottom = 
		dpb->delta_pic_order_cnt_bottom_lo |
		(dpb->delta_pic_order_cnt_bottom_hi << 16);
	const int delta_pic_order_cnt_0 = 
		dpb->delta_pic_order_cnt_0_lo |
		(dpb->delta_pic_order_cnt_0_hi << 16);
	const int delta_pic_order_cnt_1 = 
		dpb->delta_pic_order_cnt_1_lo |
		(dpb->delta_pic_order_cnt_1_hi << 16);
	const uint32_t MaxPicOrderCntLsb = (1 << (params->log2_max_pic_order_cnt_lsb));
	const int max_frame_num = 1 << params->log2_max_frame_num;
	const uint16_t *offset_for_ref_frame = mmco->offset_for_ref_frame_base;
	const bool is_reference = entry->is_reference;

	int ThisPOC = 0;

	switch (params->pic_order_cnt_type) {
		case 0: /* POC MODE 0 */
			// Reset POC MSB and LSB for IDR pictures or after MMCO5
			if (entry->is_idr) {
				poc->PrevPicOrderCntMsb = 0;
				poc->PrevPicOrderCntLsb = 0;
			} else if (poc->last_mmco_has_reset) {
				if (poc->last_pic_bottom_field) {
					poc->PrevPicOrderCntMsb = 0;
					poc->PrevPicOrderCntLsb = 0;
				} else {
					poc->PrevPicOrderCntMsb = 0;
					poc->PrevPicOrderCntLsb = entry->top_poc;
				}
			}

			// Calculate POC MSB
			if (pic_order_cnt_lsb < poc->PrevPicOrderCntLsb &&
					(poc->PrevPicOrderCntLsb - pic_order_cnt_lsb) >= (MaxPicOrderCntLsb >> 1))
				poc->PicOrderCntMsb = poc->PrevPicOrderCntMsb + MaxPicOrderCntLsb;
			else if (pic_order_cnt_lsb > poc->PrevPicOrderCntLsb &&
					(pic_order_cnt_lsb - poc->PrevPicOrderCntLsb) > (MaxPicOrderCntLsb >> 1))
				poc->PicOrderCntMsb = poc->PrevPicOrderCntMsb - MaxPicOrderCntLsb;
			else
				poc->PicOrderCntMsb = poc->PrevPicOrderCntMsb;

			// Calculate POC based on picture type
			if (!field_pic_flag) {
				entry->top_poc = poc->PicOrderCntMsb + pic_order_cnt_lsb;
				entry->bottom_poc = entry->top_poc + delta_pic_order_cnt_bottom;
				ThisPOC = entry->frame_poc = 
					(entry->top_poc < entry->bottom_poc) ? entry->top_poc : entry->bottom_poc;
			} else if (!bottom_field_flag) {
				ThisPOC = entry->top_poc = 
					poc->PicOrderCntMsb + pic_order_cnt_lsb;
			} else {
				ThisPOC = entry->bottom_poc = 
					poc->PicOrderCntMsb + pic_order_cnt_lsb;
			}
			entry->frame_poc = ThisPOC;

			// Update previous values for reference pictures
			poc->PreviousFrameNum = frame_num;
			if (is_reference) {
				poc->PrevPicOrderCntLsb = pic_order_cnt_lsb;
				poc->PrevPicOrderCntMsb = poc->PicOrderCntMsb;
			}
			break;

		case 1: /* POC MODE 1 */
			// Calculate FrameNumOffset
			if (entry->is_idr) {
				poc->FrameNumOffset = 0;
			} else {
				if (poc->last_mmco_has_reset) {
					poc->PreviousFrameNumOffset = 0;
					poc->PreviousFrameNum = 0;
				}
				if (frame_num < poc->PreviousFrameNum)
					poc->FrameNumOffset = poc->PreviousFrameNumOffset + max_frame_num;
				else
					poc->FrameNumOffset = poc->PreviousFrameNumOffset;
			}

			// Calculate AbsFrameNum
			if (params->num_ref_frames_in_pic_order_cnt_cycle)
				poc->AbsFrameNum = poc->FrameNumOffset + frame_num;
			else
				poc->AbsFrameNum = 0;

			if ((!is_reference) && poc->AbsFrameNum > 0)
				poc->AbsFrameNum--;

			// Calculate ExpectedDeltaPerPicOrderCntCycle
			poc->ExpectedDeltaPerPicOrderCntCycle = 0;
			if (params->num_ref_frames_in_pic_order_cnt_cycle) {
				for (int i = 0; i < params->num_ref_frames_in_pic_order_cnt_cycle && i < 128; i++)
					poc->ExpectedDeltaPerPicOrderCntCycle += offset_for_ref_frame[i];
			}

			// Calculate ExpectedPicOrderCnt
			if (poc->AbsFrameNum) {
				poc->PicOrderCntCycleCnt = (poc->AbsFrameNum - 1) / 
					params->num_ref_frames_in_pic_order_cnt_cycle;
				poc->FrameNumInPicOrderCntCycle = (poc->AbsFrameNum - 1) % 
					params->num_ref_frames_in_pic_order_cnt_cycle;
				poc->ExpectedPicOrderCnt = poc->PicOrderCntCycleCnt * 
					poc->ExpectedDeltaPerPicOrderCntCycle;

				for (int i = 0; i <= poc->FrameNumInPicOrderCntCycle; i++)
					poc->ExpectedPicOrderCnt += offset_for_ref_frame[i];
			} else {
				poc->ExpectedPicOrderCnt = 0;
			}

			if (!is_reference)
				poc->ExpectedPicOrderCnt += params->offset_for_non_ref_pic;

			// Calculate final POC values
			if (!field_pic_flag) {
				entry->top_poc = poc->ExpectedPicOrderCnt + delta_pic_order_cnt_0;
				entry->bottom_poc = entry->top_poc + params->offset_for_top_to_bottom_field + delta_pic_order_cnt_1;
				ThisPOC = entry->frame_poc = 
					(entry->top_poc < entry->bottom_poc) ? entry->top_poc : entry->bottom_poc;
			} else if (!bottom_field_flag) {
				ThisPOC = entry->top_poc = 
					poc->ExpectedPicOrderCnt + delta_pic_order_cnt_0;
			} else {
				ThisPOC = entry->bottom_poc = poc->ExpectedPicOrderCnt + 
					params->offset_for_top_to_bottom_field + delta_pic_order_cnt_0;
			}
			entry->frame_poc = ThisPOC;

			// Update previous values
			poc->PreviousFrameNum = frame_num;
			poc->PreviousFrameNumOffset = poc->FrameNumOffset;
			break;

		case 2: /* POC MODE 2 */
			if (entry->is_idr) {
				poc->FrameNumOffset = 0;
				ThisPOC = entry->frame_poc = entry->top_poc = entry->bottom_poc = 0;
			} else {
				if (poc->last_mmco_has_reset) {
					poc->PreviousFrameNum = 0;
					poc->PreviousFrameNumOffset = 0;
				}
				if (frame_num < poc->PreviousFrameNum)
					poc->FrameNumOffset = poc->PreviousFrameNumOffset + max_frame_num;
				else
					poc->FrameNumOffset = poc->PreviousFrameNumOffset;

				poc->AbsFrameNum = poc->FrameNumOffset + frame_num;
				ThisPOC = (!is_reference) ? 
					(2 * poc->AbsFrameNum - 1) : (2 * poc->AbsFrameNum);

				if (!field_pic_flag)
					entry->top_poc = entry->bottom_poc = entry->frame_poc = ThisPOC;
				else if (!bottom_field_flag)
					entry->top_poc = entry->frame_poc = ThisPOC;
				else
					entry->bottom_poc = entry->frame_poc = ThisPOC;
			}

			poc->PreviousFrameNum = frame_num;
			poc->PreviousFrameNumOffset = poc->FrameNumOffset;
			break;
	}
}

// Initialize DPB buffer
void amlvdec_h264_dpb_init(dpb_buffer_t *dpb, const struct amlvdec_h264_lmem *hw_dpb) {
	memset(dpb, 0, sizeof(dpb_buffer_t));
	dpb->hw_dpb = hw_dpb;
	dpb->max_frames = 0;
}

int amlvdec_h264_dpb_adjust_size(dpb_buffer_t *dpb) {
	int max_num_refs = dpb->hw_dpb->params.max_reference_frame_num;

	/* More granular size could potentially be
	 * determined by using the following values
	 */
#if 0
	int bitstream_restriction_flag = ctx->lmem->params.sps_flags2.bitstream_restriction_flag;
	int num_reorder_frames = dpb->hw_dpb->params.num_reorder_frames;
	int max_dec_frame_buffering = dpb->hw_dpb->params.max_buffer_frame;
	enum profile_level level = dpb->hw_dpb->params.level_idc_mmco;
	int max_long_term_frame_idx = dpb->max_long_term_frame_idx;
#endif

	int new_size = umin(max_num_refs + DPB_SIZE_MARGIN, MAX_DPB_FRAMES);
	if (new_size > dpb->max_frames) {
		/* increase DPB size */
		/* shrinking would require to init
		 * the increased dpb entries */
		dpb->max_frames = new_size;
	} else if (new_size < dpb->max_frames) {
		/* ignore DPB shrinking */
		/* shrinking would require to sort
		 * and filter the dpb entries first */
	}

	return dpb->max_frames;
}

static dpb_entry_t* amlvdec_h264_dpb_alloc_pic(dpb_buffer_t *dpb) {

	// Count reference frames and find oldest one
	for (int i = 0; i < dpb->max_frames; i++) {
		if (!dpb->frames[i].is_used) {
			return &dpb->frames[i];
		}
	}

	// DPB full with reference frames - error condition
	pr_err("DPB: DPB full with reference frames");
	return NULL;
}

static void amlvdec_h264_dpb_fill_gaps(dpb_buffer_t *dpb, int current_frame_num) {
    int max_frame_num = 1 << dpb->hw_dpb->params.log2_max_frame_num;

    if (!dpb->hw_dpb->params.frame_num_gap_allowed)
        return;

    int expected_frame_num = (dpb->prev_frame_num + 1) % max_frame_num;
    
    while (expected_frame_num != current_frame_num) {
		dpb_entry_t *gap_entry = amlvdec_h264_dpb_alloc_pic(dpb);
        
        if (gap_entry) {
            // Initialize gap frame
            gap_entry->is_used = true;
            gap_entry->is_gap_frame = true;
            gap_entry->frame_num = expected_frame_num;
            gap_entry->is_reference = true;
            gap_entry->is_long_term = false;
            // Other fields set to default values
        }
        // TODO: Handle error case when no empty slot is available
        
        expected_frame_num = (expected_frame_num + 1) % max_frame_num;
    }

    // Update previous frame number for gap detection
    dpb->prev_frame_num = current_frame_num;
}

// Update DPB with new frame info from hardware registers
static dpb_entry_t* amlvdec_h264_dpb_store_pic(dpb_buffer_t *dpb) {
	const struct amlvdec_h264_lmem *hw_dpb = dpb->hw_dpb;

	dpb_entry_t *entry = amlvdec_h264_dpb_alloc_pic(dpb);
	if (entry == NULL) {
		return NULL;
	}

	// Fill entry with current frame info
	entry->frame_num = hw_dpb->dpb.frame_num;
	entry->picture_structure = hw_dpb->dpb.picture_structure_mmco;
	entry->slice_type = dpb->hw_dpb->params.slice_type;
	entry->is_idr = is_idr_pic(hw_dpb);

	// Set buffer management flags
	entry->is_reference = is_reference_slice(dpb->hw_dpb);
	entry->is_used = true;

#if 0
	if (entry->is_idr) {
		uint16_t cmd = amlvdec_h264_mmco_cmd(hw_dpb, 0);
		bool long_term_reference_flag = cmd & 1;
		bool no_output_of_prior_pics_flag = (cmd >> 1) & 1;
	}
#endif

	return entry;
}

// Sort reference list by POC
static void sort_ref_list_by_poc(dpb_entry_t **ref_list, int size, int32_t curr_poc, bool before) {
	// Simple bubble sort
	for (int i = 0; i < size - 1; i++) {
		for (int j = 0; j < size - i - 1; j++) {
			bool should_swap;
			if (before) {
				// For ref_list0: closest POC before current
				should_swap = abs(ref_list[j]->frame_poc - curr_poc) > abs(ref_list[j + 1]->frame_poc - curr_poc);
			} else {
				// For ref_list1: closest POC after current
				should_swap = abs(ref_list[j]->frame_poc - curr_poc) < abs(ref_list[j + 1]->frame_poc - curr_poc);
			}

			if (should_swap) {
				dpb_entry_t *temp = ref_list[j];
				ref_list[j] = ref_list[j + 1];
				ref_list[j + 1] = temp;
			}
		}
	}
}

// Sort reference list by frame number
static void sort_ref_list_by_frame_num(dpb_entry_t **ref_list, int size, uint16_t curr_frame_num) {
#define MAX_FRAME_NUM U16_MAX

	// Simple bubble sort
	for (int i = 0; i < size - 1; i++) {
		for (int j = 0; j < size - i - 1; j++) {
			int diff_j = (curr_frame_num >= ref_list[j]->frame_num) ?
				(curr_frame_num - ref_list[j]->frame_num) :
				(curr_frame_num + MAX_FRAME_NUM - ref_list[j]->frame_num);

			int diff_next = (curr_frame_num >= ref_list[j + 1]->frame_num) ?
				(curr_frame_num - ref_list[j + 1]->frame_num) :
				(curr_frame_num + MAX_FRAME_NUM - ref_list[j + 1]->frame_num);

			if (diff_j > diff_next) {
				dpb_entry_t *temp = ref_list[j];
				ref_list[j] = ref_list[j + 1];
				ref_list[j + 1] = temp;
			}
		}
	}
}

static void amlvdec_h264_dpb_reorder_refs(dpb_buffer_t *dpb, dpb_entry_t *current_pic) {
	int max_frame_num = 1 << dpb->hw_dpb->params.log2_max_frame_num;

	if (current_pic->is_idr) {
		return;
	}

	// Process L0 than L1 reordering	
	for (int ref_list = REF_LIST0; ref_list < NUM_REF_LIST; ref_list++) {
		for (int i = 0; i < MAX_REORDER_COMMANDS + MAX_REORDER_PARAMS; i += 2) {
			uint16_t opcode = amlvdec_h264_reorder_cmd(dpb->hw_dpb, ref_list, i);
			uint16_t param = amlvdec_h264_reorder_cmd(dpb->hw_dpb, ref_list, i + 1);

			if (opcode == REORDER_END) {
				break;
			}

			switch (opcode) {
				case REORDER_SHORT_TERM_DECREASE:
				case REORDER_SHORT_TERM_INCREASE: {
					int pic_num_pred = current_pic->frame_num;
					int abs_diff_pic_num = param + 1;

					if (opcode == REORDER_SHORT_TERM_DECREASE)
						pic_num_pred -= abs_diff_pic_num;
					else
						pic_num_pred += abs_diff_pic_num;

					if (pic_num_pred < 0)
						pic_num_pred += max_frame_num;
					else if (pic_num_pred >= max_frame_num)
						pic_num_pred -= max_frame_num;

					// Find matching picture and move to front of list
					for (int i = 0; i < dpb->ref_list0_size; i++) {
						if (!dpb->ref_list0[i]->is_long_term && dpb->ref_list0[i]->frame_num == pic_num_pred) {
							dpb_entry_t *temp = dpb->ref_list0[i];
							memmove(&dpb->ref_list0[1], &dpb->ref_list0[0], i * sizeof(dpb_entry_t*));
							dpb->ref_list0[0] = temp;
							break;
						}
					}
					break;
				}

				case REORDER_LONG_TERM: {
					// Find long-term picture with matching index and move to front
					for (int i = 0; i < dpb->ref_list0_size; i++) {
						if (dpb->ref_list0[i]->is_long_term && dpb->ref_list0[i]->long_term_frame_idx == param) {
							dpb_entry_t *temp = dpb->ref_list0[i];
							memmove(&dpb->ref_list0[1], &dpb->ref_list0[0], i * sizeof(dpb_entry_t*));
							dpb->ref_list0[0] = temp;
							break;
						}
					}
					break;
				}
			}
		}
	}
}

// Build reference lists for current frame
static void dpb_build_ref_lists(dpb_buffer_t *dpb, dpb_entry_t *entry) { 
	bool is_b_slice = IS_B_SLICE(entry->slice_type);

	memset(dpb->ref_list0, 0, sizeof(dpb->ref_list0));
	memset(dpb->ref_list1, 0, sizeof(dpb->ref_list1));

	if (IS_I_SLICE(entry->slice_type)) {
		dpb->ref_list0_size = 0;
		dpb->ref_list1_size = 0;
		return;
	}

	int ref_list0_max = umin(dpb->hw_dpb->dpb.num_ref_idx_l0_active_minus1 + 1, MAX_REF_LIST_SIZE);
	int ref_list1_max = umin(dpb->hw_dpb->dpb.num_ref_idx_l1_active_minus1 + 1, MAX_REF_LIST_SIZE);

	int ref_list0_idx = 0;
	int ref_list1_idx = 0;
	int ref_list0_st_size = 0;
	int ref_list1_st_size = 0;


	// First handle short term references
	for (int i = 0; i < dpb->max_frames; i++) {
		if (!dpb->frames[i].is_used ||
			!dpb->frames[i].is_reference ||
			dpb->frames[i].is_long_term ||
			!dpb->frames[i].is_decoded)
			continue;

		if (is_b_slice) {
			// For B-slices, sort by POC
			if (dpb->frames[i].frame_poc < entry->frame_poc) {
				dpb->ref_list0[ref_list0_idx++] = &dpb->frames[i];
			} else {
				dpb->ref_list1[ref_list1_idx++] = &dpb->frames[i];
			}
		} else {
			// For P-slices, sort by frame_num difference
			dpb->ref_list0[ref_list0_idx++] = &dpb->frames[i];
		}
	}

	ref_list0_st_size = ref_list0_idx;
	ref_list1_st_size = ref_list1_idx;

	if (is_b_slice) {
		sort_ref_list_by_poc(
				dpb->ref_list0,
				ref_list0_st_size,
				entry->frame_poc, true);
		sort_ref_list_by_poc(
				dpb->ref_list1,
				ref_list1_st_size,
				entry->frame_poc, false);
	} else {
		sort_ref_list_by_frame_num(
				dpb->ref_list0,
				ref_list0_st_size,
				entry->frame_num);
	}

	// Then append long term references to both lists
	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].is_used &&
			dpb->frames[i].is_decoded &&
			dpb->frames[i].is_reference &&
			dpb->frames[i].is_long_term) {

			dpb->ref_list0[ref_list0_idx++] = &dpb->frames[i];

			if (is_b_slice) {
				// For B-slices, add long term to both lists
				dpb->ref_list1[ref_list1_idx++] = &dpb->frames[i];
			}
		}
	}

	if (is_b_slice) {
		// For B-slices long term references, sort by frame_num
		sort_ref_list_by_frame_num(
				&dpb->ref_list0[ref_list0_st_size],
				ref_list0_idx - ref_list0_st_size,
				entry->frame_num);
		sort_ref_list_by_frame_num(
				&dpb->ref_list1[ref_list1_st_size],
				ref_list1_idx - ref_list1_st_size,
				entry->frame_num);
	} else {
		sort_ref_list_by_frame_num(
				&dpb->ref_list0[ref_list0_st_size],
				ref_list0_idx - ref_list0_st_size,
				entry->frame_num);
	}

	if (ref_list0_idx > ref_list0_max) {
		pr_warn("DPB: ref_list0 too large (size=%d, max=%d)\n", ref_list0_idx, ref_list0_max);
		ref_list0_idx = ref_list0_max;
	}
	if (ref_list1_idx > ref_list1_max) {
		pr_warn("DPB: ref_list1 too large (size=%d, max=%d)\n", ref_list1_idx, ref_list1_max);
		ref_list1_idx = ref_list1_max;
	}

	dpb->ref_list0_size = ref_list0_idx;
	dpb->ref_list1_size = ref_list1_idx;
	amlvdec_h264_dpb_reorder_refs(dpb, entry);
}

static void mark_all_unused(dpb_buffer_t *dpb) {
	// Clear all reference pictures
	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].is_used &&
			dpb->frames[i].is_decoded) {

			dpb->frames[i].is_reference = false;
			dpb->frames[i].is_long_term = false;
			if (dpb->frames[i].is_output) {
				dpb->frames[i].is_used = false;
				dpb->frames[i].is_output = false;
			}

			amlvdec_h264_dpb_dump_pic(__func__, &dpb->frames[i]);
		}
	}

	// Handle no_output_of_prior_pics_flag if needed
	// TODO: Implement if required
}

// Mark frames as unused based on MMCO commands
static void mark_short_term_unused(dpb_buffer_t *dpb, uint16_t frame_num) {
	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].is_used && 
			!dpb->frames[i].is_long_term &&
			dpb->frames[i].frame_num == frame_num) {
			dpb->frames[i].is_reference = false;

			amlvdec_h264_dpb_dump_pic(__func__, &dpb->frames[i]);
		}
	}
}

static void mark_long_term_unused(dpb_buffer_t *dpb, uint16_t frame_num) {
	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].is_used && 
			dpb->frames[i].is_long_term &&
			dpb->frames[i].frame_num == frame_num) {
			dpb->frames[i].is_reference = false;
			dpb->frames[i].is_long_term = false;
			dpb->frames[i].long_term_frame_idx = -1;

			amlvdec_h264_dpb_dump_pic(__func__, &dpb->frames[i]);
		}
	}
}

static void mark_long_term_index(dpb_buffer_t *dpb, uint16_t frame_num, int long_term_frame_idx) {
	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].is_used && 
			!dpb->frames[i].is_long_term &&
			dpb->frames[i].frame_num == frame_num) {
			dpb->frames[i].is_reference = true;
			dpb->frames[i].is_long_term = true;
			dpb->frames[i].long_term_frame_idx = long_term_frame_idx;

			amlvdec_h264_dpb_dump_pic(__func__, &dpb->frames[i]);
		}
	}
}

static void mark_long_term_max_index_unused(dpb_buffer_t *dpb, int max_long_term_frame_idx) {
	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].is_used && 
			dpb->frames[i].is_long_term &&
			dpb->frames[i].long_term_frame_idx > max_long_term_frame_idx) {
			dpb->frames[i].is_reference = false;
			dpb->frames[i].is_long_term = false;
			dpb->frames[i].long_term_frame_idx = -1;

			amlvdec_h264_dpb_dump_pic(__func__, &dpb->frames[i]);
		}
	}
}

static void adaptive_memory_management(dpb_buffer_t *dpb, dpb_entry_t *current_pic) {
	const struct amlvdec_h264_lmem *hw_dpb = dpb->hw_dpb;
    int max_frame_num = 1 << dpb->hw_dpb->params.log2_max_frame_num;

	dpb->poc_state.last_mmco_has_reset = false;

	for (int i = 0; i < MAX_MMCO_COMMANDS; i++) {
		uint16_t cmd = amlvdec_h264_mmco_cmd(hw_dpb, i);

		if (cmd == MMCO_END)
			break;

		pr_debug("DPB: MMCO CMD: cmd=%u, ", cmd);

		switch (cmd) {
			case MMCO_SHORT_TERM_UNUSED: {
				int difference_of_pic_nums_minus1 = amlvdec_h264_mmco_cmd(hw_dpb, i++);
				pr_cont("difference_of_pic_nums_minus1=%u\n", difference_of_pic_nums_minus1);

				int pic_num_x = current_pic->frame_num - (difference_of_pic_nums_minus1 + 1);
				if (pic_num_x < 0)
					pic_num_x += max_frame_num;

				mark_short_term_unused(dpb, pic_num_x);
				break;
			}
			
			case MMCO_LONG_TERM_UNUSED: {
				int long_term_pic_num = amlvdec_h264_mmco_cmd(hw_dpb, i++);
				pr_cont("long_term_pic_num=%u\n", long_term_pic_num);

				mark_long_term_unused(dpb, long_term_pic_num);
				break;
			}
			
			case MMCO_ASSIGN_LONG_TERM: {
				int difference_of_pic_nums_minus1 = amlvdec_h264_mmco_cmd(hw_dpb, i++);
				int long_term_frame_idx = amlvdec_h264_mmco_cmd(hw_dpb, i++);
				pr_cont("difference_of_pic_nums_minus1=%u, long_term_frame_idx=%u\n", difference_of_pic_nums_minus1, long_term_frame_idx);

				int pic_num_x = current_pic->frame_num - (difference_of_pic_nums_minus1 + 1);
				if (pic_num_x < 0)
					pic_num_x += max_frame_num;
				
				mark_long_term_index(dpb, pic_num_x, long_term_frame_idx);	
				break;
			}
			
			case MMCO_MAX_LONG_TERM_IDX: {
				int max_long_term_frame_idx_plus1 = amlvdec_h264_mmco_cmd(hw_dpb, i++);
				pr_cont("max_long_term_frame_idx_plus1=%u\n", max_long_term_frame_idx_plus1);

				mark_long_term_max_index_unused(dpb, max_long_term_frame_idx_plus1 - 1);
				break;
			}

			case MMCO_RESET_ALL: {
				dpb->poc_state.last_mmco_has_reset = true;
				pr_cont("last_mmco_has_reset=%u\n", dpb->poc_state.last_mmco_has_reset);

				mark_all_unused(dpb);
				break;
			}

			case MMCO_CURRENT_LONG_TERM: {
				int long_term_frame_idx = amlvdec_h264_mmco_cmd(hw_dpb, i++);
				pr_cont("long_term_frame_idx=%u\n", long_term_frame_idx);

				current_pic->is_long_term = true;
				current_pic->long_term_frame_idx = long_term_frame_idx;
				amlvdec_h264_dpb_dump_pic(__func__, current_pic);
				break;
			}

			default:
				pr_cont("\n");
				pr_warn("DPB: MMCO CMD: Invalid cmd=%u\n", cmd);
				break;
		}
	}
}

static void sliding_window_reference(dpb_buffer_t *dpb) {
	int max_num_refs = dpb->hw_dpb->params.max_reference_frame_num;
	int num_ref_frames = 0;
	int oldest_ref_idx = -1;
	int oldest_ref_num = INT_MAX;

	// Count reference frames and find oldest one
	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].is_used &&
			dpb->frames[i].is_reference &&
			!dpb->frames[i].is_long_term &&
			!dpb->frames[i].is_output &&
			dpb->frames[i].is_decoded) {
			num_ref_frames++;
			if (dpb->frames[i].frame_num < oldest_ref_num) {
				oldest_ref_num = dpb->frames[i].frame_num;
				oldest_ref_idx = i;
			}
		}
	}

	// If we've reached the maximum, remove oldest reference
	if (num_ref_frames > max_num_refs &&
		oldest_ref_idx >= 0) {
		dpb->frames[oldest_ref_idx].is_reference = false;

		amlvdec_h264_dpb_dump_pic(__func__, &dpb->frames[oldest_ref_idx]);
	}
}

static void sliding_window_reference(dpb_buffer_t *dpb);

dpb_entry_t* amlvdec_h264_dpb_new_pic(dpb_buffer_t *dpb) {
	dpb_entry_t* entry;

	// Store new frame in DPB
	entry = amlvdec_h264_dpb_store_pic(dpb);
	if (entry == NULL) {
		return NULL;
	}

	if (entry->is_idr) {
		mark_all_unused(dpb);
	} else {
		adaptive_memory_management(dpb, entry);
		sliding_window_reference(dpb);
	}

#if 0
    // Handle frame gaps if any
    amlvdec_h264_dpb_fill_gaps(dpb, entry->frame_num);
#endif

	amlvdec_h264_dpb_decode_poc(
			dpb->hw_dpb,
			&dpb->poc_state,
			entry);

	// Build reference lists
	dpb_build_ref_lists(dpb, entry);

	amlvdec_h264_dpb_dump_pic(__func__, entry);

	return entry;
}

void amlvdec_h264_dpb_pic_done(dpb_buffer_t *dpb, dpb_entry_t *current_pic) {
	current_pic->is_decoded = true;
}

void amlvdec_h264_dpb_recycle_pic(dpb_buffer_t *dpb, dpb_entry_t *pic) {
	pic->is_used = false;
	pic->is_reference = false;
	pic->is_long_term = false;
	pic->long_term_frame_idx = -1;
	pic->is_output = false;
	pic->is_decoded = false;
	pic->is_gap_frame = false;
}

int amlvdec_h264_dpb_flush_output(dpb_buffer_t *dpb, bool flush_all, output_pic_callback output_pic) {
	int count = 0;

	for (int i = 0; i < dpb->max_frames; i++) {
		dpb_entry_t *pic = &dpb->frames[i];

		if (pic->is_used &&
			!pic->is_output &&
			(flush_all || (
			dpb->frames[i].is_decoded &&
			!pic->is_reference &&
			!pic->is_long_term))) {

			if (output_pic) {
				output_pic(dpb, pic);
			}
			if (pic->is_output) {
				amlvdec_h264_dpb_recycle_pic(dpb, pic);
			}
			count++;
		}
	}

	return count;
}

void amlvdec_h264_dpb_dump(dpb_buffer_t *dpb) {
	for (int i = 0; i < dpb->max_frames; i++) {
		amlvdec_h264_dpb_dump_pic(__func__, &dpb->frames[i]);
	}
}
