#include <linux/compat.h>

#include "amlogic.h"
#include "register.h"

#define VLD_PADDING_SIZE 1024
#define VDEC_FIFO_ALIGN     8


void amlvdec_vdec_start(void);
void amlvdec_vdec_stop(void);
void amlvdec_vdec_reset(void);
void amlvdec_vdec_configure_input(dma_addr_t buf_paddr, u32 buf_size, u32 data_len);
