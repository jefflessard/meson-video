#include "amlvdec_h264_dpb.h"

#define MAX_FRAME_NUM U16_MAX

static void amlvdec_h264_decode_poc(const struct amlvdec_h264_lmem *lmem, poc_state_t *poc, dpb_entry_t *entry)
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

	int ThisPOC = 0;

	switch (params->pic_order_cnt_type) {
		case 0: /* POC MODE 0 */
			// Reset POC MSB and LSB for IDR pictures or after MMCO5
			if (is_idr_pic(lmem)) {
				poc->PrevPicOrderCntMsb = 0;
				poc->PrevPicOrderCntLsb = 0;
			} else if (poc->last_has_mmco_5) {
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
			if (params->nal_ref_idc) {
				poc->PrevPicOrderCntLsb = pic_order_cnt_lsb;
				poc->PrevPicOrderCntMsb = poc->PicOrderCntMsb;
			}
			break;

		case 1: /* POC MODE 1 */
			// Calculate FrameNumOffset
			if (is_idr_pic(lmem)) {
				poc->FrameNumOffset = 0;
			} else {
				if (poc->last_has_mmco_5) {
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

			if ((!params->nal_ref_idc) && poc->AbsFrameNum > 0)
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

			if (!params->nal_ref_idc)
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
			if (is_idr_pic(lmem)) {
				poc->FrameNumOffset = 0;
				ThisPOC = entry->frame_poc = entry->top_poc = entry->bottom_poc = 0;
			} else {
				if (poc->last_has_mmco_5) {
					poc->PreviousFrameNum = 0;
					poc->PreviousFrameNumOffset = 0;
				}
				if (frame_num < poc->PreviousFrameNum)
					poc->FrameNumOffset = poc->PreviousFrameNumOffset + max_frame_num;
				else
					poc->FrameNumOffset = poc->PreviousFrameNumOffset;

				poc->AbsFrameNum = poc->FrameNumOffset + frame_num;
				ThisPOC = (!params->nal_ref_idc) ? 
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

// Find empty slot in DPB
static int find_empty_dpb_slot(dpb_buffer_t *dpb) {
	for (int i = 0; i < dpb->max_frames; i++) {
		if (!dpb->frames[i].is_used) {
			return i;
		}
	}
	return -1;
}

// Find a frame that can be removed from DPB
static int find_removable_frame(dpb_buffer_t *dpb) {
	int oldest_poc = INT_MAX;
	int oldest_idx = -1;

	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].is_used && !dpb->frames[i].is_reference) {
			if (dpb->frames[i].frame_poc < oldest_poc) {
				oldest_poc = dpb->frames[i].frame_poc;
				oldest_idx = i;
			}
		}
	}
	return oldest_idx;
}


// Mark frames as unused based on MMCO commands
static void dpb_mark_short_term_unused(dpb_buffer_t *dpb, uint16_t frame_num) {
	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].is_used && 
			!dpb->frames[i].is_long_term &&
			dpb->frames[i].frame_num == frame_num) {
			dpb->frames[i].is_reference = false;
		}
	}
}

static void dpb_mark_long_term_unused(dpb_buffer_t *dpb, uint16_t long_term_pic_num) {
	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].is_used && 
			dpb->frames[i].is_long_term &&
			dpb->frames[i].frame_num == long_term_pic_num) {
			dpb->frames[i].is_reference = false;
			dpb->frames[i].is_long_term = false;
		}
	}
}

