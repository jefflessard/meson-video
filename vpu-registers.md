# Amlogic Meson SoCs Video Codec

## Instances Offsets

The SoC can handle multiple concurrent video encoding and decoding operations. Specifically, it can encode and decode up to three videos concurrently, leveraging the three independent VPU, DOS and parser instances.

All instances (DOS, Parser, VPU) use the same set of clocks. Each instance does not have its own dedicated clock but shares the common clocks configured for the SoC.

| **Instance** | **Component** | **Base Address** | **Offset Range** | **Interrupts**         |
|--------------|---------------|------------------|------------------|------------------------|
| **1**        | DOS           | 0xC1100000       | 0x0000 - 0x0FFF  | DOS_MBOX_INT1          |
|              | Parser        | 0xC1103000       | 0x3000 - 0x3FFF  | PARSER_INT1            |
|              | VPU           | 0xC1106000       | 0x6000 - 0x6FFF  | VPU_INT1               |
| **2**        | DOS           | 0xC1101000       | 0x1000 - 0x1FFF  | DOS_MBOX_INT2          |
|              | Parser        | 0xC1104000       | 0x4000 - 0x4FFF  | PARSER_INT2            |
|              | VPU           | 0xC1107000       | 0x7000 - 0x7FFF  | VPU_INT2               |
| **3**        | DOS           | 0xC1102000       | 0x2000 - 0x2FFF  | DOS_MBOX_INT3          |
|              | Parser        | 0xC1105000       | 0x5000 - 0x5FFF  | PARSER_INT3            |
|              | VPU           | 0xC1108000       | 0x8000 - 0x8FFF  | VPU_INT3               |

### Instances Interrupts
| **Interrupt**    | **Function**                                                                 | **Registers to Check**               | **Values to Check**                  |
|------------------|------------------------------------------------------------------------------|--------------------------------------|--------------------------------------|
| DOS_MBOX_INTn    | Handles communication and control signals for DOS instance n.                | DOS_MBOXn_STATUS                     | Check for specific status flags.     |
| PARSER_INTn      | Manages parsing operations and signals for Parser instance n.                | PARSERn_STATUS<br>PARSERn_ERROR_STATUS                       | Check for parsing completion or errors. |
| VPU_INTn         | Controls video processing unit operations for VPU instance n.                | VPUn_STATUS                          | Check for processing completion or errors. |


---

## Clocks
| **Clock**       | **Description**                                                                 | **Recommended Configuration**                                                                 |
|-----------------|---------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------|
| HCODEC_CLK      | High-performance video codec clock.                                             | Enable for H.264, MPEG-1/2/4, VC-1 decoding.                                                |
| HEVC_CLK        | HEVC (H.265) codec clock.                                                       | Enable for HEVC decoding and encoding (A311D only).                                         |
| VDEC_CLK        | General video decoder clock.                                                    | Enable for all decoding operations.                                                         |
| VPU_CLK         | Video processing unit clock.                                                    | Enable for all pixel format processing (YUV420, YUV422, YUV444, NV12, NV21).                |
| VENC_CLK        | Video encoder clock.                                                            | Enable for all encoding operations.                                                         |
| DOS_CLK         | Decoder Output Stage clock.                                                     | Enable for all DOS operations.                                                              |
| PARSER_CLK      | Parser clock.                                                                   | Enable for all parsing operations.                                                          |

---

## Decoder Configuration

### Decoder Supported Input Codecs
| **Codec**       | **Register**     | **Bits** | **Values**                                      | **Description**           | **Clocks**                |
|-----------------|------------------|----------|-------------------------------------------------|---------------------------|---------------------------|
| H.264           | VPU_H264_DEC     | [7:0]    | 0: Start decoding<br>1: Stop decoding<br>2: Reset decoder | H.264 video decoding      | HCODEC_CLK<br>VDEC_CLK    |
| H.265 (HEVC)    | VPU_HEVC_DEC     | [7:0]    | 0: Start decoding<br>1: Stop decoding<br>2: Reset decoder | H.265 video decoding      | HEVC_CLK<br>VDEC_CLK      |
| VP9             | VPU_VP9_DEC      | [7:0]    | 0: Start decoding<br>1: Stop decoding<br>2: Reset decoder | VP9 video decoding        | VDEC_CLK                  |
| AV1             | VPU_AV1_DEC      | [7:0]    | 0: Start decoding<br>1: Stop decoding<br>2: Reset decoder | AV1 video decoding        | VDEC_CLK                  |
| VC-1            | VPU_VC1_DEC      | [7:0]    | 0: Start decoding<br>1: Stop decoding<br>2: Reset decoder | VC-1 video decoding       | HCODEC_CLK<br>VDEC_CLK    |
| MPEG-1/2/4      | VPU_MPEG_DEC     | [7:0]    | 0: Start decoding<br>1: Stop decoding<br>2: Reset decoder | MPEG-1/2/4 video decoding | HCODEC_CLK<br>VDEC_CLK    |
| RealVideo       | VPU_REAL_DEC     | [7:0]    | 0: Start decoding<br>1: Stop decoding<br>2: Reset decoder | RealVideo decoding        | VDEC_CLK                  |
| MJPEG           | VPU_MJPEG_DEC    | [7:0]    | 0: Start decoding<br>1: Stop decoding<br>2: Reset decoder | MJPEG video decoding      | VDEC_CLK                  |
| JPEG            | VPU_JPEG_DEC     | [7:0]    | 0: Start decoding<br>1: Stop decoding<br>2: Reset decoder | JPEG image decoding       | VDEC_CLK                  |

