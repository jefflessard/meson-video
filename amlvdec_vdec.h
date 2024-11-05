#include <linux/compat.h>

#include "amlogic.h"
#include "register.h"

#define ASSIST_MBOX1_CLR_REG VDEC_ASSIST_MBOX1_CLR_REG
#define ASSIST_MBOX1_MASK    VDEC_ASSIST_MBOX1_MASK
#define ASSIST_MBOX1_IRQ_REG VDEC_ASSIST_MBOX1_IRQ_REG

#define VLD_PADDING_SIZE 1024
#define VDEC_FIFO_ALIGN     8


void amlvdec_vdec_start(void);
void amlvdec_vdec_stop(void);
void amlvdec_vdec_reset(void);
void amlvdec_vdec_configure_input(dma_addr_t buf_paddr, u32 buf_size, u32 data_len);
void amlvdec_vdec_enable_input(void);
