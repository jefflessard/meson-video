# Findings from amlogic code

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
