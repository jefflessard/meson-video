#include <linux/sort.h>

#include "amlvdec_h264_dpb.h"

static int compare_by_poc_asc(const void *va, const void *vb) {
	const dpb_picture_t *a = *(const dpb_picture_t **) va;
	const dpb_picture_t *b = *(const dpb_picture_t **) vb;

	return ((int) a->output_order_key - (int) b->output_order_key);
}

static int compare_by_poc_desc(const void *va, const void *vb) {
	return compare_by_poc_asc(vb, va);
}

static int compare_by_frame_num_asc(const void *va, const void *vb) {
	const dpb_picture_t *a = *(const dpb_picture_t **) va;
	const dpb_picture_t *b = *(const dpb_picture_t **) vb;

	return ((int) a->frame_num - (int) b->frame_num) |
		((int) a->coded_sequence - (int) b->coded_sequence) << 16;
}

static int compare_by_frame_num_desc(const void *va, const void *vb) {
	return compare_by_frame_num_asc(vb, va);
}

static int compare_by_long_term_idx_asc(const void *va, const void *vb) {
	const dpb_picture_t *a = *(const dpb_picture_t **) va;
	const dpb_picture_t *b = *(const dpb_picture_t **) vb;

	return (a->long_term_frame_idx - b->long_term_frame_idx) |
		((int) a->coded_sequence - (int) b->coded_sequence) << 16;
}

static inline void amlvdec_h264_dpb_dump_pic(const char *prefix, dpb_picture_t *pic) {
	pr_debug("DPB: %s: frame=%d, idr=%d, seq=%d, poc=%d (%d, %d) index=%d, state=%d, ref=%d\n",
			prefix,
			pic->frame_num,
			pic->is_idr,
			pic->coded_sequence,
			pic->frame_poc,
			pic->top_poc,
			pic->bottom_poc,
			pic->buffer_index,
			pic->state,
			pic->ref_type);
}

