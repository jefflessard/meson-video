#ifndef __AML_VDEC_H264_LMEM_H__
#define __AML_VDEC_H264_LMEM_H__

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>

#define SWAP_LMEM_OFFSETS
#define SWAP_OFFSET(offset) (((offset) & ~3) + (3 - ((offset) & 3)))

#define FRAME_IN_DPB	24
#define DPB_OFFSET		0x100
#define MMCO_OFFSET		0x200


/* VLD waiting states */
enum vld_wait_state : uint16_t {
	VLD_NO_WAIT      = 0,
	VLD_WAIT_BUFFER  = 1,
	VLD_WAIT_HOST    = 2,
	VLD_WAIT_GAP     = 3
};

/* Code loading levels */
enum code_load_level : uint16_t {
	CODE_LOAD_LEVEL_MB      = 0,
	CODE_LOAD_LEVEL_PICTURE = 1,
	CODE_LOAD_LEVEL_SLICE   = 2
};

/* Top info reading states */
enum read_top_info_state : uint16_t {
	READ_TOP_NO_NEED    = 0,
	READ_TOP_WAIT_LEFT  = 1,
	READ_TOP_READ_INTRA = 2,
	READ_TOP_READ_MV    = 3
};

/* NAL unit types */
enum nal_unit_type : uint16_t {
	NAL_SLICE           = 1,
	NAL_SLICE_DPA       = 2,
	NAL_SLICE_DPB       = 3,
	NAL_SLICE_DPC       = 4,
	NAL_SLICE_IDR       = 5,
	NAL_SEI             = 6,
	NAL_SPS             = 7,
	NAL_PPS             = 8,
	NAL_AUD             = 9,
	NAL_END_SEQUENCE    = 10,
	NAL_END_STREAM      = 11,
	NAL_FILLER_DATA     = 12
};

/* Slice types */
enum slice_type : uint16_t {
	SLICE_P             = 0,
	SLICE_B             = 1,
	SLICE_I             = 2,
	SLICE_SP            = 3,
	SLICE_SI            = 4,
	SLICE_P_ONLY        = 5,
	SLICE_B_ONLY        = 6,
	SLICE_I_ONLY        = 7,
	SLICE_SP_ONLY       = 8,
	SLICE_SI_ONLY       = 9
};

/* Picture structure */
enum picture_structure : uint16_t {
	FRAME               = 0,
	TOP_FIELD           = 1,
	BOTTOM_FIELD        = 2,
	MBAFF_FRAME         = 3
};

/* Additional bitfield structures */

/* Picture type flags */
struct pictype_bits {
	uint16_t is_reference:1;
	uint16_t is_long_term:1;
	uint16_t is_bottom_field:1;
	uint16_t is_top_field:1;
	uint16_t is_idr:1;
	uint16_t idr_gop_number:11;
} __attribute__((packed));

/* MB type flags */
struct mb_type_bits {
	uint16_t is_intra:1;
	uint16_t is_skip:1;
	uint16_t is_8x8:1;
	uint16_t is_16x16:1;
	uint16_t is_pcm:1;
	uint16_t transform_size_8x8_flag:1;
	uint16_t reserved:10;
} __attribute__((packed));

/* Error MB status */
struct error_mb_status_bits {
	uint16_t error_type:4;         /* Type of error encountered */
	uint16_t recovery_point:1;     /* Error recovery point reached */
	uint16_t concealment_type:2;   /* Type of error concealment used */
	uint16_t reserved:9;
} __attribute__((packed));

/* Current picture information */
struct current_pic_info_bits {
	uint16_t field_pic_flag:1;
	uint16_t reference_flag:1;
	uint16_t bottom_field_flag:1;
	uint16_t second_field:1;
	uint16_t broken_link:1;
	uint16_t has_mmco_5:1;
	uint16_t reserved:10;
} __attribute__((packed));

/* basic bitfield definitions */

/* SPS flags bitfield definition */
struct sps_flags2_bits {
	uint16_t nal_hrd_parameters_present_flag:1;
	uint16_t vcl_hrd_parameters_present_flag:1;
	uint16_t pic_struct_present_flag:1;
	uint16_t bitstream_restriction_flag:1;
	uint16_t reserved:12;
} __attribute__((packed));

/* MBFF info bitfield definition */
struct mbff_info_bits {
	uint16_t frame_mbs_only_flag:1;
	uint16_t mb_adaptive_frame_field_flag:1;
	enum picture_structure picture_structure:2;
	uint16_t reserved:12;
} __attribute__((packed));

/* Decode status bitfield definition */
struct decode_status_bits {
	uint16_t sps_may_change:1;
	uint16_t pps_may_change:1;
	uint16_t dpb_just_initialized:1;
	uint16_t idr_not_decoded_yet:1;
	enum code_load_level code_load_level:2;    /* 0:mb, 1:picture, 2:slice */
	uint16_t reserved:10;
} __attribute__((packed));

/* VUI status bitfield definition */
struct vui_status_bits {
	uint16_t aspect_ratio_info_present_flag:1;
	uint16_t timing_info_present_flag:1;
	uint16_t nal_hrd_parameters_present_flag:1;
	uint16_t vcl_hrd_parameters_present_flag:1;
	uint16_t pic_struct_present_flag:1;
	uint16_t bitstream_restriction_flag:1;
	uint16_t reserved:10;
} __attribute__((packed));

#ifndef SWAP_LMEM_OFFSETS

