#include "amlvdec_h264_lmem.h"


void dump_dpb_params(const struct amlvdec_h264_rpm *params) {
	pr_debug("DPB PARAM: pts_zero_0: %u", params->pts_zero_0);
	pr_debug("DPB PARAM: pts_zero_1: %u", params->pts_zero_1);
	pr_debug("DPB PARAM: fixed_frame_rate_flag: %u", params->fixed_frame_rate_flag);
	pr_debug("DPB PARAM: offset_delimiter_lo: %u", params->offset_delimiter_lo);
	pr_debug("DPB PARAM: offset_delimiter_hi: %u", params->offset_delimiter_hi);
	pr_debug("DPB PARAM: slice_iponly_break: %u", params->slice_iponly_break);
	pr_debug("DPB PARAM: prev_max_reference_frame_num: %u", params->prev_max_reference_frame_num);
	pr_debug("DPB PARAM: eos: %u", params->eos);
	pr_debug("DPB PARAM: frame_packing_type: %u", params->frame_packing_type);
	pr_debug("DPB PARAM: old_poc_par_1: %u", params->old_poc_par_1);
	pr_debug("DPB PARAM: old_poc_par_2: %u", params->old_poc_par_2);
	pr_debug("DPB PARAM: prev_mbx: %u", params->prev_mbx);
	pr_debug("DPB PARAM: prev_mby: %u", params->prev_mby);
	pr_debug("DPB PARAM: error_skip_mb_num: %u", params->error_skip_mb_num);
	pr_debug("DPB PARAM: error_mb_status_raw: %u", params->error_mb_status_raw);
	pr_debug("DPB PARAM: l0_pic0_status: %u", params->l0_pic0_status);
	pr_debug("DPB PARAM: timeout_counter: %u", params->timeout_counter);
	pr_debug("DPB PARAM: buffer_size: %u", params->buffer_size);
	pr_debug("DPB PARAM: buffer_size_hi: %u", params->buffer_size_hi);
	pr_debug("DPB PARAM: frame_crop_left_offset: %u", params->frame_crop_left_offset);
	pr_debug("DPB PARAM: frame_crop_right_offset: %u", params->frame_crop_right_offset);
	pr_debug("DPB PARAM: sps_flags2_raw: %u", params->sps_flags2_raw);
	pr_debug("DPB PARAM: num_reorder_frames: %u", params->num_reorder_frames);
	pr_debug("DPB PARAM: max_buffer_frame: %u", params->max_buffer_frame);
	pr_debug("DPB PARAM: non_conforming_stream: %u", params->non_conforming_stream);
	pr_debug("DPB PARAM: recovery_point: %u", params->recovery_point);
	pr_debug("DPB PARAM: post_canvas: %u", params->post_canvas);
	pr_debug("DPB PARAM: post_canvas_h: %u", params->post_canvas_h);
	pr_debug("DPB PARAM: skip_pic_count: %u", params->skip_pic_count);
	pr_debug("DPB PARAM: target_num_scaling_list: %u", params->target_num_scaling_list);
	pr_debug("DPB PARAM: ff_post_one_frame: %u", params->ff_post_one_frame);
	pr_debug("DPB PARAM: previous_bit_cnt: %u", params->previous_bit_cnt);
	pr_debug("DPB PARAM: mb_not_shift_count: %u", params->mb_not_shift_count);
	pr_debug("DPB PARAM: pic_status: %u", params->pic_status);
	pr_debug("DPB PARAM: frame_counter: %u", params->frame_counter);
	pr_debug("DPB PARAM: new_slice_type: %u", params->new_slice_type);
	pr_debug("DPB PARAM: new_picture_structure: %u", params->new_picture_structure);
	pr_debug("DPB PARAM: new_frame_num: %u", params->new_frame_num);
	pr_debug("DPB PARAM: new_idr_pic_id: %u", params->new_idr_pic_id);
	pr_debug("DPB PARAM: idr_pic_id: %u", params->idr_pic_id);
	pr_debug("DPB PARAM: nal_unit_type: %u", params->nal_unit_type);
	pr_debug("DPB PARAM: nal_ref_idc: %u", params->nal_ref_idc);
	pr_debug("DPB PARAM: slice_type: %u", params->slice_type);
	pr_debug("DPB PARAM: log2_max_frame_num: %u", params->log2_max_frame_num);
	pr_debug("DPB PARAM: frame_mbs_only_flag: %u", params->frame_mbs_only_flag);
	pr_debug("DPB PARAM: pic_order_cnt_type: %u", params->pic_order_cnt_type);
	pr_debug("DPB PARAM: log2_max_pic_order_cnt_lsb: %u", params->log2_max_pic_order_cnt_lsb);
	pr_debug("DPB PARAM: pic_order_present_flag: %u", params->pic_order_present_flag);
	pr_debug("DPB PARAM: redundant_pic_cnt_present_flag: %u", params->redundant_pic_cnt_present_flag);
	pr_debug("DPB PARAM: pic_init_qp_minus26: %u", params->pic_init_qp_minus26);
	pr_debug("DPB PARAM: deblocking_filter_control_present_flag: %u", params->deblocking_filter_control_present_flag);
	pr_debug("DPB PARAM: num_slice_groups_minus1: %u", params->num_slice_groups_minus1);
	pr_debug("DPB PARAM: mode_8x8_flags: %u", params->mode_8x8_flags);
	pr_debug("DPB PARAM: entropy_coding_mode_flag: %u", params->entropy_coding_mode_flag);
	pr_debug("DPB PARAM: slice_quant: %u", params->slice_quant);
	pr_debug("DPB PARAM: total_mb_height: %u", params->total_mb_height);
	pr_debug("DPB PARAM: picture_structure: %u", params->picture_structure);
	pr_debug("DPB PARAM: top_intra_type: %u", params->top_intra_type);
	pr_debug("DPB PARAM: rv_ai_status: %u", params->rv_ai_status);
	pr_debug("DPB PARAM: ai_read_start: %u", params->ai_read_start);
	pr_debug("DPB PARAM: ai_write_start: %u", params->ai_write_start);
	pr_debug("DPB PARAM: ai_cur_buffer: %u", params->ai_cur_buffer);
	pr_debug("DPB PARAM: ai_dma_buffer: %u", params->ai_dma_buffer);
	pr_debug("DPB PARAM: ai_read_offset: %u", params->ai_read_offset);
	pr_debug("DPB PARAM: ai_write_offset: %u", params->ai_write_offset);
	pr_debug("DPB PARAM: ai_write_offset_save: %u", params->ai_write_offset_save);
	pr_debug("DPB PARAM: rv_ai_buff_start: %u", params->rv_ai_buff_start);
	pr_debug("DPB PARAM: i_pic_mb_count: %u", params->i_pic_mb_count);
	pr_debug("DPB PARAM: ai_wr_dcac_dma_ctrl: %u", params->ai_wr_dcac_dma_ctrl);
	pr_debug("DPB PARAM: slice_mb_count: %u", params->slice_mb_count);
	pr_debug("DPB PARAM: pictype_raw: %u", params->pictype_raw);
	pr_debug("DPB PARAM: slice_group_map_type: %u", params->slice_group_map_type);
	pr_debug("DPB PARAM: mb_type_raw: %u", params->mb_type_raw);
	pr_debug("DPB PARAM: mb_aff_added_dma: %u", params->mb_aff_added_dma);
	pr_debug("DPB PARAM: previous_mb_type: %u", params->previous_mb_type);
	pr_debug("DPB PARAM: weighted_pred_flag: %u", params->weighted_pred_flag);
	pr_debug("DPB PARAM: weighted_bipred_idc: %u", params->weighted_bipred_idc);
	pr_debug("DPB PARAM: mbff_info_raw: %u", params->mbff_info_raw);
	pr_debug("DPB PARAM: top_intra_type_top: %u", params->top_intra_type_top);
	pr_debug("DPB PARAM: rv_ai_buff_inc: %u", params->rv_ai_buff_inc);
	pr_debug("DPB PARAM: default_mb_info_lo: %u", params->default_mb_info_lo);
	pr_debug("DPB PARAM: need_read_top_info: %u", params->need_read_top_info);
	pr_debug("DPB PARAM: read_top_info_state: %u", params->read_top_info_state);
	pr_debug("DPB PARAM: dcac_mbx: %u", params->dcac_mbx);
	pr_debug("DPB PARAM: top_mb_info_offset: %u", params->top_mb_info_offset);
	pr_debug("DPB PARAM: top_mb_info_rd_idx: %u", params->top_mb_info_rd_idx);
	pr_debug("DPB PARAM: top_mb_info_wr_idx: %u", params->top_mb_info_wr_idx);
	pr_debug("DPB PARAM: vld_waiting: %u", params->vld_waiting);
	pr_debug("DPB PARAM: mb_x_num: %u", params->mb_x_num);
	pr_debug("DPB PARAM: mb_width: %u", params->mb_width);
	pr_debug("DPB PARAM: mb_height: %u", params->mb_height);
	pr_debug("DPB PARAM: mbx: %u", params->mbx);
	pr_debug("DPB PARAM: total_mby: %u", params->total_mby);
	pr_debug("DPB PARAM: intr_msk_save: %u", params->intr_msk_save);
	pr_debug("DPB PARAM: need_disable_ppe: %u", params->need_disable_ppe);
	pr_debug("DPB PARAM: is_new_picture: %u", params->is_new_picture);
	pr_debug("DPB PARAM: prev_nal_ref_idc: %u", params->prev_nal_ref_idc);
	pr_debug("DPB PARAM: prev_nal_unit_type: %u", params->prev_nal_unit_type);
	pr_debug("DPB PARAM: frame_mb_count: %u", params->frame_mb_count);
	pr_debug("DPB PARAM: slice_group_ucode: %u", params->slice_group_ucode);
	pr_debug("DPB PARAM: slice_group_change_rate: %u", params->slice_group_change_rate);
	pr_debug("DPB PARAM: slice_group_change_cycle_len: %u", params->slice_group_change_cycle_len);
	pr_debug("DPB PARAM: delay_length: %u", params->delay_length);
	pr_debug("DPB PARAM: picture_struct: %u", params->picture_struct);
	pr_debug("DPB PARAM: frame_crop_top_offset: %u", params->frame_crop_top_offset);
	pr_debug("DPB PARAM: dcac_previous_mb_type: %u", params->dcac_previous_mb_type);
	pr_debug("DPB PARAM: time_stamp: %u", params->time_stamp);
	pr_debug("DPB PARAM: h_time_stamp: %u", params->h_time_stamp);
	pr_debug("DPB PARAM: vpts_map_addr: %u", params->vpts_map_addr);
	pr_debug("DPB PARAM: h_vpts_map_addr: %u", params->h_vpts_map_addr);
	pr_debug("DPB PARAM: frame_crop_bottom_offset: %u", params->frame_crop_bottom_offset);
	pr_debug("DPB PARAM: pic_insert_flag: %u", params->pic_insert_flag);
	pr_debug("DPB PARAM: time_stamp_region[0x18]: %u", params->time_stamp_region[0x18]);
	pr_debug("DPB PARAM: offset_for_non_ref_pic: %u", params->offset_for_non_ref_pic);
	pr_debug("DPB PARAM: offset_for_top_to_bottom_field: %u", params->offset_for_top_to_bottom_field);
	pr_debug("DPB PARAM: max_reference_frame_num: %u", params->max_reference_frame_num);
	pr_debug("DPB PARAM: frame_num_gap_allowed: %u", params->frame_num_gap_allowed);
	pr_debug("DPB PARAM: num_ref_frames_in_pic_order_cnt_cycle: %u", params->num_ref_frames_in_pic_order_cnt_cycle);
	pr_debug("DPB PARAM: profile_idc_mmco: %u", params->profile_idc_mmco);
	pr_debug("DPB PARAM: profile_idc: %u", params->profile_idc);
	pr_debug("DPB PARAM: level_idc_mmco: %u", params->level_idc_mmco);
	pr_debug("DPB PARAM: frame_size_in_mb: %u", params->frame_size_in_mb);
	pr_debug("DPB PARAM: delta_pic_order_always_zero_flag: %u", params->delta_pic_order_always_zero_flag);
	pr_debug("DPB PARAM: pps_num_ref_idx_l0_active_minus1: %u", params->pps_num_ref_idx_l0_active_minus1);
	pr_debug("DPB PARAM: pps_num_ref_idx_l1_active_minus1: %u", params->pps_num_ref_idx_l1_active_minus1);
	pr_debug("DPB PARAM: current_sps_id: %u", params->current_sps_id);
	pr_debug("DPB PARAM: current_pps_id: %u", params->current_pps_id);
	pr_debug("DPB PARAM: decode_status_raw: %u", params->decode_status_raw);
	pr_debug("DPB PARAM: first_mb_in_slice: %u", params->first_mb_in_slice);
	pr_debug("DPB PARAM: prev_mb_width: %u", params->prev_mb_width);
	pr_debug("DPB PARAM: prev_frame_size_in_mb: %u", params->prev_frame_size_in_mb);
	pr_debug("DPB PARAM: max_reference_frame_num_in_mem: %u", params->max_reference_frame_num_in_mem);
	pr_debug("DPB PARAM: chroma_format_idc: %u", params->chroma_format_idc);
	pr_debug("DPB PARAM: vui_status_raw: %u", params->vui_status_raw);
	pr_debug("DPB PARAM: aspect_ratio_idc: %u", params->aspect_ratio_idc);
	pr_debug("DPB PARAM: aspect_ratio_sar_width: %u", params->aspect_ratio_sar_width);
	pr_debug("DPB PARAM: aspect_ratio_sar_height: %u", params->aspect_ratio_sar_height);
	pr_debug("DPB PARAM: num_units_in_tick: %u", params->num_units_in_tick);
	pr_debug("DPB PARAM: time_scale: %u", params->time_scale);
	pr_debug("DPB PARAM: current_pic_info_raw: %u", params->current_pic_info_raw);
	pr_debug("DPB PARAM: dpb_buffer_info: %u", params->dpb_buffer_info);
	pr_debug("DPB PARAM: reference_pool_info: %u", params->reference_pool_info);
	pr_debug("DPB PARAM: reference_list_info: %u\n", params->reference_list_info);
}

