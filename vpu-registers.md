# Amlogic Meson SoCs Video Codec

## Components Overview

### Power Domains, Resets and Clocks
| Component | Power Domain | Reset Control | Clock(s) | Description |
|-----------------|--------------------|--------------------|-------------------|-----------------------------------------------------------------------------|
| VPU (Video Processing Unit) | VPU Power Domain | VPU_RESET | VPU_CLK | Manages video processing tasks, including scaling, color space conversion, and blending. |
| DOS (Decoder Output Stage) | DOS Power Domain | DOS_RESET | DOS_CLK, VDEC_CLK, HEVC_CLK, HCODEC_CLK | Handles video decoding tasks for various codecs, including MPEG, H.264, HEVC, and VP9. |
| Parser | DOS Power Domain | DOS_PARSER_RESET | PARSER_CLK | Processes bitstreams before decoding, supporting various video formats. |
| Encoder | Encoder Power Domain | ENCODER_RESET | VENC_CLK | Manages video encoding tasks, converting raw video data into compressed formats. |

### Clocks Description
| **Clock** | **Description** | **Recommended Configuration** |
|-----------------|---------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------|
| HCODEC_CLK | High-performance video codec clock. | Enable for H.264, MPEG-1/2/4, VC-1 decoding. |
| HEVC_CLK | HEVC (H.265) codec clock. | Enable for HEVC decoding and encoding (A311D only). |
| VDEC_CLK | General video decoder clock. | Enable for all decoding operations. |
| VPU_CLK | Video processing unit clock. | Enable for all pixel format processing (YUV420, YUV422, YUV444, NV12, NV21). |
| VENC_CLK | Video encoder clock. | Enable for all encoding operations. |
| DOS_CLK | Decoder Output Stage clock. | Enable for all DOS operations. |
| PARSER_CLK | Parser clock. | Enable for all parsing operations. |

### Interrupts 
| Interrupt | Value | Function Description |
|------------------------|-------|--------------------------------------------------------------------------------------|
| INT_DOS_MAILBOX_0 | 75 | Used for handling commands and status updates between the CPU and the DOS hardware. This interrupt ensures that the CPU is aware of the current state and any changes in the DOS hardware. Aliased as INT_VDEC2. |
| INT_DOS_MAILBOX_1 | 76 | Manages data flow and buffer control within the DOS. This interrupt is crucial for maintaining the integrity and efficiency of data processing by controlling the flow of data and managing buffer states. Aliased as INT_VDEC. |
| INT_DOS_MAILBOX_2 | 77 | Handles error reporting and recovery mechanisms in the DOS. This interrupt is responsible for alerting the CPU to any errors in the DOS hardware and initiating recovery procedures to maintain system stability. |
| INT_VIU_VSYNC | 35 | Triggers on vertical synchronization signal for video input unit (VIU). This interrupt is essential for synchronizing video frames to ensure smooth playback. |
| INT_DEMUX | 55 | Manages interrupts from the demultiplexer, which separates multiplexed data streams. This interrupt helps in processing and routing different data streams efficiently. |
| INT_DEMUX_1 | 37 | Manages interrupts from the first demultiplexer unit. This interrupt is used for handling specific data streams processed by the first demultiplexer. |
| INT_DEMUX_2 | 85 | Manages interrupts from the second demultiplexer unit. This interrupt is used for handling specific data streams processed by the second demultiplexer. |
| INT_ASYNC_FIFO_FILL | 50 | Indicates that the asynchronous FIFO buffer is filled. This interrupt ensures that the CPU can process the data in the FIFO buffer before it overflows. |
| INT_ASYNC_FIFO_FLUSH | 51 | Indicates that the asynchronous FIFO buffer has been flushed. This interrupt ensures that the CPU is aware that the buffer is empty and ready for new data. |
| INT_ASYNC_FIFO2_FILL | 56 | Indicates that the second asynchronous FIFO buffer is filled. This interrupt ensures that the CPU can process the data in the second FIFO buffer before it overflows. |
| INT_ASYNC_FIFO2_FLUSH | 57 | Indicates that the second asynchronous FIFO buffer has been flushed. This interrupt ensures that the CPU is aware that the second buffer is empty and ready for new data. |
| INT_PARSER | 64 | Signals the CPU when the parser has processed a data packet or encountered an error. This interrupt ensures that the CPU can promptly handle parsed data or address any issues that arise during parsing. |


---

## Decoder Configuration

### Decoder Supported Input Codecs
| **Codec** | **Clocks** |
|-----------------|---------------------------|
| H.264 | HCODEC_CLK<br>VDEC_CLK |
| H.265 (HEVC) |  HEVC_CLK<br>VDEC_CLK |
| VP9 | VDEC_CLK |
| AV1 | VDEC_CLK |
| VC-1 | HCODEC_CLK<br>VDEC_CLK |
| MPEG-1/2/4 | HCODEC_CLK<br>VDEC_CLK |
| RealVideo | VDEC_CLK |
| MJPEG | VDEC_CLK |
| JPEG | VDEC_CLK |

### Decoder Supported Output Formats
| **Format** |
|-----------------|
| YUV420 |
| YUV422 |
| YUV444 |
| NV12 |
| NV21 |

### Decoder Destination
- HDMI
- CVBS
- Memory


---

## Encoder Configuration

### Encoder Supported Input Formats
| **Format** |
|-----------------|
| YUV420 |
| YUV422 |
| YUV444 |
| NV12 |
| NV21 |