static void amlvdec_h264_dpb_decode_poc(const struct amlvdec_h264_lmem *lmem, poc_state_t *poc, dpb_picture_t *pic)
{
	const struct amlvdec_h264_rpm *params = &lmem->params;
	const struct amlvdec_h264_dpb *dpb = &lmem->dpb;
	const struct amlvdec_h264_mmco *mmco = &lmem->mmco;

	const bool field_pic_flag =
		pic->picture_structure == MMCO_TOP_FIELD ||
		pic->picture_structure == MMCO_BOTTOM_FIELD;
	const bool bottom_field_flag =
		pic->picture_structure == MMCO_BOTTOM_FIELD;
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
	const bool is_reference = pic->ref_type != DPB_NON_REF;

	int ThisPOC = 0;

	switch (params->pic_order_cnt_type) {
		case 0: /* POC MODE 0 */
			// Reset POC MSB and LSB for IDR pictures or after MMCO5
			if (pic->is_idr) {
				poc->PrevPicOrderCntMsb = 0;
				poc->PrevPicOrderCntLsb = 0;
			} else if (poc->last_mmco_has_reset) {
				if (poc->last_pic_bottom_field) {
					poc->PrevPicOrderCntMsb = 0;
					poc->PrevPicOrderCntLsb = 0;
				} else {
					poc->PrevPicOrderCntMsb = 0;
					poc->PrevPicOrderCntLsb = pic->top_poc;
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
				pic->top_poc = poc->PicOrderCntMsb + pic_order_cnt_lsb;
				pic->bottom_poc = pic->top_poc + delta_pic_order_cnt_bottom;
				ThisPOC = pic->frame_poc = 
					(pic->top_poc < pic->bottom_poc) ? pic->top_poc : pic->bottom_poc;
			} else if (!bottom_field_flag) {
				ThisPOC = pic->top_poc = 
					poc->PicOrderCntMsb + pic_order_cnt_lsb;
			} else {
				ThisPOC = pic->bottom_poc = 
					poc->PicOrderCntMsb + pic_order_cnt_lsb;
			}
			pic->frame_poc = ThisPOC;

			// Update previous values for reference pictures
			poc->PreviousFrameNum = frame_num;
			if (is_reference) {
				poc->PrevPicOrderCntLsb = pic_order_cnt_lsb;
				poc->PrevPicOrderCntMsb = poc->PicOrderCntMsb;
			}
			break;

		case 1: /* POC MODE 1 */
			// Calculate FrameNumOffset
			if (pic->is_idr) {
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
				pic->top_poc = poc->ExpectedPicOrderCnt + delta_pic_order_cnt_0;
				pic->bottom_poc = pic->top_poc + params->offset_for_top_to_bottom_field + delta_pic_order_cnt_1;
				ThisPOC = pic->frame_poc = 
					(pic->top_poc < pic->bottom_poc) ? pic->top_poc : pic->bottom_poc;
			} else if (!bottom_field_flag) {
				ThisPOC = pic->top_poc = 
					poc->ExpectedPicOrderCnt + delta_pic_order_cnt_0;
			} else {
				ThisPOC = pic->bottom_poc = poc->ExpectedPicOrderCnt + 
					params->offset_for_top_to_bottom_field + delta_pic_order_cnt_0;
			}
			pic->frame_poc = ThisPOC;

			// Update previous values
			poc->PreviousFrameNum = frame_num;
			poc->PreviousFrameNumOffset = poc->FrameNumOffset;
			break;

		case 2: /* POC MODE 2 */
			if (pic->is_idr) {
				poc->FrameNumOffset = 0;
				ThisPOC = pic->frame_poc = pic->top_poc = pic->bottom_poc = 0;
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
					pic->top_poc = pic->bottom_poc = pic->frame_poc = ThisPOC;
				else if (!bottom_field_flag)
					pic->top_poc = pic->frame_poc = ThisPOC;
				else
					pic->bottom_poc = pic->frame_poc = ThisPOC;
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
}

int amlvdec_h264_dpb_adjust_size(dpb_buffer_t *dpb) {
	int new_size = 0;

	new_size += dpb->hw_dpb->params.max_reference_frame_num;
	new_size += dpb->hw_dpb->params.num_reorder_frames;
	new_size += dpb->hw_dpb->params.max_buffer_frame;
	new_size += DPB_SIZE_MARGIN;

	new_size = umin(new_size, MAX_DPB_FRAMES);
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

static dpb_picture_t* amlvdec_h264_dpb_alloc_pic(dpb_buffer_t *dpb) {

	// Count reference frames and find oldest one
	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].state == DPB_UNUSED) {
			return &dpb->frames[i];
		}
	}

	// DPB full with reference frames - error condition
	pr_err("DPB: DPB full with reference frames");
	return NULL;
}

// Update DPB with new frame info from hardware registers
static dpb_picture_t* amlvdec_h264_dpb_store_pic(dpb_buffer_t *dpb) {
	const struct amlvdec_h264_lmem *hw_dpb = dpb->hw_dpb;

	dpb_picture_t *pic = amlvdec_h264_dpb_alloc_pic(dpb);
	if (pic == NULL) {
		return NULL;
	}

	// Fill pic with current frame info
	pic->frame_num = hw_dpb->dpb.frame_num;
	pic->picture_structure = hw_dpb->dpb.picture_structure_mmco;
	pic->slice_type = dpb->hw_dpb->params.slice_type;
	pic->is_idr = is_idr_pic(hw_dpb);

	// Set buffer management flags
	pic->state = DPB_INIT;
	if (is_reference_slice(dpb->hw_dpb))
		pic->ref_type = DPB_SHORT_TERM;

	pic->is_output = true;

	return pic;
}

bool amlvdec_h264_dpb_is_matching_field(dpb_buffer_t *dpb, dpb_picture_t *pic) {
	uint16_t frame_num = dpb->hw_dpb->dpb.frame_num;
	enum picture_structure_mmco structure = dpb->hw_dpb->dpb.picture_structure_mmco;

	if (pic != NULL &&
		pic->frame_num == frame_num &&
		pic->coded_sequence == dpb->current_sequence &&
		((pic->picture_structure == MMCO_TOP_FIELD &&
			structure == MMCO_BOTTOM_FIELD) ||
		 (pic->picture_structure == MMCO_BOTTOM_FIELD &&
			structure == MMCO_TOP_FIELD))) {

		return true;
	}

	return false;
}

static void amlvdec_h264_dpb_reorder_refs(dpb_buffer_t *dpb, dpb_picture_t *pic) {
	int max_frame_num = 1 << dpb->hw_dpb->params.log2_max_frame_num;

	if (pic->is_idr) {
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
					int pic_num_pred = pic->frame_num;
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
						if (dpb->ref_list0[i]->ref_type == DPB_SHORT_TERM &&
							dpb->ref_list0[i]->frame_num == pic_num_pred) {
							dpb_picture_t *temp = dpb->ref_list0[i];
							memmove(&dpb->ref_list0[1], &dpb->ref_list0[0], i * sizeof(dpb_picture_t*));
							dpb->ref_list0[0] = temp;
							break;
						}
					}
					break;
				}

				case REORDER_LONG_TERM: {
					// Find long-term picture with matching index and move to front
					for (int i = 0; i < dpb->ref_list0_size; i++) {
						if (dpb->ref_list0[i]->ref_type == DPB_LONG_TERM &&
							dpb->ref_list0[i]->long_term_frame_idx == param) {
							dpb_picture_t *temp = dpb->ref_list0[i];
							memmove(&dpb->ref_list0[1], &dpb->ref_list0[0], i * sizeof(dpb_picture_t*));
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
static void dpb_build_ref_lists(dpb_buffer_t *dpb, dpb_picture_t *pic) { 
	bool is_b_slice = IS_B_SLICE(pic->slice_type);

	memset(dpb->ref_list0, 0, sizeof(dpb->ref_list0));
	memset(dpb->ref_list1, 0, sizeof(dpb->ref_list1));

	if (IS_I_SLICE(pic->slice_type)) {
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
		if (dpb->frames[i].state != DPB_REF_IN_USE ||
			dpb->frames[i].ref_type != DPB_SHORT_TERM)
			continue;

		if (is_b_slice) {
			// For B-slices, sort by POC
			if (dpb->frames[i].frame_poc < pic->frame_poc) {
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
		sort(dpb->ref_list0,
				ref_list0_st_size,
				sizeof(dpb_picture_t *),
				compare_by_poc_desc, NULL);
		sort(dpb->ref_list1,
				ref_list1_st_size,
				sizeof(dpb_picture_t *),
				compare_by_poc_asc, NULL);
	} else {
		sort(dpb->ref_list0,
				ref_list0_st_size,
				sizeof(dpb_picture_t *),
				compare_by_frame_num_desc, NULL);
	}

	// Then append long term references to both lists
	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].state == DPB_REF_IN_USE &&
			dpb->frames[i].ref_type == DPB_LONG_TERM) {

			dpb->ref_list0[ref_list0_idx++] = &dpb->frames[i];

			if (is_b_slice) {
				// For B-slices, add long term to both lists
				dpb->ref_list1[ref_list1_idx++] = &dpb->frames[i];
			}
		}
	}

	if (is_b_slice) {
		// For B-slices long term references, sort by frame_num
		sort(&dpb->ref_list0[ref_list0_st_size],
				ref_list0_idx - ref_list0_st_size,
				sizeof(dpb_picture_t *),
				compare_by_long_term_idx_asc, NULL);
		sort(&dpb->ref_list1[ref_list1_st_size],
				ref_list1_idx - ref_list1_st_size,
				sizeof(dpb_picture_t *),
				compare_by_long_term_idx_asc, NULL);
	} else {
		sort(&dpb->ref_list0[ref_list0_st_size],
				ref_list0_idx - ref_list0_st_size,
				sizeof(dpb_picture_t *),
				compare_by_long_term_idx_asc, NULL);
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
	amlvdec_h264_dpb_reorder_refs(dpb, pic);
}

static void mark_all_unused(dpb_buffer_t *dpb) {
	// Clear all reference pictures
	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].state == DPB_REF_IN_USE) {
			dpb->frames[i].state = DPB_OUTPUT_READY;
			dpb->frames[i].long_term_frame_idx = -1;

			amlvdec_h264_dpb_dump_pic(__func__, &dpb->frames[i]);
		}
	}

	// Handle no_output_of_prior_pics_flag if needed
	// TODO: Implement if required
}

// Mark frames as unused based on MMCO commands
static void mark_short_term_unused(dpb_buffer_t *dpb, uint16_t frame_num) {
	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].state == DPB_REF_IN_USE &&
			dpb->frames[i].ref_type == DPB_SHORT_TERM && 
			dpb->frames[i].coded_sequence == dpb->current_sequence &&
			dpb->frames[i].frame_num == frame_num) {
			dpb->frames[i].state = DPB_OUTPUT_READY;

			amlvdec_h264_dpb_dump_pic(__func__, &dpb->frames[i]);
		}
	}
}