### Decoder Supported Output Formats
| **Format**      | **Register**     | **Bits** | **Values**                                      | **Description**           |
|-----------------|------------------|----------|-------------------------------------------------|---------------------------|
| YUV420          | VPU_YUV_PROC     | [1:0]    | 0: YUV420                                       | YUV420 format processing  |
| YUV422          | VPU_YUV_PROC     | [3:2]    | 0: YUV422                                       | YUV422 format processing  |
| YUV444          | VPU_YUV_PROC     | [5:4]    | 0: YUV444                                       | YUV444 format processing  |
| NV12            | VPU_NV12_PROC    | [1:0]    | 0: NV12                                         | NV12 format processing    |
| NV21            | VPU_NV21_PROC    | [3:2]    | 0: NV21                                         | NV21 format processing    |

### Decoder Destination
| **Register**       | **Bits** | **Values**                                      | **Description**           |
|--------------------|----------|-------------------------------------------------|---------------------------|
| VPU_DEC_OUT_CTRL   | [1:0]    | 0: Output to memory<br>1: Output to HDMI<br>2: Output to CVBS | Control decoding output destination |
|                    | [31:2]   | Reserved                                        | Unused bits               |
| VPU_HDMI_CTRL      | [3:2]    | 0: Disable HDMI output<br>1: Enable HDMI output | Control HDMI output       |
|                    | [31:4]   | Reserved                                        | Unused bits               |
| VPU_CVBS_CTRL      | [5:4]    | 0: Disable CVBS output<br>1: Enable CVBS output | Control CVBS output       |
|                    | [31:6]   | Reserved                                        | Unused bits               |


---

## Encoder Configuration

### Encoder Supported Input Formats
| **Format**      | **Register**     | **Bits** | **Values**                                      | **Description**           |
|-----------------|------------------|----------|-------------------------------------------------|---------------------------|
| YUV420          | VPU_VIU_VENC_CTRL| [1:0]    | 0: YUV420                                       | YUV420 format processing  |
| YUV422          | VPU_VIU_VENC_CTRL| [3:2]    | 0: YUV422                                       | YUV422 format processing  |
| YUV444          | VPU_VIU_VENC_CTRL| [5:4]    | 0: YUV444                                       | YUV444 format processing  |
| NV12            | VPU_VIU_VENC_CTRL| [1:0]    | 0: NV12                                         | NV12 format processing    |
| NV21            | VPU_VIU_VENC_CTRL| [3:2]    | 0: NV21                                         | NV21 format processing    |

### Encoder Supported Output Codecs
| **Codec**       | **Register**     | **Bits** | **Values**                                      | **Description**           | **Clocks**                |
|-----------------|------------------|----------|-------------------------------------------------|---------------------------|---------------------------|
| H.264 Encoder   | VPU_H264_ENC     | [7:0]    | 0: Start encoding<br>1: Stop encoding<br>2: Reset encoder | H.264 video encoding      | VENC_CLK<br>HCODEC_CLK    |
| JPEG Encoder    | VPU_JPEG_ENC     | [7:0]    | 0: Start encoding<br>1: Stop encoding<br>2: Reset encoder | JPEG image encoding       | VENC_CLK                  |
| HEVC Encoder (A311D only) | VPU_HEVC_ENC | [7:0] | 0: Start encoding<br>1: Stop encoding<br>2: Reset encoder | HEVC video encoding       | VENC_CLK<br>HEVC_CLK      |

### Encoder Source
| **Register**       | **Bits** | **Values**                                      | **Description**           |
|--------------------|----------|-------------------------------------------------|---------------------------|
| VPU_ENC_IN_CTRL    | [1:0]    | 0: Memory input<br>1: TV input<br>2: HDMI input | Control encoding input source |
|                    | [31:2]   | Reserved                                        | Unused bits               |
| VPU_TVIN_CTRL      | [3:2]    | 0: Disable TV input<br>1: Enable TV input       | Control TV input          |
|                    | [31:4]   | Reserved                                        | Unused bits               |
| VPU_HDMI_IN_CTRL   | [5:4]    | 0: Disable HDMI input<br>1: Enable HDMI input   | Control HDMI input        |
|                    | [31:6]   | Reserved                                        | Unused bits               |

---

## VPU Configuration

