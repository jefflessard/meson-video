#include "amlvdec_vdec.h"

void amlvdec_vdec_start(void)
{
	/* #if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6 */
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_M6) {
		READ_VREG(DOS_SW_RESET0);
		READ_VREG(DOS_SW_RESET0);
		READ_VREG(DOS_SW_RESET0);

		WRITE_VREG(DOS_SW_RESET0, (1 << 12) | (1 << 11));
		WRITE_VREG(DOS_SW_RESET0, 0);

		READ_VREG(DOS_SW_RESET0);
		READ_VREG(DOS_SW_RESET0);
		READ_VREG(DOS_SW_RESET0);
#ifdef TODO
	} else {
		/* #else */
		/* additional cbus dummy register reading for timing control */
		READ_RESET_REG(RESET0_REGISTER);
		READ_RESET_REG(RESET0_REGISTER);
		READ_RESET_REG(RESET0_REGISTER);
		READ_RESET_REG(RESET0_REGISTER);

		WRITE_RESET_REG(RESET0_REGISTER, RESET_VCPU | RESET_CCPU);

		READ_RESET_REG(RESET0_REGISTER);
		READ_RESET_REG(RESET0_REGISTER);
		READ_RESET_REG(RESET0_REGISTER);
#endif
	}
	/* #endif */

	WRITE_VREG(MPSR, 0x0001);
}

void amlvdec_vdec_stop(void)
{
	ulong timeout = jiffies + HZ/10;

	WRITE_VREG(MPSR, 0);
	WRITE_VREG(CPSR, 0);

	while (READ_VREG(IMEM_DMA_CTRL) & 0x8000) {
		if (time_after(jiffies, timeout))
			break;
	}

	timeout = jiffies + HZ/10;
	while (READ_VREG(LMEM_DMA_CTRL) & 0x8000) {
		if (time_after(jiffies, timeout))
			break;
	}

	/* #if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6 */
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_M6) {
		READ_VREG(DOS_SW_RESET0);
		READ_VREG(DOS_SW_RESET0);
		READ_VREG(DOS_SW_RESET0);

		WRITE_VREG(DOS_SW_RESET0, (1 << 12) | (1 << 11));
		WRITE_VREG(DOS_SW_RESET0, 0);

		READ_VREG(DOS_SW_RESET0);
		READ_VREG(DOS_SW_RESET0);
		READ_VREG(DOS_SW_RESET0);
#ifdef TODO
	} else {
		/* #else */
		WRITE_RESET_REG(RESET0_REGISTER, RESET_VCPU | RESET_CCPU);

		/* additional cbus dummy register reading for timing control */
		READ_RESET_REG(RESET0_REGISTER);
		READ_RESET_REG(RESET0_REGISTER);
		READ_RESET_REG(RESET0_REGISTER);
		READ_RESET_REG(RESET0_REGISTER);
#endif
	}
	/* #endif */
} 

void amlvdec_vdec_reset(void)
{
#ifdef TODO
	if ((get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T7) ||
		(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T3) ||
		(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_S5)) {
		/* t7 no dmc req for vdec only */
		vdec_dbus_ctrl(0);
	} else {
		dec_dmc_port_ctrl(0, VDEC_INPUT_TARGET_VLD);
	}
#endif
	/*
	 * 2: assist
	 * 3: vld_reset
	 * 4: vld_part_reset
	 * 5: vfifo reset
	 * 6: iqidct
	 * 7: mc
	 * 8: dblk
	 * 9: pic_dc
	 * 10: psc
	 * 11: mcpu
	 * 12: ccpu
	 * 13: ddr
	 * 14: afifo
	 */
	WRITE_VREG(DOS_SW_RESET0, (1<<3)|(1<<4)|(1<<5)|(1<<7)|(1<<8)|(1<<9));
	WRITE_VREG(DOS_SW_RESET0, 0);

#ifdef TODO
	if ((get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T7) ||
		(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T3) ||
		(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_S5))
		vdec_dbus_ctrl(1);
	else
		dec_dmc_port_ctrl(1, VDEC_INPUT_TARGET_VLD);
#endif
}