// Update DPB with new frame info from hardware registers
dpb_entry_t* dpb_store_frame(dpb_buffer_t *dpb) {
	const struct amlvdec_h264_lmem *hw_dpb = dpb->hw_dpb;

	// Find or make space in DPB
	int slot = find_empty_dpb_slot(dpb);
	if (slot < 0) {
		slot = find_removable_frame(dpb);
		if (slot < 0) {
			pr_err("DPB: DPB full with reference frames");
			// DPB full with reference frames - error condition
			return NULL;
		}
	}

	dpb_entry_t *entry = &dpb->frames[slot];

	// Fill entry with current frame info
	entry->frame_num = hw_dpb->dpb.frame_num;
	entry->pic_order_cnt_lsb = hw_dpb->dpb.pic_order_cnt_lsb;
	entry->picture_structure = hw_dpb->dpb.picture_structure_mmco;

	// Extract NAL info
	entry->nal_ref_idc = hw_dpb->dpb.nal_info_mmco.nal_ref_idc;
	entry->nal_unit_type = hw_dpb->dpb.nal_info_mmco.nal_unit_type;

#if 0
	// Store POC information
	entry->frame_poc = (hw_dpb->dpb.frame_pic_order_cnt_hi << 16) | hw_dpb->dpb.frame_pic_order_cnt_lo;
	entry->top_poc = (hw_dpb->dpb.top_field_pic_order_cnt_hi << 16) | hw_dpb->dpb.top_field_pic_order_cnt_lo;
	entry->bottom_poc = entry->frame_poc - entry->top_poc;
#endif

	// Set buffer management flags
	entry->is_reference = (entry->nal_ref_idc != 0);
	entry->is_used = true;

	bool is_idr = is_idr_pic(hw_dpb);
	if (is_idr) {
		uint16_t cmd = amlvdec_h264_mmco_cmd(hw_dpb, 0);
		bool long_term_reference_flag = cmd & 1;
		bool no_output_of_prior_pics_flag = (cmd >> 1) & 1;
	}

	bool adaptive_ref_pic_buffering_flag = false;

	dpb->poc_state.last_has_mmco_5 = false;

	if (entry->is_reference) {
		// Process MMCO commands if present
		// TODO lmem swapped
		for (int i = 0; i < 44; i++) {
			uint16_t cmd = amlvdec_h264_mmco_cmd(hw_dpb, i);

			if (cmd == 0)
				break;

			adaptive_ref_pic_buffering_flag = true;

			int difference_of_pic_nums_minus1 = 0;
			int long_term_pic_num = 0;
			int long_term_frame_idx = 0;
			int max_long_term_frame_idx_plus1 = 0;

			pr_debug("DPB MMCO CMD: cmd=%u, ", cmd);
			switch (cmd) {
				case 1:
					difference_of_pic_nums_minus1 = amlvdec_h264_mmco_cmd(hw_dpb, i++);
					pr_cont("difference_of_pic_nums_minus1=%u\n", difference_of_pic_nums_minus1);
					// TODO mm_unmark_short_term_for_reference
					dpb_mark_short_term_unused(dpb,
							entry->frame_num - (difference_of_pic_nums_minus1 + 1));
					break;
				case 2:
					long_term_pic_num = amlvdec_h264_mmco_cmd(hw_dpb, i++);
					pr_cont("long_term_pic_num=%u\n", long_term_pic_num);
					// TODO mm_unmark_long_term_for_reference
					dpb_mark_long_term_unused(dpb, long_term_pic_num);
					break;
				case 3:
					difference_of_pic_nums_minus1 = amlvdec_h264_mmco_cmd(hw_dpb, i++);
					long_term_frame_idx = amlvdec_h264_mmco_cmd(hw_dpb, i++);
					pr_cont("difference_of_pic_nums_minus1=%u, long_term_frame_idx=%u\n", difference_of_pic_nums_minus1, long_term_frame_idx);
					// TODO mm_assign_long_term_frame_idx
					if (entry->is_reference) {
						entry->is_long_term = true;
					}
					break;
				case 4:
					max_long_term_frame_idx_plus1 = amlvdec_h264_mmco_cmd(hw_dpb, i++);
					pr_cont("max_long_term_frame_idx_plus1=%u\n", max_long_term_frame_idx_plus1);
					// TODO mm_update_max_long_term_frame_idx
					break;
				case 5:
					dpb->poc_state.last_has_mmco_5 = true;
					pr_cont("last_has_mmco_5=%u\n", dpb->poc_state.last_has_mmco_5);
					// TODO mm_unmark_all_short_term_for_reference
					// TODO mm_unmark_all_long_term_for_reference
					break;
				case 6:
					long_term_frame_idx = amlvdec_h264_mmco_cmd(hw_dpb, i++);
					pr_cont("long_term_frame_idx=%u\n", long_term_frame_idx);
					// TODO mm_mark_current_picture_long_term
					break;
				default:
					pr_cont("\n");
					pr_warn("DPB MMCO CMD: Invalid cmd=%u\n", cmd);
					break;
			}
		}
	}

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

// Build reference lists for current frame
void dpb_build_ref_lists(dpb_buffer_t *dpb, 
		uint16_t curr_frame_num,
		int32_t curr_poc,
		bool is_b_slice) {
	dpb->ref_list0_size = 0;
	dpb->ref_list1_size = 0;

	// First handle short term references
	for (int i = 0; i < dpb->max_frames; i++) {
		if (!dpb->frames[i].is_used ||
			!dpb->frames[i].is_reference ||
			dpb->frames[i].is_long_term)
			continue;

		if (is_b_slice) {
			// For B-slices, sort by POC
			if (dpb->frames[i].frame_poc < curr_poc) {
				dpb->ref_list0[dpb->ref_list0_size++] = &dpb->frames[i];
			} else {
				dpb->ref_list1[dpb->ref_list1_size++] = &dpb->frames[i];
			}
		} else {
			// For P-slices, sort by frame_num difference
			dpb->ref_list0[dpb->ref_list0_size++] = &dpb->frames[i];
		}
	}

	// Then append long term references to both lists
	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].is_used &&
			dpb->frames[i].is_reference &&
			dpb->frames[i].is_long_term) {
			if (dpb->ref_list0_size < MAX_REF_FRAMES) {
				dpb->ref_list0[dpb->ref_list0_size++] = &dpb->frames[i];
			}
			if (is_b_slice && dpb->ref_list1_size < MAX_REF_FRAMES) {
				dpb->ref_list1[dpb->ref_list1_size++] = &dpb->frames[i];
			}
		}
	}

	// Sort reference lists
	if (is_b_slice) {
		sort_ref_list_by_poc(dpb->ref_list0, dpb->ref_list0_size, curr_poc, true);
		sort_ref_list_by_poc(dpb->ref_list1, dpb->ref_list1_size, curr_poc, false);
	} else {
		sort_ref_list_by_frame_num(dpb->ref_list0, dpb->ref_list0_size, curr_frame_num);
	}
}