static void mark_long_term_unused(dpb_buffer_t *dpb, uint16_t frame_num) {
	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].state == DPB_REF_IN_USE &&
			dpb->frames[i].ref_type == DPB_LONG_TERM && 
			dpb->frames[i].coded_sequence == dpb->current_sequence &&
			dpb->frames[i].frame_num == frame_num) {
			dpb->frames[i].state = DPB_OUTPUT_READY;
			dpb->frames[i].long_term_frame_idx = -1;

			amlvdec_h264_dpb_dump_pic(__func__, &dpb->frames[i]);
		}
	}
}

static void mark_long_term_index(dpb_buffer_t *dpb, uint16_t frame_num, int long_term_frame_idx) {
	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].coded_sequence == dpb->current_sequence &&
			dpb->frames[i].frame_num == frame_num) {
			dpb->frames[i].state = DPB_REF_IN_USE;
			dpb->frames[i].ref_type = DPB_LONG_TERM;
			dpb->frames[i].long_term_frame_idx = long_term_frame_idx;

			amlvdec_h264_dpb_dump_pic(__func__, &dpb->frames[i]);
		}
	}
}

static void mark_long_term_max_index_unused(dpb_buffer_t *dpb, int max_long_term_frame_idx) {
	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].state == DPB_REF_IN_USE && 
			dpb->frames[i].ref_type == DPB_LONG_TERM &&
			dpb->frames[i].coded_sequence == dpb->current_sequence &&
			dpb->frames[i].long_term_frame_idx > max_long_term_frame_idx) {
			dpb->frames[i].state = DPB_OUTPUT_READY;
			dpb->frames[i].long_term_frame_idx = -1;

			amlvdec_h264_dpb_dump_pic(__func__, &dpb->frames[i]);
		}
	}
}