void dump_dpb(const struct amlvdec_h264_dpb *dpb) {
	pr_debug("DPB: dpb_max_buffer_frame: %u", dpb->dpb_max_buffer_frame);
	pr_debug("DPB: actual_dpb_size: %u", dpb->actual_dpb_size);
	pr_debug("DPB: colocated_buf_status: %u", dpb->colocated_buf_status);
	pr_debug("DPB: num_forward_short_term_reference_pic: %u", dpb->num_forward_short_term_reference_pic);
	pr_debug("DPB: num_short_term_reference_pic: %u", dpb->num_short_term_reference_pic);
	pr_debug("DPB: num_reference_pic: %u", dpb->num_reference_pic);
	pr_debug("DPB: current_dpb_index: %u", dpb->current_dpb_index);
	pr_debug("DPB: current_decoded_frame_num: %u", dpb->current_decoded_frame_num);
	pr_debug("DPB: current_reference_frame_num: %u", dpb->current_reference_frame_num);
	pr_debug("DPB: l0_size: %u", dpb->l0_size);
	pr_debug("DPB: l1_size: %u", dpb->l1_size);
	pr_debug("dpb: nal_info_mmco: %u (nal_unit_type=%u, nal_ref_idc=%u)",
			dpb->nal_info_mmco_raw,
			dpb->nal_info_mmco.nal_unit_type,
			dpb->nal_info_mmco.nal_ref_idc);
	pr_debug("DPB: picture_structure_mmco: %u", dpb->picture_structure_mmco);
	pr_debug("DPB: frame_num: %u", dpb->frame_num);
	pr_debug("DPB: pic_order_cnt_lsb: %u", dpb->pic_order_cnt_lsb);
	pr_debug("DPB: num_ref_idx_l0_active_minus1: %u", dpb->num_ref_idx_l0_active_minus1);
	pr_debug("DPB: num_ref_idx_l1_active_minus1: %u", dpb->num_ref_idx_l1_active_minus1);
	pr_debug("DPB: PrevPicOrderCntLsb: %u", dpb->PrevPicOrderCntLsb);
	pr_debug("DPB: PreviousFrameNum: %u", dpb->PreviousFrameNum);
	pr_debug("DPB: delta_pic_order_cnt_bottom[2]: %u %u",
			dpb->delta_pic_order_cnt_bottom_hi,
			dpb->delta_pic_order_cnt_bottom_lo);
	pr_debug("DPB: delta_pic_order_cnt_0[2]: %u %u",
			dpb->delta_pic_order_cnt_0_hi,
			dpb->delta_pic_order_cnt_0_lo);
	pr_debug("DPB: delta_pic_order_cnt_1[2]: %u %u",
			dpb->delta_pic_order_cnt_1_hi,
			dpb->delta_pic_order_cnt_1_lo);
	pr_debug("DPB: PrevPicOrderCntMsb[2]: %u %u",
			dpb->PrevPicOrderCntMsb_hi,
			dpb->PrevPicOrderCntMsb_lo);
	pr_debug("DPB: PrevFrameNumOffset[2]: %u %u",
			dpb->PrevFrameNumOffset_hi,
			dpb->PrevFrameNumOffset_lo);
	pr_debug("DPB: frame_pic_order_cnt[2]: %u %u",
			dpb->frame_pic_order_cnt_hi,
			dpb->frame_pic_order_cnt_lo);
	pr_debug("DPB: top_field_pic_order_cnt[2]: %u %u",
			dpb->top_field_pic_order_cnt_hi,
			dpb->top_field_pic_order_cnt_lo);
	pr_debug("DPB: bottom_field_pic_order_cnt[2]: %u %u",
			dpb->bottom_field_pic_order_cnt_hi,
			dpb->bottom_field_pic_order_cnt_lo);
	pr_debug("DPB: colocated_mv_addr_start[2]: %u %u",
			dpb->colocated_mv_addr_start_hi,
			dpb->colocated_mv_addr_start_lo);
	pr_debug("DPB: colocated_mv_addr_end[2]: %u %u",
			dpb->colocated_mv_addr_end_hi,
			dpb->colocated_mv_addr_end_lo);
	pr_debug("DPB: colocated_mv_wr_addr[2]: %u %u",
			dpb->colocated_mv_wr_addr_hi,
			dpb->colocated_mv_wr_addr_lo);
	pr_debug("DPB: frame_crop_left_offset: %u", dpb->frame_crop_left_offset);
	pr_debug("DPB: frame_crop_right_offset: %u", dpb->frame_crop_right_offset);
	pr_debug("DPB: frame_crop_top_offset: %u", dpb->frame_crop_top_offset);
	pr_debug("DPB: frame_crop_bottom_offset: %u", dpb->frame_crop_bottom_offset);
	pr_debug("DPB: chroma_format_idc: %u\n", dpb->chroma_format_idc);
}

