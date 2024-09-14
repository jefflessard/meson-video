/*
 * Copyright (C) 2024 Jean-François Lessard
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "amlogic.h"
#include "meson-vcodec.h"

struct meson_vcodec_core *MESON_VCODEC_CORE;

inline enum AM_MESON_CPU_MAJOR_ID get_cpu_major_id(void) {
	return MESON_VCODEC_CORE->platform_specs->platform_id;
}

inline void WRITE_HREG(u32 reg, u32 val) {
	meson_vcodec_reg_write(MESON_VCODEC_CORE, DOS_BASE, reg << 2, val);
}

inline u32 READ_HREG(u32 reg) {
	return meson_vcodec_reg_read(MESON_VCODEC_CORE, DOS_BASE, reg << 2);
}

inline void WRITE_VREG(u32 reg, u32 val) {
	meson_vcodec_reg_write(MESON_VCODEC_CORE, DOS_BASE, reg << 2, val);
}

inline u32 READ_VREG(u32 reg) {
	return meson_vcodec_reg_read(MESON_VCODEC_CORE, DOS_BASE, reg << 2);
}

inline s32 hcodec_hw_reset(void) {
	return meson_vcodec_reset(MESON_VCODEC_CORE, RESET_HCODEC);
}