### Encoder Supported Output Codecs
| **Codec** | **Register** | **Bits** | **Values** | **Description** | **Clocks** |
|-----------------|------------------|----------|-------------------------------------------------|---------------------------|---------------------------|
| H.264 Encoder | VPU_H264_ENC | [7:0] | 0: Start encoding<br>1: Stop encoding<br>2: Reset encoder | H.264 video encoding | VENC_CLK<br>HCODEC_CLK |
| JPEG Encoder | VPU_JPEG_ENC | [7:0] | 0: Start encoding<br>1: Stop encoding<br>2: Reset encoder | JPEG image encoding | VENC_CLK |
| HEVC Encoder (A311D only) | VPU_HEVC_ENC | [7:0] | 0: Start encoding<br>1: Stop encoding<br>2: Reset encoder | HEVC video encoding | VENC_CLK<br>HEVC_CLK |

### Encoder Source
- HDMI-in
- TVIN
- Memory


---
# *TO BE REVIEWED*
---


## VPU Configuration

| Register | Bits | Values | Description |
|----------------|--------|---------------------------------------------|-----------------------|
| VPU_CTRL | [1:0] | 0: Enable VPU<br>1: Disable VPU | Control VPU operations|
| | [31:2] | Reserved | Unused bits |
| VPU_STATUS | [1:0] | 0: Idle<br>1: Busy | VPU status |
| | [31:2] | Reserved | Unused bits |
| VPU_INT_EN | [1:0] | 0: Disable interrupts<br>1: Enable interrupts | VPU interrupt enable |
| | [31:2] | Reserved | Unused bits |
| VPU_INT_STATUS | [1:0] | 0: No interrupt<br>1: Interrupt pending | VPU interrupt status |
| | [31:2] | Reserved | Unused bits |
| VPU_RESET | [1:0] | 0: No reset<br>1: Reset VPU | VPU reset control |
| | [31:2] | Reserved | Unused bits |
| VPU_RESOLUTION | [1:0] | 0: 720p<br>1: 1080p<br>2: 4K | Set video resolution |
| | [31:2] | Reserved | Unused bits |
| VPU_HDR_CTRL | [1:0] | 0: Disable HDR<br>1: Enable HDR | HDR control |
| | [31:2] | Reserved | Unused bits |
| VPU_SDR_CTRL | [1:0] | 0: Disable SDR<br>1: Enable SDR | SDR control |
| | [31:2] | Reserved | Unused bits |
| VPU_COLOR_SPACE| [1:0] | 0: BT.709<br>1: BT.2020 | Set color space |
| | [31:2] | Reserved | Unused bits |
| VPU_BIT_DEPTH | [1:0] | 0: 8-bit<br>1: 10-bit | Set bit depth |
| | [31:2] | Reserved | Unused bits |

---

## DOS Configuration

| Register | Bits | Values | Description |
|----------------|--------|---------------------------------------------|-------------------------------|
| DOS_SW_RESET0 | [1:0] | 0: Reset VLD<br>1: Reset IQIDCT<br>2: Reset MC | Reset various DOS components |
| | [31:2] | Reserved | Unused bits |
| DOS_GEN_CTRL | [1:0] | 0: Enable DOS<br>1: Disable DOS | General DOS control |
| | [31:2] | Reserved | Unused bits |
| DOS_STATUS | [1:0] | 0: Idle<br>1: Busy | DOS status |
| | [31:2] | Reserved | Unused bits |
| DOS_INT_EN | [1:0] | 0: Disable interrupts<br>1: Enable interrupts | DOS interrupt enable |
| | [31:2] | Reserved | Unused bits |
| DOS_INT_STATUS | [1:0] | 0: No interrupt<br>1: Interrupt pending | DOS interrupt status |
| | [31:2] | Reserved | Unused bits |
| DOS_MBOXn_STATUS | [0:7] | 0: Idle<br>1: Busy<br>2: Error | DOS mailbox status |
| | [31:8] | Reserved | Unused bits |

---

## Parser Configuration

| Register | Bits | Values | Description |
|---------------------|--------|---------------------------------------------|-------------------------------|
| PARSER_CTRL | [1:0] | 0: Enable parser<br>1: Disable parser | Control parser operations |
| | [31:2] | Reserved | Unused bits |
| PARSER_STATUS | [0:15] | 0: Idle<br>1: Busy<br>2: Error | Parsing status |
| | [31:16] | Reserved | Unused bits |
| PARSER_INT_EN | [1:0] | 0: Disable interrupts<br>1: Enable interrupts | Parser interrupt enable |
| | [31:2] | Reserved | Unused bits |
| PARSER_INT_STATUS | [1:0] | 0: No interrupt<br>1: Interrupt pending | Parser interrupt status |
| | [31:2] | Reserved | Unused bits |
| PARSER_VIDEO_FORMAT | [3:0] | 0: H.264<br>1: H.265<br>2: VP9<br>3: AV1 | Set video format for parser |
| | [31:4] | Reserved | Unused bits |
| PARSER_HDR_CTRL | [1:0] | 0: Disable HDR<br>1: Enable HDR | HDR control for parser |
| | [31:2] | Reserved | Unused bits |
| PARSER_SDR_CTRL | [1:0] | 0: Disable SDR<br>1: Enable SDR | SDR control for parser |
| | [31:2] | Reserved | Unused bits |
| PARSER_ENVELOPE | [7:0] | 0: Disable envelope<br>1: Enable envelope | Parser envelope control |
| | [31:8] | Reserved | Unused bits |
| PARSER_ERROR_STATUS | [0:7] | 0: No error<br>1: Buffer overflow<br>2: Timeout | Parsing error status |
| | [31:8] | Reserved | Unused bits |