struct amlvdec_h264_rpm {
	uint16_t h_time_stamp_region[0x18];           // 0x00 - 0x17
	uint16_t pts_zero_0;                          // 0x18
	uint16_t pts_zero_1;                          // 0x19
	uint16_t reserved_1a;                         // 0x1A
	uint16_t reserved_1b;                         // 0x1B
	uint16_t reserved_1c;                         // 0x1C
	uint16_t reserved_1d;                         // 0x1D
	uint16_t reserved_1e;                         // 0x1E
	uint16_t reserved_1f;                         // 0x1F
	uint16_t reserved_20;                         // 0x20
	uint16_t fixed_frame_rate_flag;               // 0x21
	uint16_t reserved_22;                         // 0x22
	uint16_t reserved_23;                         // 0x23
	uint16_t reserved_24;                         // 0x24
	uint16_t reserved_25;                         // 0x25
	uint16_t reserved_26;                         // 0x26
	uint16_t reserved_27;                         // 0x27
	uint16_t reserved_28;                         // 0x28
	uint16_t reserved_29;                         // 0x29
	uint16_t reserved_2a;                         // 0x2A
	uint16_t reserved_2b;                         // 0x2B
	uint16_t reserved_2c;                         // 0x2C
	uint16_t reserved_2d;                         // 0x2D
	uint16_t reserved_2e;                         // 0x2E
	uint16_t offset_delimiter_lo;                 // 0x2F
	uint16_t offset_delimiter_hi;                 // 0x30
	uint16_t reserved_31;                         // 0x31
	uint16_t reserved_32;                         // 0x32
	uint16_t reserved_33;                         // 0x33
	uint16_t reserved_34;                         // 0x34
	uint16_t reserved_35;                         // 0x35
	uint16_t reserved_36;                         // 0x36
	uint16_t reserved_37;                         // 0x37
	uint16_t reserved_38;                         // 0x38
	uint16_t reserved_39;                         // 0x39
	uint16_t reserved_3a;                         // 0x3A
	uint16_t reserved_3b;                         // 0x3B
	uint16_t reserved_3c;                         // 0x3C
	uint16_t reserved_3d;                         // 0x3D
	uint16_t reserved_3e;                         // 0x3E
	uint16_t reserved_3f;                         // 0x3F
	uint16_t reserved_40;                         // 0x40
	uint16_t reserved_41;                         // 0x41
	uint16_t reserved_42;                         // 0x42
	uint16_t reserved_43;                         // 0x43
	uint16_t reserved_44;                         // 0x44
	uint16_t reserved_45;                         // 0x45
	uint16_t reserved_46;                         // 0x46
	uint16_t reserved_47;                         // 0x47
	uint16_t reserved_48;                         // 0x48
	uint16_t reserved_49;                         // 0x49
	uint16_t reserved_4a;                         // 0x4A
	uint16_t reserved_4b;                         // 0x4B
	uint16_t reserved_4c;                         // 0x4C
	uint16_t reserved_4d;                         // 0x4D
	uint16_t reserved_4e;                         // 0x4E
	uint16_t reserved_4f;                         // 0x4F
	uint16_t reserved_50;                         // 0x50
	uint16_t reserved_51;                         // 0x51
	uint16_t reserved_52;                         // 0x52
	uint16_t reserved_53;                         // 0x53
	uint16_t reserved_54;                         // 0x54
	uint16_t reserved_55;                         // 0x55
	uint16_t reserved_56;                         // 0x56
	uint16_t reserved_57;                         // 0x57
	uint16_t reserved_58;                         // 0x58
	uint16_t reserved_59;                         // 0x59
	uint16_t reserved_5a;                         // 0x5A
	uint16_t reserved_5b;                         // 0x5B
	uint16_t slice_iponly_break;                  // 0x5C
	uint16_t prev_max_reference_frame_num;        // 0x5D
	uint16_t eos;                                 // 0x5E
	uint16_t frame_packing_type;                  // 0x5F
	uint16_t old_poc_par_1;                       // 0x60
	uint16_t old_poc_par_2;                       // 0x61
	uint16_t prev_mbx;                           // 0x62
	uint16_t prev_mby;                           // 0x63
	uint16_t error_skip_mb_num;                   // 0x64
	union {                                       // 0x65
		uint16_t error_mb_status_raw;
		struct error_mb_status_bits error_mb_status;
	};
	uint16_t l0_pic0_status;                      // 0x66
	uint16_t timeout_counter;                     // 0x67
	uint16_t buffer_size;                         // 0x68
	uint16_t buffer_size_hi;                      // 0x69
	uint16_t frame_crop_left_offset;              // 0x6A
	uint16_t frame_crop_right_offset;             // 0x6B
	union {                                       // 0x6C
		uint16_t sps_flags2_raw;
		struct sps_flags2_bits sps_flags2;
	};
	uint16_t num_reorder_frames;                  // 0x6D
	uint16_t max_buffer_frame;                    // 0x6E
	uint16_t reserved_6f;                         // 0x6F
	uint16_t non_conforming_stream;               // 0x70
	uint16_t recovery_point;                      // 0x71
	uint16_t post_canvas;                         // 0x72
	uint16_t post_canvas_h;                       // 0x73
	uint16_t skip_pic_count;                      // 0x74
	uint16_t target_num_scaling_list;             // 0x75
	uint16_t ff_post_one_frame;                   // 0x76
	uint16_t previous_bit_cnt;                    // 0x77
	uint16_t mb_not_shift_count;                  // 0x78
	uint16_t pic_status;                          // 0x79
	uint16_t frame_counter;                       // 0x7A
	uint16_t new_slice_type;                      // 0x7B
	enum picture_structure new_picture_structure; // 0x7C
	uint16_t new_frame_num;                       // 0x7D
	uint16_t new_idr_pic_id;                      // 0x7E
	uint16_t idr_pic_id;                          // 0x7F
	enum nal_unit_type nal_unit_type;             // 0x80
	uint16_t nal_ref_idc;                         // 0x81
	uint16_t slice_type;                          // 0x82
	uint16_t log2_max_frame_num;                  // 0x83
	uint16_t frame_mbs_only_flag;                 // 0x84
	uint16_t pic_order_cnt_type;                  // 0x85
	uint16_t log2_max_pic_order_cnt_lsb;          // 0x86
	uint16_t pic_order_present_flag;              // 0x87
	uint16_t redundant_pic_cnt_present_flag;      // 0x88
	uint16_t pic_init_qp_minus26;                 // 0x89
	uint16_t deblocking_filter_control_present_flag; // 0x8A
	uint16_t num_slice_groups_minus1;             // 0x8B
	uint16_t mode_8x8_flags;                      // 0x8C
	uint16_t entropy_coding_mode_flag;            // 0x8D
	uint16_t slice_quant;                         // 0x8E
	uint16_t total_mb_height;                     // 0x8F
	enum picture_structure picture_structure;     // 0x90
	uint16_t top_intra_type;                      // 0x91
	uint16_t rv_ai_status;                        // 0x92
	uint16_t ai_read_start;                       // 0x93
	uint16_t ai_write_start;                      // 0x94
	uint16_t ai_cur_buffer;                       // 0x95
	uint16_t ai_dma_buffer;                       // 0x96
	uint16_t ai_read_offset;                      // 0x97
	uint16_t ai_write_offset;                     // 0x98
	uint16_t ai_write_offset_save;                // 0x99
	uint16_t rv_ai_buff_start;                    // 0x9A
	uint16_t i_pic_mb_count;                      // 0x9B
	uint16_t ai_wr_dcac_dma_ctrl;                 // 0x9C
	uint16_t slice_mb_count;                      // 0x9D
	union {                                       // 0x9E
		uint16_t pictype_raw;
		struct pictype_bits pictype;
	};
	uint16_t slice_group_map_type;                // 0x9F
	union {                                       // 0xA0
		uint16_t mb_type_raw;
		struct mb_type_bits mb_type;
	};
	uint16_t mb_aff_added_dma;                    // 0xA1
	uint16_t previous_mb_type;                    // 0xA2
	uint16_t weighted_pred_flag;                  // 0xA3
	uint16_t weighted_bipred_idc;                 // 0xA4
	union {                                       // 0xA5
		uint16_t mbff_info_raw;
		struct mbff_info_bits mbff_info;
	};
	uint16_t top_intra_type_top;                  // 0xA6
	uint16_t rv_ai_buff_inc;                      // 0xA7
	uint16_t default_mb_info_lo;                  // 0xA8
	uint16_t need_read_top_info;                  // 0xA9
	uint16_t read_top_info_state;                 // 0xAA
	uint16_t dcac_mbx;                           // 0xAB
	uint16_t top_mb_info_offset;                  // 0xAC
	uint16_t top_mb_info_rd_idx;                  // 0xAD
	uint16_t top_mb_info_wr_idx;                  // 0xAE
	uint16_t vld_waiting;                         // 0xAF
	uint16_t mb_x_num;                           // 0xB0
	uint16_t mb_width;                           // 0xB2
	uint16_t mb_height;                          // 0xB2
	uint16_t mbx;                                // 0xB3
	uint16_t total_mby;                          // 0xB4
	uint16_t intr_msk_save;                       // 0xB5
	uint16_t need_disable_ppe;                    // 0xB6
	uint16_t is_new_picture;                      // 0xB7
	uint16_t prev_nal_ref_idc;                    // 0xB8
	enum nal_unit_type prev_nal_unit_type;        // 0xB9
	uint16_t frame_mb_count;                      // 0xBA
	uint16_t slice_group_ucode;                   // 0xBB
	uint16_t slice_group_change_rate;             // 0xBC
	uint16_t slice_group_change_cycle_len;        // 0xBD
	uint16_t delay_length;                        // 0xBE
	uint16_t picture_struct;                      // 0xBF
	uint16_t frame_crop_top_offset;               // 0xC0
	uint16_t dcac_previous_mb_type;               // 0xC1
	uint16_t time_stamp;                          // 0xC2
	uint16_t h_time_stamp;                        // 0xC3
	uint16_t vpts_map_addr;                       // 0xC4
	uint16_t h_vpts_map_addr;                     // 0xC5
	uint16_t frame_crop_bottom_offset;            // 0xC6
	uint16_t pic_insert_flag;                     // 0xC7
	uint16_t time_stamp_region[0x18];             // 0xC8 - 0xDF
	uint16_t offset_for_non_ref_pic;              // 0xE0
	uint16_t reserved_e1;                         // 0xE1
	uint16_t offset_for_top_to_bottom_field;      // 0xE2
	uint16_t reserved_e3;                         // 0xE3
	uint16_t max_reference_frame_num;             // 0xE4
	uint16_t frame_num_gap_allowed;               // 0xE5
	uint16_t num_ref_frames_in_pic_order_cnt_cycle; // 0xE6
	uint8_t profile_idc_mmco;                    // 0xE7 lo
	uint8_t profile_idc;                         // 0xE7 hi
	uint16_t level_idc_mmco;                      // 0xE8
	uint16_t frame_size_in_mb;                    // 0xE9
	uint16_t delta_pic_order_always_zero_flag;    // 0xEA
	uint16_t pps_num_ref_idx_l0_active_minus1;    // 0xEB
	uint16_t pps_num_ref_idx_l1_active_minus1;    // 0xEC
	uint16_t current_sps_id;                      // 0xED
	uint16_t current_pps_id;                      // 0xEE
	union {                                       // 0xEF
		uint16_t decode_status_raw;
		struct decode_status_bits decode_status;
	};
	uint16_t first_mb_in_slice;                   // 0xF0
	uint16_t prev_mb_width;                       // 0xF1
	uint16_t prev_frame_size_in_mb;               // 0xF2
	uint8_t max_reference_frame_num_in_mem;       // 0xF3 lo
	uint8_t chroma_format_idc;                    // 0xF3 hi
	union {                                       // 0xF4
		uint16_t vui_status_raw;
		struct vui_status_bits vui_status;
	};
	uint16_t aspect_ratio_idc;                    // 0xF5
	uint16_t aspect_ratio_sar_width;              // 0xF6
	uint16_t aspect_ratio_sar_height;             // 0xF7
	uint16_t num_units_in_tick;                   // 0xF8
	uint16_t reserved_f9;                         // 0xF9
	uint16_t time_scale;                          // 0xFA
	uint16_t reserved_fb;                         // 0xFB
	union {                                       // 0xFC
		uint16_t current_pic_info_raw;
		struct current_pic_info_bits current_pic_info;
	};
	uint16_t dpb_buffer_info;                     // 0xFD
	uint16_t reference_pool_info;                 // 0xFE
	uint16_t reference_list_info;                 // 0xFF
} __attribute__((packed));

