#include <linux/sort.h>

#include "amlvdec_mpeg_dpb.h"

static inline void amlvdec_mpeg_dpb_dump_pic(const char *prefix, dpb_picture_t *pic) {
	pr_debug("DPB: %s: frame=%d, seq=%d, poc=%d, ref=%d, state=%d\n",
			prefix,
			pic->frame_num,
			pic->coded_sequence,
			pic->poc,
			pic->is_reference,
			pic->state);
}

void amlvdec_mpeg_dpb_init(dpb_buffer_t *dpb, int size) {
	memset(dpb, 0, sizeof(dpb_buffer_t));
	dpb->num_pics = clamp(size, 0, MAX_DPB_FRAMES);
}

static dpb_picture_t* amlvdec_mpeg_dpb_alloc_pic(dpb_buffer_t *dpb) {

	for (int i = 0; i < dpb->num_pics; i++) {
		if (dpb->frames[i].state == DPB_UNUSED) {
			return &dpb->frames[i];
		}
	}

	pr_err("DPB: DPB full with reference frames");
	return NULL;
}

dpb_picture_t* amlvdec_mpeg_dpb_new_pic(dpb_buffer_t *dpb, void *buffer, bool is_reference, bool new_sequence, uint16_t poc) {
	dpb_picture_t* pic = amlvdec_mpeg_dpb_alloc_pic(dpb);
	if (pic == NULL) {
		return NULL;
	}

	pic->state = DPB_INIT;
	pic->buffer = buffer;
	pic->is_reference = is_reference;
	pic->poc = poc;
	pic->frame_num = dpb->frame_counter++;

	if (new_sequence) {
		dpb->current_sequence++;
	}
	pic->coded_sequence = dpb->current_sequence;
	pic->output_order_key = ((uint32_t) pic->coded_sequence) << 16 | pic->poc;

	amlvdec_mpeg_dpb_dump_pic(__func__, pic);

	return pic;
}

void amlvdec_mpeg_dpb_pic_done(dpb_buffer_t *dpb, dpb_picture_t *pic) {
	if (pic->is_reference) {
		pic->state = DPB_REF_IN_USE;
		if (dpb->backward_ref) {
			dpb->backward_ref->state = DPB_OUTPUT_READY;
		}
		dpb->backward_ref = dpb->forward_ref;
		dpb->forward_ref = pic;
	} else {
		pic->state = DPB_OUTPUT_READY;
	}
}

void amlvdec_mpeg_dpb_recycle_pic(dpb_buffer_t *dpb, dpb_picture_t *pic) {
	if (dpb->forward_ref == pic)
		dpb->forward_ref = NULL;

	if (dpb->backward_ref == pic)
		dpb->backward_ref = NULL;

	pic->state = DPB_UNUSED;
	pic->is_reference = false;
	pic->frame_num = 0;
	pic->poc = 0;
	pic->coded_sequence = 0;
	pic->output_order_key = 0;
	amlvdec_mpeg_dpb_dump_pic(__func__, pic);
}

dpb_picture_t* amlvdec_mpeg_dpb_next_output(dpb_buffer_t *dpb, bool flush_all) {
	// Find the next picture that can be safely output
	for (int i = 0; i < dpb->num_pics; i++) {
		dpb_picture_t *pic = &dpb->frames[i];

		// Skip pictures that are not ready for output
		if ((flush_all && pic->state == DPB_UNUSED) ||
			(!flush_all && pic->state != DPB_OUTPUT_READY)) {
			continue;
		}

		// Check if there are any other DPB pictures with a lower POC
		bool can_output = true;
		for (int j = 0; j < dpb->num_pics; j++) {
			if (i != j &&
					dpb->frames[j].state != DPB_UNUSED &&
					dpb->frames[j].output_order_key < pic->output_order_key) {
				can_output = false;
				break;
			}
		}

		if (can_output) {
			amlvdec_mpeg_dpb_dump_pic(__func__, pic);
			return pic;
		}
	}
	return NULL;
}

void amlvdec_mpeg_dpb_dump(dpb_buffer_t *dpb) {
	for (int i = 0; i < dpb->num_pics; i++) {
		amlvdec_mpeg_dpb_dump_pic(__func__, &dpb->frames[i]);
	}
}
