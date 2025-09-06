#include "meson-formats.h"
#include "meson-codecs.h"
#include "meson-vcodec.h"

#define MHz (1000000)
#define DECODER_TIMEOUT_MS 5000

#define MC_MAX_SIZE         (16 * SZ_4K)

int vdec1_load_firmware(struct meson_vcodec_core *core, dma_addr_t paddr);
int vdec1_init(struct meson_vcodec_core *core);
int vdec1_release(struct meson_vcodec_core *core);
