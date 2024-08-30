# Findings from amlogic code

## Hardware Revision IDs
| CPU ID Macro                  | Value | Note                                                                 |
|-------------------------------|-------|----------------------------------------------------------------------|
| `MESON_CPU_MAJOR_ID_M6`       | `0x16`|                                                                      |
| `MESON_CPU_MAJOR_ID_M6TV`     | `0x17`|                                                                      |
| `MESON_CPU_MAJOR_ID_M6TVL`    | `0x18`|                                                                      |
| `MESON_CPU_MAJOR_ID_M8`       | `0x19`|                                                                      |
| `MESON_CPU_MAJOR_ID_M8M2`     | `0x1D`| Readback value same as M8 (0x19), changed to 0x1D in software        |
| `MESON_CPU_MAJOR_ID_MTVD`     | `0x1A`|                                                                      |
| `MESON_CPU_MAJOR_ID_M8B`      | `0x1B`|                                                                      |
| `MESON_CPU_MAJOR_ID_MG9TV`    | `0x1C`|                                                                      |
| `MESON_CPU_MAJOR_ID_GXBB`     | `0x1F`|                                                                      |
| `MESON_CPU_MAJOR_ID_GXTVBB`   | `0x20`|                                                                      |
| `MESON_CPU_MAJOR_ID_GXL`      | `0x21`|                                                                      |
| `MESON_CPU_MAJOR_ID_GXM`      | `0x22`|                                                                      |
| `MESON_CPU_MAJOR_ID_TXL`      | `0x23`|                                                                      |
| `MESON_CPU_MAJOR_ID_TXLX`     | `0x24`|                                                                      |
| `MESON_CPU_MAJOR_ID_AXG`      | `0x25`|                                                                      |
| `MESON_CPU_MAJOR_ID_GXLX`     | `0x26`|                                                                      |
| `MESON_CPU_MAJOR_ID_TXHD`     | `0x27`|                                                                      |
| `MESON_CPU_MAJOR_ID_G12A`     | `0x28`|                                                                      |
| `MESON_CPU_MAJOR_ID_G12B`     | `0x29`|                                                                      |
| `MESON_CPU_MAJOR_ID_SM1`      | `0x2B`|                                                                      |
| `MESON_CPU_MAJOR_ID_TL1`      | `0x2E`|                                                                      |
| `MESON_CPU_MAJOR_ID_TM2`      | `0x2F`|                                                                      |
| `MESON_CPU_MAJOR_ID_C1`       | `0x30`|                                                                      |
| `MESON_CPU_MAJOR_ID_SC2`      | `0x32`|                                                                      |
| `MESON_CPU_MAJOR_ID_T5`       | `0x34`|                                                                      |
| `MESON_CPU_MAJOR_ID_T5D`      | `0x35`|                                                                      |
| `MESON_CPU_MAJOR_ID_T7`       | `0x36`|                                                                      |
| `MESON_CPU_MAJOR_ID_S4`       | `0x37`|                                                                      |
| `MESON_CPU_MAJOR_ID_T3`       | `0x38`|                                                                      |
| `MESON_CPU_MAJOR_ID_S4D`      | `0x3a`|                                                                      |
| `MESON_CPU_MAJOR_ID_T5W`      | `0x3b`|                                                                      |
| `MESON_CPU_MAJOR_ID_UNKNOWN`  | `0x3c`|                                                                      |


## Register macro to address block mapping

| Macro                | IO BUS       | CODECIO BASE       | Special Cases                                                                 |
|-----------------------------|----------------|----------------------|------------------------------------------------------------------------------------|
| `READ_DMCREG` / `WRITE_DMCREG` | `dmcbus`       | `CODECIO_DMCBUS_BASE` | From `MESON_CPU_MAJOR_ID_GXBB` only                                                                               |
| `READ_AOREG` / `WRITE_AOREG`   | `aobus`        | `CODECIO_AOBUS_BASE`  | None                                                                               |
| `READ_VREG` / `WRITE_VREG`     | `dosbus`       | `CODECIO_DOSBUS_BASE` | None                                                                               |
| `READ_HREG` / `WRITE_HREG` | `cbus`         | `CODECIO_CBUS_BASE`   | `r \| 0x1000` where r is the register address                                                                 |
| `READ_SEC_REG` / `WRITE_SEC_REG` | `TODO`         | `TODO`               | None                                                                               |
| `READ_MPEG_REG` / `WRITE_MPEG_REG` | `cbus`         | `CODECIO_CBUS_BASE`   | None                                                                               |
| `READ_PARSER_REG` / `WRITE_PARSER_REG` | `parsbus`      | `CODECIO_CBUS_BASE`   | Above `MESON_CPU_MAJOR_ID_TXL`, offset = `0xf00`                                     |
| `READ_HHI_REG` / `WRITE_HHI_REG` | `hhibus`       | `CODECIO_CBUS_BASE` | From `MESON_CPU_MAJOR_ID_GXBB`, offset = `-0x1000` on `CODECIO_HIUBUS_BASE`                               |
| `READ_AIU_REG` / `WRITE_AIU_REG` | `aiubus`       | `CODECIO_CBUS_BASE`   | - Above `MESON_CPU_MAJOR_ID_TXL`, offset = `-0x100` <br> - From `MESON_CPU_MAJOR_ID_G12A`, for registers between `AIU_AIFIFO_CTRL` and `AIU_MEM_AIFIFO_MEM_CTL`, offset `-0x80`                                    |
| `READ_DEMUX_REG` / `WRITE_DEMUX_REG` | `demuxbus`     | `CODECIO_CBUS_BASE`   | Above `MESON_CPU_MAJOR_ID_TXL`, offset = `0x200`                                     |
| `READ_RESET_REG` / `WRITE_RESET_REG` | `resetbus`     | `CODECIO_CBUS_BASE`   | Above `MESON_CPU_MAJOR_ID_TXL`, offset = `-0xd00` <br> (except for `MESON_CPU_MAJOR_ID_SC2` other than `T5` or `T5D`)  |
| `READ_EFUSE_REG` / `WRITE_EFUSE_REG` | `efusebus`     | `CODECIO_EFUSE_BASE`  | None                                                                               |
| `READ_VCBUS_REG` / `WRITE_VCBUS_REG` | `vcbus`        | `CODECIO_VCBUS_BASE`  | None                                                                               |


unmapped:
- `IO_VLD_BUS`, `IO_VDEC_BUS`, `IO_VDEC2_BUS`, `IO_HCODEC_BUS`, `IO_HEVC_BUS`
- `IO_VPP_BUS` = `CODECIO_VCBUS_BASE`
- `CODECIO_NOC_BASE` (2019 only)