void dump_mmco(const struct amlvdec_h264_mmco *mmco) {
	print_hex_dump(KERN_DEBUG,
			"MMCO: offset_for_ref_frame_base ",
			DUMP_PREFIX_OFFSET,
			16, 2,
			mmco->offset_for_ref_frame_base,
			sizeof(mmco->offset_for_ref_frame_base),
			false);

	print_hex_dump(KERN_DEBUG,
			"MMCO: reference_base ",
			DUMP_PREFIX_OFFSET,
			16, 2,
			mmco->reference_base,
			sizeof(mmco->reference_base),
			false);
#if 0
	print_hex_dump(KERN_DEBUG,
			"MMCO: l0_reorder_cmd ",
			DUMP_PREFIX_OFFSET,
			16, 2,
			mmco->l0_reorder_cmd,
			sizeof(mmco->l0_reorder_cmd),
			false);

	print_hex_dump(KERN_DEBUG,
			"MMCO: l1_reorder_cmd ",
			DUMP_PREFIX_OFFSET,
			16, 2,
			mmco->l1_reorder_cmd,
			sizeof(mmco->l1_reorder_cmd),
			false);
#else
	print_hex_dump(KERN_DEBUG,
			"MMCO: reorder_cmd ",
			DUMP_PREFIX_OFFSET,
			16, 2,
			mmco->reorder_cmd,
			sizeof(mmco->reorder_cmd),
			false);
	for (int i=0; i < sizeof(mmco->reorder_cmd); i += 4) {
		if (i % 16 == 0) {
			pr_debug("MMCO: reorder_cmd (swapped) 0x%04x: ", i);
		}
		for (int j=3; j >= 0; j--) {
		//for (int j=0; j < 4; j++) {
			pr_cont("%01u ", mmco->reorder_cmd[i+j]);
		}
		if (i == sizeof(mmco->reorder_cmd) - 4) {
			pr_cont("\n");
		}
	}
#endif

	print_hex_dump(KERN_DEBUG,
			"MMCO: mmco_cmd ",
			DUMP_PREFIX_OFFSET,
			16, 2,
			mmco->mmco_cmd,
			sizeof(mmco->mmco_cmd),
			false);

	print_hex_dump(KERN_DEBUG,
			"MMCO: l0_base ",
			DUMP_PREFIX_OFFSET,
			16, 2,
			mmco->l0_base,
			sizeof(mmco->l0_base),
			false);

	print_hex_dump(KERN_DEBUG,
			"MMCO: l1_base ",
			DUMP_PREFIX_OFFSET,
			16, 2,
			mmco->l1_base,
			sizeof(mmco->l1_base),
			false);
}