static void idr_memory_management(dpb_buffer_t *dpb, dpb_picture_t *pic) {
	uint16_t cmd = amlvdec_h264_mmco_cmd(dpb->hw_dpb, 0);
	bool long_term_reference_flag = cmd & 1;
	// TODO bool no_output_of_prior_pics_flag = (cmd >> 1) & 1;

	mark_all_unused(dpb);

	if (long_term_reference_flag) {
		pic->ref_type = DPB_LONG_TERM;
		pic->long_term_frame_idx = 0;
	}
}

static bool adaptive_memory_management(dpb_buffer_t *dpb, dpb_picture_t *pic) {
	const struct amlvdec_h264_lmem *hw_dpb = dpb->hw_dpb;
    int max_frame_num = 1 << dpb->hw_dpb->params.log2_max_frame_num;
	bool has_mmco = false;

	dpb->poc_state.last_mmco_has_reset = false;

	for (int i = 0; i < MAX_MMCO_COMMANDS; i++) {
		uint16_t cmd = amlvdec_h264_mmco_cmd(hw_dpb, i);

		if (cmd == MMCO_END)
			break;

		has_mmco = true;

		pr_debug("DPB: MMCO CMD: cmd=%u, ", cmd);

		switch (cmd) {
			case MMCO_SHORT_TERM_UNUSED: {
				int difference_of_pic_nums_minus1 = amlvdec_h264_mmco_cmd(hw_dpb, i++);
				pr_cont("difference_of_pic_nums_minus1=%u\n", difference_of_pic_nums_minus1);

				int pic_num_x = pic->frame_num - (difference_of_pic_nums_minus1 + 1);
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

				int pic_num_x = pic->frame_num - (difference_of_pic_nums_minus1 + 1);
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

				pic->state = DPB_REF_IN_USE;
				pic->ref_type = DPB_LONG_TERM;
				pic->long_term_frame_idx = long_term_frame_idx;
				amlvdec_h264_dpb_dump_pic(__func__, pic);
				break;
			}

			default:
				pr_cont("\n");
				pr_warn("DPB: MMCO CMD: Invalid cmd=%u\n", cmd);
				break;
		}
	}

	return has_mmco;
}