#define VALIDATE_OFFSET(field, expected_offset) \
	static_assert(offsetof(struct amlvdec_h264_rpm, field) == (expected_offset * sizeof(uint16_t)), \
			"Offset mismatch for " #field)

#else

/* H.264 DPB Parameters Structure
 * Values are organized in groups of 4 to match hardware layout
 * Each group is reversed from the hardware representation
 */
struct amlvdec_h264_rpm {
	uint16_t h_time_stamp_region[0x18];           // 0x00 - 0x17
	uint16_t reserved_1b;                         // 0x1B
	uint16_t reserved_1a;                         // 0x1A
	uint16_t pts_zero_1;                          // 0x19
	uint16_t pts_zero_0;                          // 0x18

	uint16_t reserved_1f;                         // 0x1F
	uint16_t reserved_1e;                         // 0x1E
	uint16_t reserved_1d;                         // 0x1D
	uint16_t reserved_1c;                         // 0x1C

	uint16_t reserved_23;                         // 0x23
	uint16_t reserved_22;                         // 0x22
	uint16_t fixed_frame_rate_flag;               // 0x21
	uint16_t reserved_20;                         // 0x20

	uint16_t reserved_27;                         // 0x27
	uint16_t reserved_26;                         // 0x26
	uint16_t reserved_25;                         // 0x25
	uint16_t reserved_24;                         // 0x24

	uint16_t reserved_2b;                         // 0x2B
	uint16_t reserved_2a;                         // 0x2A
	uint16_t reserved_29;                         // 0x29
	uint16_t reserved_28;                         // 0x28

	uint16_t offset_delimiter_lo;                 // 0x2F
	uint16_t reserved_2e;                         // 0x2E
	uint16_t reserved_2d;                         // 0x2D
	uint16_t reserved_2c;                         // 0x2C

	uint16_t reserved_33;                         // 0x33
	uint16_t reserved_32;                         // 0x32
	uint16_t reserved_31;                         // 0x31
	uint16_t offset_delimiter_hi;                 // 0x30

	uint16_t reserved_37;                         // 0x37
	uint16_t reserved_36;                         // 0x36
	uint16_t reserved_35;                         // 0x35
	uint16_t reserved_34;                         // 0x34
	
	uint16_t reserved_38;                         // 0x38
	uint16_t reserved_39;                         // 0x39
	uint16_t reserved_3a;                         // 0x3A
	uint16_t reserved_3b;                         // 0x3B

	uint16_t reserved_3f;                         // 0x3F
	uint16_t reserved_3e;                         // 0x3E
	uint16_t reserved_3d;                         // 0x3D
	uint16_t reserved_3c;                         // 0x3C

	uint16_t reserved_43;                         // 0x43
	uint16_t reserved_42;                         // 0x42
	uint16_t reserved_41;                         // 0x41
	uint16_t reserved_40;                         // 0x40

	uint16_t reserved_47;                         // 0x47
	uint16_t reserved_46;                         // 0x46
	uint16_t reserved_45;                         // 0x45
	uint16_t reserved_44;                         // 0x44

	uint16_t reserved_4b;                         // 0x4B
	uint16_t reserved_4a;                         // 0x4A
	uint16_t reserved_49;                         // 0x49
	uint16_t reserved_48;                         // 0x48

	uint16_t reserved_4f;                         // 0x4F
	uint16_t reserved_4e;                         // 0x4E
	uint16_t reserved_4d;                         // 0x4D
	uint16_t reserved_4c;                         // 0x4C

	uint16_t reserved_53;                         // 0x53
	uint16_t reserved_52;                         // 0x52
	uint16_t reserved_51;                         // 0x51
	uint16_t reserved_50;                         // 0x50

	uint16_t reserved_57;                         // 0x57
	uint16_t reserved_56;                         // 0x56
	uint16_t reserved_55;                         // 0x55
	uint16_t reserved_54;                         // 0x54

	uint16_t reserved_5b;                         // 0x5B
	uint16_t reserved_5a;                         // 0x5A
	uint16_t reserved_59;                         // 0x59
	uint16_t reserved_58;                         // 0x58

	uint16_t frame_packing_type;                  // 0x5F
	uint16_t eos;                                 // 0x5E
	uint16_t prev_max_reference_frame_num;        // 0x5D
	uint16_t slice_iponly_break;                  // 0x5C

	uint16_t prev_mby;                           // 0x63
	uint16_t prev_mbx;                           // 0x62
	uint16_t old_poc_par_2;                       // 0x61
	uint16_t old_poc_par_1;                       // 0x60

	uint16_t timeout_counter;                     // 0x67
	uint16_t l0_pic0_status;                      // 0x66
	union {                                       // 0x65
		uint16_t error_mb_status_raw;
		struct error_mb_status_bits error_mb_status;
	};
	uint16_t error_skip_mb_num;                   // 0x64

	uint16_t frame_crop_right_offset;             // 0x6B
	uint16_t frame_crop_left_offset;              // 0x6A
	uint16_t buffer_size_hi;                      // 0x69
	uint16_t buffer_size;                         // 0x68

	uint16_t reserved_6f;                         // 0x6F
	uint16_t max_buffer_frame;                    // 0x6E
	uint16_t num_reorder_frames;                  // 0x6D
	union {                                       // 0x6C
		uint16_t sps_flags2_raw;
		struct sps_flags2_bits sps_flags2;
	};

	uint16_t post_canvas_h;                       // 0x73
	uint16_t post_canvas;                         // 0x72
	uint16_t recovery_point;                      // 0x71
	uint16_t non_conforming_stream;               // 0x70

	uint16_t previous_bit_cnt;                    // 0x77
	uint16_t ff_post_one_frame;                   // 0x76
	uint16_t target_num_scaling_list;             // 0x75
	uint16_t skip_pic_count;                      // 0x74

	uint16_t new_slice_type;                      // 0x7B
	uint16_t frame_counter;                       // 0x7A
	uint16_t pic_status;                          // 0x79
	uint16_t mb_not_shift_count;                  // 0x78

	uint16_t idr_pic_id;                          // 0x7F
	uint16_t new_idr_pic_id;                      // 0x7E
	uint16_t new_frame_num;                       // 0x7D
	enum picture_structure new_picture_structure; // 0x7C

	uint16_t log2_max_frame_num;                  // 0x83
	uint16_t slice_type;                          // 0x82
	uint16_t nal_ref_idc;                         // 0x81
	enum nal_unit_type nal_unit_type;             // 0x80

	uint16_t pic_order_present_flag;              // 0x87
	uint16_t log2_max_pic_order_cnt_lsb;          // 0x86
	uint16_t pic_order_cnt_type;                  // 0x85
	uint16_t frame_mbs_only_flag;                 // 0x84

	uint16_t num_slice_groups_minus1;             // 0x8B
	uint16_t deblocking_filter_control_present_flag; // 0x8A
	uint16_t pic_init_qp_minus26;                 // 0x89
	uint16_t redundant_pic_cnt_present_flag;      // 0x88

	uint16_t total_mb_height;                     // 0x8F
	uint16_t slice_quant;                         // 0x8E
	uint16_t entropy_coding_mode_flag;            // 0x8D
	uint16_t mode_8x8_flags;                      // 0x8C

	uint16_t ai_read_start;                       // 0x93
	uint16_t rv_ai_status;                        // 0x92
	uint16_t top_intra_type;                      // 0x91
	enum picture_structure picture_structure;     // 0x90

	uint16_t ai_read_offset;                      // 0x97
	uint16_t ai_dma_buffer;                       // 0x96
	uint16_t ai_cur_buffer;                       // 0x95
	uint16_t ai_write_start;                      // 0x94

	uint16_t i_pic_mb_count;                      // 0x9B
	uint16_t rv_ai_buff_start;                    // 0x9A
	uint16_t ai_write_offset_save;                // 0x99
	uint16_t ai_write_offset;                     // 0x98

	uint16_t slice_group_map_type;                // 0x9F
	union {                                       // 0x9E
		uint16_t pictype_raw;
		struct pictype_bits pictype;
	};
	uint16_t slice_mb_count;                      // 0x9D
	uint16_t ai_wr_dcac_dma_ctrl;                 // 0x9C

	uint16_t weighted_pred_flag;                  // 0xA3
	uint16_t previous_mb_type;                    // 0xA2
	uint16_t mb_aff_added_dma;                    // 0xA1
	union {                                       // 0xA0
		uint16_t mb_type_raw;
		struct mb_type_bits mb_type;
	};

	uint16_t rv_ai_buff_inc;                      // 0xA7
	uint16_t top_intra_type_top;                  // 0xA6
	union {                                       // 0xA5
		uint16_t mbff_info_raw;
		struct mbff_info_bits mbff_info;
	};
	uint16_t weighted_bipred_idc;                 // 0xA4

	uint16_t dcac_mbx;                           // 0xAB
	uint16_t read_top_info_state;                 // 0xAA
	uint16_t need_read_top_info;                  // 0xA9
	uint16_t default_mb_info_lo;                  // 0xA8

	uint16_t vld_waiting;                         // 0xAF
	uint16_t top_mb_info_wr_idx;                  // 0xAE
	uint16_t top_mb_info_rd_idx;                  // 0xAD
	uint16_t top_mb_info_offset;                  // 0xAC

	uint16_t mbx;                                // 0xB3
	uint16_t mb_height;                          // 0xB2
	uint16_t mb_width;                           // 0xB2
	uint16_t mb_x_num;                           // 0xB0

	uint16_t is_new_picture;                      // 0xB7
	uint16_t need_disable_ppe;                    // 0xB6
	uint16_t intr_msk_save;                       // 0xB5
	uint16_t total_mby;                          // 0xB4

	uint16_t slice_group_ucode;                   // 0xBB
	uint16_t frame_mb_count;                      // 0xBA
	enum nal_unit_type prev_nal_unit_type;        // 0xB9
	uint16_t prev_nal_ref_idc;                    // 0xB8

	uint16_t picture_struct;                      // 0xBF
	uint16_t delay_length;                        // 0xBE
	uint16_t slice_group_change_cycle_len;        // 0xBD
	uint16_t slice_group_change_rate;             // 0xBC

	uint16_t h_time_stamp;                        // 0xC3
	uint16_t time_stamp;                          // 0xC2
	uint16_t dcac_previous_mb_type;               // 0xC1
	uint16_t frame_crop_top_offset;               // 0xC0

	uint16_t pic_insert_flag;                     // 0xC7
	uint16_t frame_crop_bottom_offset;            // 0xC6
	uint16_t h_vpts_map_addr;                     // 0xC5
	uint16_t vpts_map_addr;                       // 0xC4

	uint16_t time_stamp_region[0x18];             // 0xC8 - 0xDF

	uint16_t reserved_e3;                         // 0xE3
	uint16_t offset_for_top_to_bottom_field;      // 0xE2
	uint16_t reserved_e1;                         // 0xE1
	uint16_t offset_for_non_ref_pic;              // 0xE0

	uint8_t profile_idc_mmco;                    // 0xE7 lo
	uint8_t profile_idc;                         // 0xE7 hi
	uint16_t num_ref_frames_in_pic_order_cnt_cycle; // 0xE6
	uint16_t frame_num_gap_allowed;               // 0xE5
	uint16_t max_reference_frame_num;             // 0xE4

	uint16_t pps_num_ref_idx_l0_active_minus1;    // 0xEB
	uint16_t delta_pic_order_always_zero_flag;    // 0xEA
	uint16_t frame_size_in_mb;                    // 0xE9
	uint16_t level_idc_mmco;                      // 0xE8

	union {                                       // 0xEF
		uint16_t decode_status_raw;
		struct decode_status_bits decode_status;
	};
	uint16_t current_pps_id;                      // 0xEE
	uint16_t current_sps_id;                      // 0xED
	uint16_t pps_num_ref_idx_l1_active_minus1;    // 0xEC

	uint8_t max_reference_frame_num_in_mem;       // 0xF3 lo
	uint8_t chroma_format_idc;                    // 0xF3 hi
	uint16_t prev_frame_size_in_mb;               // 0xF2
	uint16_t prev_mb_width;                       // 0xF1
	uint16_t first_mb_in_slice;                   // 0xF0

	uint16_t aspect_ratio_sar_height;             // 0xF7
	uint16_t aspect_ratio_sar_width;              // 0xF6
	uint16_t aspect_ratio_idc;                    // 0xF5
	union {                                       // 0xF4
		uint16_t vui_status_raw;
		struct vui_status_bits vui_status;
	};

	uint16_t reserved_fb;                         // 0xFB
	uint16_t time_scale;                          // 0xFA
	uint16_t reserved_f9;                         // 0xF9
	uint16_t num_units_in_tick;                   // 0xF8

	uint16_t reference_list_info;                 // 0xFF
	uint16_t reference_pool_info;                 // 0xFE
	uint16_t dpb_buffer_info;                     // 0xFD
	union {                                       // 0xFC
		uint16_t current_pic_info_raw;
		struct current_pic_info_bits current_pic_info;
	};
} __attribute__((packed));

#define VALIDATE_OFFSET(field, offset) \
	static_assert(offsetof(struct amlvdec_h264_rpm, field) == \
			SWAP_OFFSET(offset) * sizeof(uint16_t), \
			"Offset mismatch for " #field)

#endif

/* Validate key offsets across the range */
VALIDATE_OFFSET(pts_zero_0, 0x18);
VALIDATE_OFFSET(pts_zero_1, 0x19);
VALIDATE_OFFSET(fixed_frame_rate_flag, 0x21);
VALIDATE_OFFSET(offset_delimiter_lo, 0x2F);
VALIDATE_OFFSET(offset_delimiter_hi, 0x30);
VALIDATE_OFFSET(slice_iponly_break, 0x5C);
VALIDATE_OFFSET(frame_packing_type, 0x5F);
VALIDATE_OFFSET(old_poc_par_1, 0x60);
VALIDATE_OFFSET(sps_flags2, 0x6C);
VALIDATE_OFFSET(num_reorder_frames, 0x6D);
VALIDATE_OFFSET(max_buffer_frame, 0x6E);
VALIDATE_OFFSET(non_conforming_stream, 0x70);
VALIDATE_OFFSET(nal_unit_type, 0x80);
VALIDATE_OFFSET(slice_type, 0x82);
VALIDATE_OFFSET(picture_structure, 0x90);
VALIDATE_OFFSET(mb_type, 0xA0);
VALIDATE_OFFSET(mb_x_num, 0xB0);
VALIDATE_OFFSET(mb_height, 0xB2);
VALIDATE_OFFSET(need_disable_ppe, 0xB6);
VALIDATE_OFFSET(frame_crop_top_offset, 0xC0);
VALIDATE_OFFSET(time_stamp, 0xC2);
VALIDATE_OFFSET(offset_for_non_ref_pic, 0xE0);
VALIDATE_OFFSET(max_reference_frame_num, 0xE4);
VALIDATE_OFFSET(vui_status, 0xF4);
VALIDATE_OFFSET(reference_list_info, 0xFF);

#undef VALIDATE_OFFSET

/* NAL_info_mmco bitfield */
/* [6:5] : nal_ref_idc */
/* [4:0] : nal_unit_type */
struct nal_info_mmco_bits {
	enum nal_unit_type nal_unit_type:5;
	uint16_t nal_ref_idc:2;
	uint16_t reserved:9;
} __attribute__((packed));

/* picture_structure_mmco enum */
/* [1:0] : 00 - top field, 01 - bottom field,
 *   10 - frame, 11 - mbaff frame
 */
enum picture_structure_mmco : uint16_t {
	MMCO_TOP_FIELD    = 0,
	MMCO_BOTTOM_FIELD = 1,
	MMCO_FRAME        = 2,
	MMCO_MBAFF_FRAME  = 3,
};

#ifndef SWAP_LMEM_OFFSETS

struct amlvdec_h264_dpb {
	uint16_t dpb_base[FRAME_IN_DPB<<3];

	uint16_t dpb_max_buffer_frame;
	uint16_t actual_dpb_size;

	uint16_t colocated_buf_status;

	uint16_t num_forward_short_term_reference_pic;
	uint16_t num_short_term_reference_pic;
	uint16_t num_reference_pic;

	uint16_t current_dpb_index;
	uint16_t current_decoded_frame_num;
	uint16_t current_reference_frame_num;

	uint16_t l0_size;
	uint16_t l1_size;

	union {
		uint16_t nal_info_mmco_raw;
		struct nal_info_mmco_bits nal_info_mmco;
	};

	enum picture_structure_mmco picture_structure_mmco;

	uint16_t frame_num;
	uint16_t pic_order_cnt_lsb;

	uint16_t num_ref_idx_l0_active_minus1;
	uint16_t num_ref_idx_l1_active_minus1;

	uint16_t PrevPicOrderCntLsb;
	uint16_t PreviousFrameNum;

	uint16_t delta_pic_order_cnt_bottom_lo;
	uint16_t delta_pic_order_cnt_bottom_hi;
	uint16_t delta_pic_order_cnt_0_lo;
	uint16_t delta_pic_order_cnt_0_hi;

	uint16_t delta_pic_order_cnt_1_lo;
	uint16_t delta_pic_order_cnt_1_hi;
	uint16_t PrevPicOrderCntMsb_lo;
	uint16_t PrevPicOrderCntMsb_hi;

	uint16_t PrevFrameNumOffset_lo;
	uint16_t PrevFrameNumOffset_hi;
	uint16_t frame_pic_order_cnt_lo;
	uint16_t frame_pic_order_cnt_hi;

	uint16_t top_field_pic_order_cnt_lo;
	uint16_t top_field_pic_order_cnt_hi;
	uint16_t bottom_field_pic_order_cnt_lo;
	uint16_t bottom_field_pic_order_cnt_hi;

	uint16_t colocated_mv_addr_start_lo;
	uint16_t colocated_mv_addr_start_hi;
	uint16_t colocated_mv_addr_end_lo;
	uint16_t colocated_mv_addr_end_hi;

	uint16_t colocated_mv_wr_addr_lo;
	uint16_t colocated_mv_wr_addr_hi;
	uint16_t frame_crop_left_offset;
	uint16_t frame_crop_right_offset;

	uint16_t frame_crop_top_offset;
	uint16_t frame_crop_bottom_offset;
	uint16_t chroma_format_idc;

	uint16_t reserved1;
	uint16_t reserved2;

	uint16_t padding[16];
} __attribute__((packed));

#else

struct amlvdec_h264_dpb {
	uint16_t dpb_base[FRAME_IN_DPB<<3];

	uint16_t num_forward_short_term_reference_pic;
	uint16_t colocated_buf_status;
	uint16_t actual_dpb_size;
	uint16_t dpb_max_buffer_frame;

	uint16_t current_decoded_frame_num;
	uint16_t current_dpb_index;
	uint16_t num_reference_pic;
	uint16_t num_short_term_reference_pic;

	union {
		uint16_t nal_info_mmco_raw;
		struct nal_info_mmco_bits nal_info_mmco;
	};
	uint16_t l1_size;
	uint16_t l0_size;
	uint16_t current_reference_frame_num;

	uint16_t num_ref_idx_l0_active_minus1;
	uint16_t pic_order_cnt_lsb;
	uint16_t frame_num;
	enum picture_structure_mmco picture_structure_mmco;

	uint16_t delta_pic_order_cnt_bottom_lo;
	uint16_t PreviousFrameNum;
	uint16_t PrevPicOrderCntLsb;
	uint16_t num_ref_idx_l1_active_minus1;

	uint16_t delta_pic_order_cnt_1_lo;
	uint16_t delta_pic_order_cnt_0_hi;
	uint16_t delta_pic_order_cnt_0_lo;
	uint16_t delta_pic_order_cnt_bottom_hi;

	uint16_t PrevFrameNumOffset_lo;
	uint16_t PrevPicOrderCntMsb_hi;
	uint16_t PrevPicOrderCntMsb_lo;
	uint16_t delta_pic_order_cnt_1_hi;

	uint16_t top_field_pic_order_cnt_lo;
	uint16_t frame_pic_order_cnt_hi;
	uint16_t frame_pic_order_cnt_lo;
	uint16_t PrevFrameNumOffset_hi;

	uint16_t colocated_mv_addr_start_lo;
	uint16_t bottom_field_pic_order_cnt_hi;
	uint16_t bottom_field_pic_order_cnt_lo;
	uint16_t top_field_pic_order_cnt_hi;

	uint16_t colocated_mv_wr_addr_lo;
	uint16_t colocated_mv_addr_end_hi;
	uint16_t colocated_mv_addr_end_lo;
	uint16_t colocated_mv_addr_start_hi;

	uint16_t frame_crop_top_offset;
	uint16_t frame_crop_right_offset;
	uint16_t frame_crop_left_offset;
	uint16_t colocated_mv_wr_addr_hi;

	uint16_t reserved2;
	uint16_t reserved1;
	uint16_t chroma_format_idc;
	uint16_t frame_crop_bottom_offset;

	uint16_t padding[16];
} __attribute__((packed));

#endif

struct amlvdec_h264_mmco {
	/* array base address for offset_for_ref_frame */
	uint16_t offset_for_ref_frame_base[128];

	/* 0 - Index in DPB
	 * 1 - Picture Flag
	 *  [    2] : 0 - short term reference,
	 *            1 - long term reference
	 *  [    1] : bottom field
	 *  [    0] : top field
	 * 2 - Picture Number (short term or long term) low 16 bits
	 * 3 - Picture Number (short term or long term) high 16 bits
	 */
	uint16_t	reference_base[128];

	/* command and parameter, until command is 3 */
	/* merged l0_reorder_cmd and l1_reorder_cmd
	 * to allow risc buffer swapping
	uint16_t l0_reorder_cmd[66];
	uint16_t l1_reorder_cmd[66];
	*/
	uint16_t reorder_cmd[132];

	/* command and parameter, until command is 0 */
	uint16_t mmco_cmd[44];

	uint16_t l0_base[40];
	uint16_t l1_base[40];
} __attribute__((packed));

struct amlvdec_h264_lmem {
	struct amlvdec_h264_rpm params;
	struct amlvdec_h264_dpb dpb;
	struct amlvdec_h264_mmco mmco;
};

/* Verify structure size matches hardware requirements */
static_assert(sizeof(struct amlvdec_h264_rpm) == 0x100 * sizeof(uint16_t),
		"H264 RPM params structure size mismatch");

static_assert(sizeof(struct amlvdec_h264_dpb) == 0x100 * sizeof(uint16_t),
		"H264 DPB structure size mismatch");

static_assert(offsetof(struct amlvdec_h264_lmem, params) == 0,
		"Offset mismatch for params");
static_assert(offsetof(struct amlvdec_h264_lmem, dpb) == 
		DPB_OFFSET * sizeof(uint16_t),
		"Offset mismatch for dpb");
static_assert(offsetof(struct amlvdec_h264_lmem, mmco) == 
		MMCO_OFFSET * sizeof(uint16_t),
		"Offset mismatch for mmco");

/* Helper macros for accessing time stamp region */
#define H_TIME_STAMP_START     0x00
#define H_TIME_STAMP_END      0x17
#define TIME_STAMP_START      0xC8
#define TIME_STAMP_END        0xDF

static inline uint16_t amlvdec_h264_mmco_cmd(const struct amlvdec_h264_lmem *lmem, uint8_t index) {
	if (index >= ARRAY_SIZE(lmem->mmco.mmco_cmd))
		return 0;

#ifdef SWAP_LMEM_OFFSETS
	return lmem->mmco.mmco_cmd[SWAP_OFFSET(index)];
#else
	return lmem->mmco.mmco_cmd[index];
#endif

}

#if 0
/* Helper inline function to get time stamp at specific index */
static inline uint16_t get_time_stamp(const struct amlvdec_h264_rpm *params, uint8_t index) {
	if (index <= (H_TIME_STAMP_END - H_TIME_STAMP_START)) {
		return params->h_time_stamp_region[index];
	} else if (index <= (TIME_STAMP_END - TIME_STAMP_START)) {
		return params->time_stamp_region[index - (TIME_STAMP_END - TIME_STAMP_START)];
	}
	return 0;
}
#endif

/* Helper macros for picture type identification */
#define IS_I_SLICE(slice_type) ((slice_type) == SLICE_I || (slice_type) == SLICE_I_ONLY)
#define IS_P_SLICE(slice_type) ((slice_type) == SLICE_P || (slice_type) == SLICE_P_ONLY)
#define IS_B_SLICE(slice_type) ((slice_type) == SLICE_B || (slice_type) == SLICE_B_ONLY)

#if 0
/* Helper function to check if frame needs reordering */
static inline bool needs_reordering(const struct amlvdec_h264_rpm *params) {
	return params->num_reorder_frames > 0;
}
#endif

/* Helper function to check if current picture is IDR */
static inline bool is_idr_pic(const struct amlvdec_h264_lmem *lmem) {
#if 0
	return lmem->params.nal_unit_type == NAL_SLICE_IDR;
#else
	return lmem->dpb.nal_info_mmco.nal_unit_type == NAL_SLICE_IDR;
#endif
}

#if 0
/* Helper function to get maximum DPB size */
static inline uint16_t get_max_dpb_size(const struct amlvdec_h264_rpm *params) {
	return params->max_reference_frame_num_in_mem;
}

/* Helper function to check if picture timing SEI present */
static inline bool has_pic_timing_sei(const struct amlvdec_h264_rpm *params) {
	return params->vui_status.pic_struct_present_flag != 0;
}

/* Helper function to check if current picture uses MBAFF */
static inline bool is_mbaff_frame(const struct amlvdec_h264_rpm *params) {
	return params->mbff_info.mb_adaptive_frame_field_flag != 0 &&
		params->mbff_info.frame_mbs_only_flag != 0;
}

/* Validation functions */

struct dpb_validation_error {
	const char *field_name;
	const char *error_message;
	uint16_t actual_value;
	uint16_t expected_value;
};

/* Validate NAL unit type */
static inline bool validate_nal_unit(const struct amlvdec_h264_rpm *params, 
		struct dpb_validation_error *error) {
	if (params->nal_unit_type > NAL_FILLER_DATA) {
		if (error) {
			error->field_name = "nal_unit_type";
			error->error_message = "Invalid NAL unit type";
			error->actual_value = params->nal_unit_type;
			error->expected_value = NAL_FILLER_DATA;
		}
		return false;
	}
	return true;
}

/* Validate slice type */
static inline bool validate_slice_type(const struct amlvdec_h264_rpm *params,
		struct dpb_validation_error *error) {
	if (params->slice_type > SLICE_SI_ONLY) {
		if (error) {
			error->field_name = "slice_type";
			error->error_message = "Invalid slice type";
			error->actual_value = params->slice_type;
			error->expected_value = SLICE_SI_ONLY;
		}
		return false;
	}
	return true;
}

/* Validate frame dimensions */
static inline bool validate_frame_dimensions(const struct amlvdec_h264_rpm *params,
		struct dpb_validation_error *error) {
	if (params->mb_x_num == 0 || params->mb_height == 0 ||
			params->mb_x_num > 256 || params->mb_height > 256) {
		if (error) {
			error->field_name = "frame_dimensions";
			error->error_message = "Invalid frame dimensions";
			error->actual_value = (params->mb_x_num << 8) | params->mb_height;
			error->expected_value = 0;
		}
		return false;
	}
	return true;
}

/* Validate picture structure */
static inline bool validate_picture_structure(const struct amlvdec_h264_rpm *params,
		struct dpb_validation_error *error) {
	if (params->picture_structure > MBAFF_FRAME) {
		if (error) {
			error->field_name = "picture_structure";
			error->error_message = "Invalid picture structure";
			error->actual_value = params->picture_structure;
			error->expected_value = MBAFF_FRAME;
		}
		return false;
	}
	return true;
}

/* Helper for checking temporal references */
static inline bool is_valid_temporal_reference(const struct amlvdec_h264_rpm *params) {
	return (params->new_frame_num < (1 << params->log2_max_frame_num));
}

/* Helper for checking POC values */
static inline bool is_valid_poc(const struct amlvdec_h264_rpm *params) {
	if (params->pic_order_cnt_type == 0) {
		return params->log2_max_pic_order_cnt_lsb >= 4 && 
			params->log2_max_pic_order_cnt_lsb <= 16;
	}
	return true;
}

/* Comprehensive validation */
static inline bool validate_dpb_params(const struct amlvdec_h264_rpm *params,
		struct dpb_validation_error *error) {
	return validate_nal_unit(params, error) &&
		validate_slice_type(params, error) &&
		validate_frame_dimensions(params, error) &&
		validate_picture_structure(params, error) &&
		is_valid_temporal_reference(params) &&
		is_valid_poc(params);
}

/* Helper functions for common operations */

/* Get frame rate from VUI parameters */
static inline float get_frame_rate(const struct amlvdec_h264_rpm *params) {
	if (params->vui_status.timing_info_present_flag) {
		return (float)params->time_scale / (2.0f * params->num_units_in_tick);
	}
	return 0.0f;
}

/* Check if current picture needs deblocking */
static inline bool needs_deblocking(const struct amlvdec_h264_rpm *params) {
	return params->deblocking_filter_control_present_flag &&
		(params->mb_type.is_intra || params->mb_type.is_16x16);
}

/* Get aspect ratio */
static inline float get_aspect_ratio(const struct amlvdec_h264_rpm *params) {
	if (!params->vui_status.aspect_ratio_info_present_flag) {
		return 1.0f;
	}

	if (params->aspect_ratio_idc == 255) { // Extended_SAR
		return (float)params->aspect_ratio_sar_width / 
			(float)params->aspect_ratio_sar_height;
	}

	/* Return predefined aspect ratios based on aspect_ratio_idc */
	static const float predefined_aspect_ratios[] = {
		1.0f,    /* 0 - Unspecified */
		1.0f,    /* 1 - 1:1 */
		12.0f/11.0f,     /* 2 - 12:11 */
		10.0f/11.0f,     /* 3 - 10:11 */
		16.0f/11.0f,     /* 4 - 16:11 */
		40.0f/33.0f,     /* 5 - 40:33 */
		24.0f/11.0f,     /* 6 - 24:11 */
		20.0f/11.0f,     /* 7 - 20:11 */
		32.0f/11.0f,     /* 8 - 32:11 */
		80.0f/33.0f,     /* 9 - 80:33 */
		18.0f/11.0f,     /* 10 - 18:11 */
		15.0f/11.0f,     /* 11 - 15:11 */
		64.0f/33.0f,     /* 12 - 64:33 */
		160.0f/99.0f,    /* 13 - 160:99 */
		4.0f/3.0f,       /* 14 - 4:3 */
		3.0f/2.0f,       /* 15 - 3:2 */
		2.0f            /* 16 - 2:1 */
	};

	if (params->aspect_ratio_idc < sizeof(predefined_aspect_ratios)/sizeof(float)) {
		return predefined_aspect_ratios[params->aspect_ratio_idc];
	}

	return 1.0f;  /* Default to 1:1 for unknown values */
}

/* Get maximum DPB size in bytes */
static inline uint32_t get_dpb_size_bytes(const struct amlvdec_h264_rpm *params) {
	uint32_t mb_size = 384;  /* Size per macroblock in bytes */
	return (uint32_t)params->mb_x_num * params->mb_height * 
		params->max_reference_frame_num_in_mem * mb_size;
}

/* Check if slice is reference */
static inline bool is_reference_slice(const struct amlvdec_h264_rpm *params) {
	return (params->nal_ref_idc != 0);
}
#endif

static const struct {
	uint8_t width;
	uint8_t height;
} chroma_factors[] = {
	{1, 1},  // For chroma_format_idc == 0
	{2, 2},  // For chroma_format_idc == 1
	{2, 1},  // For chroma_format_idc == 2
	{1, 1}   // For chroma_format_idc == 3
};

static inline uint32_t get_picture_width(const struct amlvdec_h264_rpm *dpb) {
	uint8_t chroma_format_idc = dpb->chroma_format_idc & 0x03;
	switch (dpb->profile_idc) {
		case 100: case 110: case 122: case 144:
			break;
		default:
			chroma_format_idc = 1;
	}

	uint8_t format_idx = umin(chroma_format_idc, 3);
	uint8_t sub_width_c = chroma_factors[format_idx].width;
	uint32_t crop_right = sub_width_c * dpb->frame_crop_right_offset;

	return (dpb->mb_width << 4) - crop_right;
}

static inline uint32_t get_picture_height(const struct amlvdec_h264_rpm *dpb) {
	uint8_t chroma_format_idc = dpb->chroma_format_idc & 0x03;
	switch (dpb->profile_idc) {
		case 100: case 110: case 122: case 144:
			break;
		default:
			chroma_format_idc = 1;
	}

	uint8_t format_idx = umin(chroma_format_idc, 3);
	uint8_t sub_height_c = chroma_factors[format_idx].height;

	uint32_t crop_bottom = sub_height_c * dpb->frame_crop_bottom_offset *
		(2 - dpb->frame_mbs_only_flag);

	return (dpb->mb_height << 4) - crop_bottom;
}

void dump_dpb_params(const struct amlvdec_h264_rpm *params);
void dump_dpb(const struct amlvdec_h264_dpb *dpb);
void dump_mmco(const struct amlvdec_h264_mmco *mmco);

#endif /* __AML_VDEC_H264_LMEM_H__ */
