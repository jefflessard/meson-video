#ifndef __AML_VDEC_H264_DPB_H__
#define __AML_VDEC_H264_DPB_H__

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>

#include "amlvdec_h264_lmem.h"

// DPB and Reference List Management
#define MAX_DPB_FRAMES 32
#define DPB_SIZE_MARGIN 7

enum ref_list {
	REF_LIST0 = 0,
	REF_LIST1 = 1,
	NUM_REF_LIST,
};

// Memory management operation commands
enum mmco_opcode {
	MMCO_END = 0,
	MMCO_SHORT_TERM_UNUSED = 1,
	MMCO_LONG_TERM_UNUSED = 2,
	MMCO_ASSIGN_LONG_TERM = 3,
	MMCO_MAX_LONG_TERM_IDX = 4,
	MMCO_RESET_ALL = 5,
	MMCO_CURRENT_LONG_TERM = 6,
};

// Reference picture reordering commands
enum reorder_opcode {
	REORDER_SHORT_TERM_DECREASE = 0,
	REORDER_SHORT_TERM_INCREASE = 1,
	REORDER_LONG_TERM = 2,
	REORDER_END = 3,
};

enum dpb_pic_state : uint8_t {
	DPB_UNUSED,
	DPB_INIT,
	DPB_REF_IN_USE,
	DPB_OUTPUT_READY,
};

enum dpb_ref_type : uint8_t {
	DPB_NON_REF,
	DPB_SHORT_TERM,
	DPB_LONG_TERM,
};

typedef struct {
    // Frame identification
    uint16_t frame_num;
    
    // Picture structure info
    enum picture_structure_mmco picture_structure;  // From picture_structure_mmco
	enum slice_type slice_type;
    
    // Reference flags
	bool is_idr;
    
    // POC related
    uint16_t frame_poc;          // Constructed from frame_pic_order_cnt[2]
    uint16_t top_poc;            // From top_field_pic_order_cnt
    uint16_t bottom_poc;         // Derived from delta_pic_order_cnt_bottom
   
	// Sequence ordering
	uint16_t coded_sequence;
	uint32_t output_order_key;	

    // Buffer management
    void *buffer;               // Physical buffer
    int buffer_index;           // Physical buffer index
	enum dpb_pic_state state;
	enum dpb_ref_type ref_type;

	// Long-term reference management
	int long_term_frame_idx;

	/* output management */
	bool is_output;
} dpb_picture_t;

// Structure to encapsulate POC calculation state
typedef struct {
	/* Persistent state (preserved between frames) */
	int last_mmco_has_reset;                     // input: previous MMCO5 flag
	uint8_t last_pic_bottom_field;               // input: previous bottom field flag
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
} poc_state_t;

typedef struct {
	const struct amlvdec_h264_lmem *hw_dpb;
	poc_state_t poc_state;

	dpb_picture_t frames[MAX_DPB_FRAMES];
    int max_frames;

    // Reference list construction
    dpb_picture_t *ref_list0[MAX_REF_LIST_SIZE];
    dpb_picture_t *ref_list1[MAX_REF_LIST_SIZE];
    int ref_list0_size;
    int ref_list1_size;

	// Gap management
	int prev_frame_num;

	// Sequence ordering
	uint16_t current_sequence;
} dpb_buffer_t;

typedef void (*output_pic_callback)(dpb_buffer_t *dpb, dpb_picture_t *pic);

void amlvdec_h264_dpb_init(dpb_buffer_t *dpb, const struct amlvdec_h264_lmem *hw_dpb);
int amlvdec_h264_dpb_adjust_size(dpb_buffer_t *dpb);
bool amlvdec_h264_dpb_is_matching_field(dpb_buffer_t *dpb, dpb_picture_t *pic);
dpb_picture_t* amlvdec_h264_dpb_new_pic(dpb_buffer_t *dpb);
void amlvdec_h264_dpb_pic_done(dpb_buffer_t *dpb, dpb_picture_t *pic);
int amlvdec_h264_dpb_flush_output(dpb_buffer_t *dpb, bool flush_all, output_pic_callback output_pic);
void amlvdec_h264_dpb_dump(dpb_buffer_t *dpb);

// Get colocated reference frame (first frame from ref_list1)
static inline dpb_picture_t* amlvdec_h264_dpb_colocated_ref(dpb_buffer_t *dpb) {
	return (dpb->ref_list1_size > 0) ? dpb->ref_list1[0] : NULL;
}

#endif /* __AML_VDEC_H264_DPB_H__ */
