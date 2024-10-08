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

#include <linux/regmap.h>

#include "amlogic.h"
#include "meson-vcodec.h"

struct meson_vcodec_core *MESON_VCODEC_CORE;

inline enum AM_MESON_CPU_MAJOR_ID get_cpu_major_id(void) {
	return MESON_VCODEC_CORE->platform_specs->platform_id;
}

inline void WRITE_MPEG_REG(u32 reg, u32 val) {
	regmap_write(MESON_VCODEC_CORE->regmaps[BUS_DOS], reg << 2, val);
}

inline u32 READ_MPEG_REG(u32 reg) {
	u32 val=0;

	regmap_read(MESON_VCODEC_CORE->regmaps[BUS_DOS], reg << 2, &val);
	return val;
}

inline void WRITE_HREG(u32 reg, u32 val) {
	regmap_write(MESON_VCODEC_CORE->regmaps[BUS_DOS], reg << 2, val);
}

inline u32 READ_HREG(u32 reg) {
	u32 val=0;

	regmap_read(MESON_VCODEC_CORE->regmaps[BUS_DOS], reg << 2, &val);
	return val;
}

inline void WRITE_VREG(u32 reg, u32 val) {
	regmap_write(MESON_VCODEC_CORE->regmaps[BUS_DOS], reg << 2, val);
}

inline u32 READ_VREG(u32 reg) {
	u32 val=0;

	regmap_read(MESON_VCODEC_CORE->regmaps[BUS_DOS], reg << 2, &val);
	return val;
}

inline void WRITE_HHI_REG(u32 reg, u32 val) {
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_GXBB)
	   reg -= 0x1000;

	regmap_write(MESON_VCODEC_CORE->regmaps[BUS_HHI], reg << 2, val);
}

inline u32 READ_HHI_REG(u32 reg) {
	u32 val=0;

	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_GXBB)
	   reg -= 0x1000;

	regmap_read(MESON_VCODEC_CORE->regmaps[BUS_HHI], reg << 2, &val);
	return val;
}

inline void WRITE_AOREG(u32 reg, u32 val) {
	regmap_write(MESON_VCODEC_CORE->regmaps[BUS_AO], reg, val);
}

inline u32 READ_AOREG(u32 reg) {
	u32 val=0;
	regmap_read(MESON_VCODEC_CORE->regmaps[BUS_AO], reg, &val);
	return val;
}

inline s32 hcodec_hw_reset(void) {
	return meson_vcodec_reset(MESON_VCODEC_CORE, RESET_HCODEC);
}