static void sliding_window_reference(dpb_buffer_t *dpb) {
	int max_num_refs = dpb->hw_dpb->params.max_reference_frame_num;
	int num_ref_frames = 0;
	int oldest_idx = -1;
	uint32_t oldest_key = U32_MAX;

	// Count reference frames and find oldest one
	for (int i = 0; i < dpb->max_frames; i++) {
		if (dpb->frames[i].state == DPB_REF_IN_USE &&
			dpb->frames[i].ref_type == DPB_SHORT_TERM) {
			num_ref_frames++;
			if (dpb->frames[i].output_order_key < oldest_key) {
				oldest_key = dpb->frames[i].output_order_key;
				oldest_idx = i;
			}
		}
	}

	// If we've reached the maximum, remove oldest reference
	if (num_ref_frames > max_num_refs &&
		oldest_idx >= 0) {
		dpb->frames[oldest_idx].state = DPB_OUTPUT_READY;

		amlvdec_h264_dpb_dump_pic(__func__, &dpb->frames[oldest_idx]);
	}
}

dpb_picture_t* amlvdec_h264_dpb_new_pic(dpb_buffer_t *dpb) {
	dpb_picture_t* pic;

	// Store new frame in DPB
	pic = amlvdec_h264_dpb_store_pic(dpb);
	if (pic == NULL) {
		return NULL;
	}

	if (pic->is_idr) {
		idr_memory_management(dpb, pic);
		dpb->current_sequence++;
	} else {
		adaptive_memory_management(dpb, pic);
	}

	amlvdec_h264_dpb_decode_poc(
			dpb->hw_dpb,
			&dpb->poc_state,
			pic);

	pic->coded_sequence = dpb->current_sequence;
	pic->output_order_key = ((uint32_t) pic->coded_sequence) << 16 | pic->frame_poc;

	// Build reference lists
	dpb_build_ref_lists(dpb, pic);

	amlvdec_h264_dpb_dump_pic(__func__, pic);

	return pic;
}

void amlvdec_h264_dpb_pic_done(dpb_buffer_t *dpb, dpb_picture_t *pic) {
	if (pic->ref_type == DPB_NON_REF) {
		pic->state = DPB_OUTPUT_READY;
	} else {
		pic->state = DPB_REF_IN_USE;
	}

	sliding_window_reference(dpb);
}

void amlvdec_h264_dpb_recycle_pic(dpb_buffer_t *dpb, dpb_picture_t *pic) {
	pic->state = DPB_UNUSED;
	pic->ref_type = DPB_NON_REF;
	pic->long_term_frame_idx = -1;
	pic->frame_num = 0;
	pic->top_poc = 0;
	pic->bottom_poc = 0;
	pic->frame_poc = 0;
	pic->coded_sequence = 0;
	pic->output_order_key = 0;
	pic->is_output = false;
	amlvdec_h264_dpb_dump_pic(__func__, pic);
}

int amlvdec_h264_dpb_flush_output(dpb_buffer_t *dpb, bool flush_all, output_pic_callback output_pic) {
    int output_count = 0;
	bool output_found;

    do {
		output_found = false;

        // Find the next picture that can be safely output
        for (int i = 0; i < dpb->max_frames; i++) {
            dpb_picture_t *pic = &dpb->frames[i];

            // Skip pictures that are not ready for output
            if ((flush_all && pic->state == DPB_UNUSED) ||
				(!flush_all && pic->state != DPB_OUTPUT_READY)) {
                continue;
            }

            // Check if there are any other DPB pictures with a lower POC
            bool can_output = true;
            for (int j = 0; j < dpb->max_frames; j++) {
                if (i != j &&
					dpb->frames[j].state != DPB_UNUSED &&
					dpb->frames[j].output_order_key < pic->output_order_key) {
                    can_output = false;
                    break;
                }
            }

            if (can_output) {
				amlvdec_h264_dpb_dump_pic(__func__, pic);

				output_pic(dpb, pic);
				amlvdec_h264_dpb_recycle_pic(dpb, pic);
				output_count++;
				output_found = true;
            }
        }
    } while (output_found);

    return output_count;
}

void amlvdec_h264_dpb_dump(dpb_buffer_t *dpb) {
	for (int i = 0; i < dpb->max_frames; i++) {
		amlvdec_h264_dpb_dump_pic(__func__, &dpb->frames[i]);
	}
}