| Register       | Bits   | Values                                      | Description           |
|----------------|--------|---------------------------------------------|-----------------------|
| VPU_CTRL       | [1:0]  | 0: Enable VPU<br>1: Disable VPU             | Control VPU operations|
|                | [31:2] | Reserved                                   | Unused bits           |
| VPU_STATUS     | [1:0]  | 0: Idle<br>1: Busy                          | VPU status            |
|                | [31:2] | Reserved                                   | Unused bits           |
| VPU_INT_EN     | [1:0]  | 0: Disable interrupts<br>1: Enable interrupts | VPU interrupt enable  |
|                | [31:2] | Reserved                                   | Unused bits           |
| VPU_INT_STATUS | [1:0]  | 0: No interrupt<br>1: Interrupt pending     | VPU interrupt status  |
|                | [31:2] | Reserved                                   | Unused bits           |
| VPU_RESET      | [1:0]  | 0: No reset<br>1: Reset VPU                 | VPU reset control     |
|                | [31:2] | Reserved                                   | Unused bits           |
| VPU_RESOLUTION | [1:0]  | 0: 720p<br>1: 1080p<br>2: 4K                | Set video resolution  |
|                | [31:2] | Reserved                                   | Unused bits           |
| VPU_HDR_CTRL   | [1:0]  | 0: Disable HDR<br>1: Enable HDR             | HDR control           |
|                | [31:2] | Reserved                                   | Unused bits           |
| VPU_SDR_CTRL   | [1:0]  | 0: Disable SDR<br>1: Enable SDR             | SDR control           |
|                | [31:2] | Reserved                                   | Unused bits           |
| VPU_COLOR_SPACE| [1:0]  | 0: BT.709<br>1: BT.2020                     | Set color space       |
|                | [31:2] | Reserved                                   | Unused bits           |
| VPU_BIT_DEPTH  | [1:0]  | 0: 8-bit<br>1: 10-bit                       | Set bit depth         |
|                | [31:2] | Reserved                                   | Unused bits           |

---

## DOS Configuration

| Register       | Bits   | Values                                      | Description                   |
|----------------|--------|---------------------------------------------|-------------------------------|
| DOS_SW_RESET0  | [1:0]  | 0: Reset VLD<br>1: Reset IQIDCT<br>2: Reset MC | Reset various DOS components  |
|                | [31:2] | Reserved                                   | Unused bits                   |
| DOS_GEN_CTRL   | [1:0]  | 0: Enable DOS<br>1: Disable DOS             | General DOS control           |
|                | [31:2] | Reserved                                   | Unused bits                   |
| DOS_STATUS     | [1:0]  | 0: Idle<br>1: Busy                          | DOS status                    |
|                | [31:2] | Reserved                                   | Unused bits                   |
| DOS_INT_EN     | [1:0]  | 0: Disable interrupts<br>1: Enable interrupts | DOS interrupt enable          |
|                | [31:2] | Reserved                                   | Unused bits                   |
| DOS_INT_STATUS | [1:0]  | 0: No interrupt<br>1: Interrupt pending     | DOS interrupt status          |
|                | [31:2] | Reserved                                   | Unused bits                   |

---

## Parser Configuration

| Register            | Bits   | Values                                      | Description                   |
|---------------------|--------|---------------------------------------------|-------------------------------|
| PARSER_CTRL         | [1:0]  | 0: Enable parser<br>1: Disable parser       | Control parser operations     |
|                     | [31:2] | Reserved                                   | Unused bits                   |
| PARSER_STATUS       | [1:0]  | 0: Idle<br>1: Busy                          | Parser status                 |
|                     | [31:2] | Reserved                                   | Unused bits                   |
| PARSER_INT_EN       | [1:0]  | 0: Disable interrupts<br>1: Enable interrupts | Parser interrupt enable       |
|                     | [31:2] | Reserved                                   | Unused bits                   |
| PARSER_INT_STATUS   | [1:0]  | 0: No interrupt<br>1: Interrupt pending     | Parser interrupt status       |
|                     | [31:2] | Reserved                                   | Unused bits                   |
| PARSER_VIDEO_FORMAT | [3:0]  | 0: H.264<br>1: H.265<br>2: VP9<br>3: AV1    | Set video format for parser   |
|                     | [31:4] | Reserved                                   | Unused bits                   |
| PARSER_HDR_CTRL     | [1:0]  | 0: Disable HDR<br>1: Enable HDR             | HDR control for parser        |
|                     | [31:2] | Reserved                                   | Unused bits                   |
| PARSER_SDR_CTRL     | [1:0]  | 0: Disable SDR<br>1: Enable SDR             | SDR control for parser        |
|                     | [31:2] | Reserved                                   | Unused bits                   |
| PARSER_ENVELOPE     | [7:0]  | 0: Disable envelope<br>1: Enable envelope   | Parser envelope control       |
|                     | [31:8] | Reserved                                   | Unused bits                   |
| PARSER_ERROR_STATUS | [7:0]  | 0: No error<br>1: Bitstream error<br>2: Syntax error<br>3: Buffer overflow | Parser error status           |
|                     | [31:8] | Reserved                                   | Unused bits                   |
