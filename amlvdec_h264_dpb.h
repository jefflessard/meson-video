#ifndef __AML_VDEC_H264_DPB_H__
#define __AML_VDEC_H264_DPB_H__

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>

#include "amlvdec_h264_lmem.h"

// DPB and Reference List Management
#define MAX_DPB_FRAMES 16
#define MAX_REF_FRAMES 16

typedef struct {
    // Frame identification
    uint16_t frame_num;
    uint16_t pic_order_cnt_lsb;
    
    // Picture structure info
    enum picture_structure_mmco picture_structure;  // From picture_structure_mmco
    
    // Reference flags
    uint16_t nal_ref_idc;       // Upper bits from nal_info_mmco
    uint16_t nal_unit_type;     // Lower bits from nal_info_mmco
    
    // POC related
    int32_t frame_poc;          // Constructed from frame_pic_order_cnt[2]
    int32_t top_poc;            // From top_field_pic_order_cnt
    int32_t bottom_poc;         // Derived from delta_pic_order_cnt_bottom
    
    // Buffer management
    void *buffer;               // Physical buffer
    int buffer_index;           // Physical buffer index
    bool is_reference;          // Active reference flag
    bool is_long_term;          // Long-term reference flag
    bool is_used;               // Slot is in use
} dpb_entry_t;

// Buffer specification structure
typedef struct {
    uint32_t info0;  // Contains structure flags and long-term reference flags
    uint32_t info1;  // Contains top_poc
    uint32_t info2;  // Contains bottom_poc
} buffer_spec_t;

// Structure to encapsulate POC calculation state
typedef struct {
	/* Input parameters from SPS */
//	int pic_order_cnt_type;                      // input: POC calculation mode
//	int log2_max_pic_order_cnt_lsb_minus4;       // input: used to calculate MaxPicOrderCntLsb
//	int num_ref_frames_in_pic_order_cnt_cycle;   // input: used in POC mode 1
//	int offset_for_non_ref_pic;                  // input: used in POC mode 1
//	int offset_for_top_to_bottom_field;          // input: used in POC mode 1
//	int *offset_for_ref_frame;                   // input: array used in POC mode 1
//	short offset_for_ref_frame[128];
//	int max_frame_num;                           // input: maximum frame number

	/* Input parameters from Slice */
//	int idr_flag;                                // input: indicates IDR picture
//	int field_pic_flag;                          // input: indicates field picture
//	int bottom_field_flag;                       // input: indicates bottom field
//	int frame_num;                               // input: frame number
//	int nal_reference_idc;                       // input: nal reference flag
//	uint8_t pic_order_cnt_lsb;                       // input: LSB part of POC
//	int delta_pic_order_cnt_bottom;              // input: bottom field POC delta
//	int delta_pic_order_cnt[2];                  // input: POC deltas

	/* Persistent state (preserved between frames) */
	int last_has_mmco_5;                         // input: previous MMCO5 flag
	uint8_t last_pic_bottom_field;                   // input: previous bottom field flag
	int PrevPicOrderCntMsb;                      // input/output: previous POC MSB
	int PrevPicOrderCntLsb;                      // input/output: previous POC LSB
	int PreviousFrameNum;                        // input/output: previous frame number
	int PreviousFrameNumOffset;                  // input/output: previous frame num offset

	/* Intermediate calculation values */
	int FrameNumOffset;                          // intermediate: current frame number offset
	int AbsFrameNum;                             // intermediate: absolute frame number
	int PicOrderCntCycleCnt;                     // intermediate: POC cycle count
	int FrameNumInPicOrderCntCycle;              // intermediate: frame number in POC cycle
	int ExpectedDeltaPerPicOrderCntCycle;        // intermediate: expected POC delta per cycle
	int ExpectedPicOrderCnt;                     // intermediate: expected POC
	int PicOrderCntMsb;                          // intermediate: POC MSB part

	/* Output values */
//	int ThisPOC;                                 // output: final POC for current picture
//	int toppoc;                                  // output: POC for top field
//	int bottompoc;                               // output: POC for bottom field
//	int framepoc;                                // output: POC for frame
} poc_state_t;

typedef struct {
	const struct amlvdec_h264_lmem *hw_dpb;
	poc_state_t poc_state;

	dpb_entry_t frames[MAX_DPB_FRAMES];
    int num_frames;
    int max_frames;             // From max_dpb_size

    // Reference list construction
    dpb_entry_t *ref_list0[MAX_REF_FRAMES];
    dpb_entry_t *ref_list1[MAX_REF_FRAMES];
    int ref_list0_size;
    int ref_list1_size;

    buffer_spec_t buffer_spec[MAX_DPB_FRAMES];
} dpb_buffer_t;

void amlvdec_h264_dpb_init(dpb_buffer_t *dpb, const struct amlvdec_h264_lmem *hw_dpb);
dpb_entry_t* amlvdec_h264_dpb_frame(dpb_buffer_t *dpb);

// Get colocated reference frame (first frame from ref_list1)
static inline dpb_entry_t* amlvdec_h264_dpb_colocated_ref(dpb_buffer_t *dpb) {
	return (dpb->ref_list1_size > 0) ? dpb->ref_list1[0] : NULL;
}

#endif /* __AML_VDEC_H264_DPB_H__ */
