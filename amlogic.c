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

inline u32 meson_dos_read(struct meson_vcodec_core *core, u32 reg)
{
	u32 val=0;

	regmap_read(core->regmaps[BUS_DOS], DOS_REMAP(reg), &val);
	return val;
}

inline void meson_dos_write(struct meson_vcodec_core *core, u32 reg, u32 val)
{
	regmap_write(core->regmaps[BUS_DOS], DOS_REMAP(reg), val);
}

inline void meson_dos_set_bits(struct meson_vcodec_core *core, u32 reg, u32 bits)
{
	regmap_set_bits(core->regmaps[BUS_DOS], DOS_REMAP(reg), bits);
}

void meson_dos_clear_bits(struct meson_vcodec_core *core, u32 reg, u32 bits)
{
	regmap_clear_bits(core->regmaps[BUS_DOS], DOS_REMAP(reg), bits);
}

inline u32 meson_parser_read(struct meson_vcodec_core *core, u32 reg)
{
	u32 val=0;

	regmap_read(core->regmaps[BUS_PARSER], PARSER_REMAP(reg), &val);
	return val;
}

inline void meson_parser_write(struct meson_vcodec_core *core, u32 reg, u32 val)
{
	regmap_write(core->regmaps[BUS_PARSER], PARSER_REMAP(reg), val);
}

inline void meson_parser_set_bits(struct meson_vcodec_core *core, u32 reg, u32 bits)
{
	regmap_set_bits(core->regmaps[BUS_PARSER], PARSER_REMAP(reg), bits);
}

void meson_parser_clear_bits(struct meson_vcodec_core *core, u32 reg, u32 bits)
{
	regmap_clear_bits(core->regmaps[BUS_PARSER], PARSER_REMAP(reg), bits);
}

inline u32 meson_hhi_read(struct meson_vcodec_core *core, u32 reg)
{
	u32 val=0;

	regmap_read(core->regmaps[BUS_HHI], HHI_REMAP(reg), &val);
	return val;
}

inline void meson_hhi_write(struct meson_vcodec_core *core, u32 reg, u32 val)
{
	regmap_write(core->regmaps[BUS_HHI], HHI_REMAP(reg), val);
}

inline void meson_hhi_set_bits(struct meson_vcodec_core *core, u32 reg, u32 bits)
{
	regmap_set_bits(core->regmaps[BUS_HHI], HHI_REMAP(reg), bits);
}

void meson_hhi_clear_bits(struct meson_vcodec_core *core, u32 reg, u32 bits)
{
	regmap_clear_bits(core->regmaps[BUS_HHI], HHI_REMAP(reg), bits);
}

inline u32 meson_ao_read(struct meson_vcodec_core *core, u32 reg)
{
	u32 val=0;

	regmap_read(core->regmaps[BUS_AO], AO_REMAP(reg), &val);
	return val;
}

inline void meson_ao_write(struct meson_vcodec_core *core, u32 reg, u32 val)
{
	regmap_write(core->regmaps[BUS_AO], AO_REMAP(reg), val);
}

inline void meson_ao_set_bits(struct meson_vcodec_core *core, u32 reg, u32 bits)
{
	regmap_set_bits(core->regmaps[BUS_AO], AO_REMAP(reg), bits);
}

void meson_ao_clear_bits(struct meson_vcodec_core *core, u32 reg, u32 bits)
{
	regmap_clear_bits(core->regmaps[BUS_AO], AO_REMAP(reg), bits);
}

inline void WRITE_MPEG_REG(u32 reg, u32 val) {
	meson_dos_write(MESON_VCODEC_CORE, reg, val);
}

inline u32 READ_MPEG_REG(u32 reg) {
	return meson_dos_read(MESON_VCODEC_CORE, reg);
}

inline void WRITE_HREG(u32 reg, u32 val) {
	meson_dos_write(MESON_VCODEC_CORE, reg, val);
}

inline u32 READ_HREG(u32 reg) {
	return meson_dos_read(MESON_VCODEC_CORE, reg);
}

inline void WRITE_VREG(u32 reg, u32 val) {
	meson_dos_write(MESON_VCODEC_CORE, reg, val);
}

inline u32 READ_VREG(u32 reg) {
	return meson_dos_read(MESON_VCODEC_CORE, reg);
}

inline void WRITE_HHI(u32 reg, u32 val) {
	meson_hhi_write(MESON_VCODEC_CORE, reg, val);
}

inline u32 READ_HHI(u32 reg) {
	return meson_hhi_read(MESON_VCODEC_CORE, reg);
}

inline void WRITE_AOREG(u32 reg, u32 val) {
	meson_ao_write(MESON_VCODEC_CORE, reg, val);
}

inline u32 READ_AOREG(u32 reg) {
	return meson_ao_read(MESON_VCODEC_CORE, reg);
}

inline s32 hcodec_hw_reset(void) {
	return meson_vcodec_reset(MESON_VCODEC_CORE, RESET_HCODEC);
}