dpb_entry_t* amlvdec_h264_dpb_frame(dpb_buffer_t *dpb) {
	dpb->max_frames = umin(dpb->hw_dpb->params.max_reference_frame_num, MAX_DPB_FRAMES);

	// 1. Store new frame in DPB
	dpb_entry_t* entry = dpb_store_frame(dpb);
	if (entry == NULL) {
		return NULL;
	}

	amlvdec_h264_decode_poc(
			dpb->hw_dpb,
			&dpb->poc_state,
			entry);

	// 3. Build reference lists
	bool is_b_slice = IS_B_SLICE(dpb->hw_dpb->params.slice_type);

	dpb_build_ref_lists(
			dpb,
			entry->frame_num,
			entry->frame_poc,
			is_b_slice);

	return entry;

// TODO configure buffers in decoder
/*

   amlvdec_h264_dpb_process_frame();

	// 2. Configure buffer info registers
	configure_buffer_info(dpb, current_buf_idx, mb_aff_frame_flag);

	// 4. Configure reference buffers
	configure_reference_buffers(dpb->ref_list0, dpb->ref_list0_size, dpb->ref_list1, dpb->ref_list1_size);

	// 5. For B-slices, configure colocated buffer
	if (is_b_slice) {
		dpb_entry_t *colocated = dpb_get_colocated_ref(dpb);
		if (colocated) {
			configure_colocated_buffers(
					dpb->current_buf,
					colocated,
					colocated_buf_size,
					mb_aff_frame_flag,
					hw_dpb->first_mb_in_slice
					);
		}
	}
*/
}