void amlvdec_vdec_configure_input(dma_addr_t buf_paddr, u32 buf_size, u32 data_len) {
/* begin vdec_prepare_input */
	/* full reset to HW input */
	WRITE_VREG(VLD_MEM_VIFIFO_CONTROL, 0);
	/* reset VLD fifo for all vdec */
	WRITE_VREG(DOS_SW_RESET0, (1<<5) | (1<<4) | (1<<3));
	WRITE_VREG(DOS_SW_RESET0, 0);

	WRITE_VREG(POWER_CTL_VLD, 1 << 4);

	WRITE_VREG(VLD_MEM_VIFIFO_START_PTR, buf_paddr);
	WRITE_VREG(VLD_MEM_VIFIFO_END_PTR, buf_paddr + buf_size - 8);
	WRITE_VREG(VLD_MEM_VIFIFO_CURR_PTR, buf_paddr);

	WRITE_VREG(VLD_MEM_VIFIFO_CONTROL, 1);
	WRITE_VREG(VLD_MEM_VIFIFO_CONTROL, 0);

	/* set to manual mode */
	WRITE_VREG(VLD_MEM_VIFIFO_BUF_CNTL, 2);

	WRITE_VREG(VLD_MEM_VIFIFO_RP, buf_paddr);
#if 1
	u32 dummy = data_len + VLD_PADDING_SIZE;
	if (dummy >= buf_size)
		dummy -= buf_size;
	WRITE_VREG(VLD_MEM_VIFIFO_WP,
			round_down(buf_paddr + dummy, VDEC_FIFO_ALIGN));
#else
	WRITE_VREG(VLD_MEM_VIFIFO_WP, buf_paddr + data_len - 8);
#endif

	WRITE_VREG(VLD_MEM_VIFIFO_BUF_CNTL, 3);
	WRITE_VREG(VLD_MEM_VIFIFO_BUF_CNTL, 2);

#if 0
#define MEM_VFIFO_BUF_400        BIT(23)
#define MEM_VFIFO_BUF_200        BIT(22)
#define MEM_VFIFO_BUF_100        BIT(21)
#define MEM_VFIFO_BUF_80         BIT(20)
#define MEM_VFIFO_BUF_40         BIT(19)
#define MEM_VFIFO_BUF_20         BIT(18)
#define MEM_VFIFO_16DW           BIT(17)
#define MEM_VFIFO_8DW            BIT(16)
#define MEM_VFIFO_USE_LEVEL      BIT(10)
#define MEM_VFIFO_NO_PARSER      (BIT(3)|BIT(4)|BIT(5)) // (7<<3)
#define MEM_VFIFO_CTRL_EMPTY_EN	 BIT(2)
#define MEM_VFIFO_CTRL_FILL_EN	 BIT(1)
#define MEM_VFIFO_CTRL_INIT      BIT(0)

	// https://github.com/codesnake/uboot-amlogic/blob/master/arch/arm/include/asm/arch-m8/dos_cbus_register.h
	WRITE_VREG(VLD_MEM_VIFIFO_CONTROL,
			MEM_VFIFO_BUF_200 | MEM_VFIFO_16DW |
			MEM_VFIFO_USE_LEVEL |
			MEM_VFIFO_NO_PARSER);
#else
	WRITE_VREG(VLD_MEM_VIFIFO_CONTROL,
			(0x11 << 16) | (1<<10) | (7<<3));
#endif
/* end vdec_prepare_input */
}

void amlvdec_vdec_enable_input(void) {
	/* vdec_enable_input */
	SET_VREG_MASK(VLD_MEM_VIFIFO_CONTROL, (1<<2) | (1<<1));
}
