#include "decoder_common.h"
#include "register.h"

#define MC_AMRISC_SIZE      (SZ_4K)

int vdec1_load_firmware(struct meson_vcodec_core *core, dma_addr_t paddr) {
	int ret;

	meson_dos_write(core, MPSR, 0);
	meson_dos_write(core, CPSR, 0);

#if 0
	meson_dos_clear_bits(core, MDEC_PIC_DC_CTRL, BIT(31));
#endif

	meson_dos_write(core, IMEM_DMA_ADR, paddr);
	meson_dos_write(core, IMEM_DMA_COUNT, MC_AMRISC_SIZE);
	meson_dos_write(core, IMEM_DMA_CTRL, (0x8000 | (7 << 16)));

	bool is_dma_complete(void) {
		return !(meson_dos_read(core, IMEM_DMA_CTRL) & 0x8000);
	}
	return read_poll_timeout(
			is_dma_complete, ret, ret, 10000, 1000000, true);
}

int vdec1_init(struct meson_vcodec_core *core)
{
	int ret;

	/* Configure the vdec clk to the maximum available */
	ret = meson_vcodec_clk_prepare(core, CLK_VDEC1, 667 * MHz);
	if (ret) {
		dev_err(core->dev, "Failed to enable VDEC1 clock\n");
		return ret;
	}

	/* Powerup VDEC1 & Remove VDEC1 ISO */
	ret = meson_vcodec_pwrc_on(core, PWRC_VDEC1);
	if (ret) {
		dev_err(core->dev, "Failed to power on VDEC1\n");
		goto disable_clk;
	}

	usleep_range(10, 20);

	/* Reset VDEC1 */
	meson_dos_write(core, DOS_SW_RESET0, 0xfffffffc);
	meson_dos_write(core, DOS_SW_RESET0, 0x00000000);

	/* Reset DOS top registers */
	meson_dos_write(core, DOS_VDEC_MCRCC_STALL_CTRL, 0);

	meson_dos_write(core, GCLK_EN, 0x3ff);

#ifdef MDEC
	meson_dos_clear_bits(core, MDEC_PIC_DC_CTRL, BIT(31));
#endif

	return 0;

//disable_vdec1:
//pwrc_off:
//	meson_vcodec_pwrc_off(core, PWRC_VDEC1);
disable_clk:
	meson_vcodec_clk_unprepare(core, CLK_VDEC1);
	return ret;
}

int vdec1_release(struct meson_vcodec_core *core)
{
	meson_dos_write(core, MPSR, 0);
	meson_dos_write(core, CPSR, 0);
	meson_dos_write(core, VDEC_ASSIST_MBOX1_MASK, 0);

	meson_dos_write(core, DOS_SW_RESET0, BIT(12) | BIT(11));
	meson_dos_write(core, DOS_SW_RESET0, 0);
	meson_dos_read(core, DOS_SW_RESET0);

	meson_vcodec_pwrc_off(core, PWRC_VDEC1);
	meson_vcodec_clk_unprepare(core, CLK_VDEC1);

	return 0;
}

