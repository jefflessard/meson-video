#ifndef __AML_VDEC_MPEG_DPB_H__
#define __AML_VDEC_MPEG_DPB_H__

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>

#define MAX_DPB_FRAMES 16

enum dpb_pic_state : uint8_t {
	DPB_UNUSED,
	DPB_INIT,
	DPB_REF_IN_USE,
	DPB_OUTPUT_READY,
};

typedef struct {
    int frame_num;
    void *buffer;
    void *data;
	bool is_reference;
    uint16_t poc;
	enum dpb_pic_state state;
   
	// Sequence ordering
	uint16_t coded_sequence;
	uint32_t output_order_key;	
} dpb_picture_t;

typedef struct {
	dpb_picture_t frames[MAX_DPB_FRAMES];
    int num_pics;
    int frame_counter;

    // References
    dpb_picture_t *forward_ref;
    dpb_picture_t *backward_ref;

	// Sequence ordering
	uint16_t current_sequence;
} dpb_buffer_t;

void amlvdec_mpeg_dpb_init(dpb_buffer_t *dpb, int size);
dpb_picture_t* amlvdec_mpeg_dpb_new_pic(dpb_buffer_t *dpb, void *buffer, bool is_reference, bool new_sequence, uint16_t gop);
void amlvdec_mpeg_dpb_pic_done(dpb_buffer_t *dpb, dpb_picture_t *pic);
dpb_picture_t* amlvdec_mpeg_dpb_next_output(dpb_buffer_t *dpb, bool flush_all);
void amlvdec_mpeg_dpb_recycle_pic(dpb_buffer_t *dpb, dpb_picture_t *pic);
void amlvdec_mpeg_dpb_dump(dpb_buffer_t *dpb);

#endif /* __AML_VDEC_MPEG_DPB_H__ */
