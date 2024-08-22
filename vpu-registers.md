### Decoder Supported Codecs and Formats

| Codec/Format | Register      | Bits   | Values                                      | Description           |
|--------------|---------------|--------|---------------------------------------------|-----------------------|
| H.264        | VPU_H264_DEC  | [7:0]  | 0: Start decoding<br>1: Stop decoding<br>2: Reset decoder | H.264 video decoding  |
|              |               | [31:8] | Reserved                                   | Unused bits           |
| H.265 (HEVC) | VPU_HEVC_DEC  | [7:0]  | 0: Start decoding<br>1: Stop decoding<br>2: Reset decoder | H.265 video decoding  |
|              |               | [31:8] | Reserved                                   | Unused bits           |
| VP9          | VPU_VP9_DEC   | [7:0]  | 0: Start decoding<br>1: Stop decoding<br>2: Reset decoder | VP9 video decoding    |
|              |               | [31:8] | Reserved                                   | Unused bits           |
| AV1          | VPU_AV1_DEC   | [7:0]  | 0: Start decoding<br>1: Stop decoding<br>2: Reset decoder | AV1 video decoding    |
|              |               | [31:8] | Reserved                                   | Unused bits           |
| VC-1         | VPU_VC1_DEC   | [7:0]  | 0: Start decoding<br>1: Stop decoding<br>2: Reset decoder | VC-1 video decoding   |
|              |               | [31:8] | Reserved                                   | Unused bits           |
| MPEG-1/2/4   | VPU_MPEG_DEC  | [7:0]  | 0: Start decoding<br>1: Stop decoding<br>2: Reset decoder | MPEG-1/2/4 video decoding |
|              |               | [31:8] | Reserved                                   | Unused bits           |
| RealVideo    | VPU_REAL_DEC  | [7:0]  | 0: Start decoding<br>1: Stop decoding<br>2: Reset decoder | RealVideo decoding    |
|              |               | [31:8] | Reserved                                   | Unused bits           |
| MJPEG        | VPU_MJPEG_DEC | [7:0]  | 0: Start decoding<br>1: Stop decoding<br>2: Reset decoder | MJPEG video decoding  |
|              |               | [31:8] | Reserved                                   | Unused bits           |
| YUV420       | VPU_YUV_PROC  | [1:0]  | 0: YUV420                                  | YUV420 format processing |
|              |               | [31:2] | Reserved                                   | Unused bits           |
| YUV422       | VPU_YUV_PROC  | [3:2]  | 0: YUV422                                  | YUV422 format processing |
|              |               | [31:4] | Reserved                                   | Unused bits           |
| YUV444       | VPU_YUV_PROC  | [5:4]  | 0: YUV444                                  | YUV444 format processing |
|              |               | [31:6] | Reserved                                   | Unused bits           |
| NV12         | VPU_NV12_PROC | [1:0]  | 0: NV12                                    | NV12 format processing |
|              |               | [31:2] | Reserved                                   | Unused bits           |
| NV21         | VPU_NV21_PROC | [3:2]  | 0: NV21                                    | NV21 format processing |
|              |               | [31:4] | Reserved                                   | Unused bits           |
| JPEG         | VPU_JPEG_DEC  | [7:0]  | 0: Start decoding<br>1: Stop decoding<br>2: Reset decoder | JPEG image decoding   |
|              |               | [31:8] | Reserved                                   | Unused bits           |


### Encoder Supported Codecs and Formats

| Codec/Format   | Register      | Bits   | Values                                      | Description           |
|----------------|---------------|--------|---------------------------------------------|-----------------------|
| H.264 Encoder  | VPU_H264_ENC  | [7:0]  | 0: Start encoding<br>1: Stop encoding<br>2: Reset encoder | H.264 video encoding  |
|                |               | [31:8] | Reserved                                   | Unused bits           |
| JPEG Encoder   | VPU_JPEG_ENC  | [7:0]  | 0: Start encoding<br>1: Stop encoding<br>2: Reset encoder | JPEG image encoding   |
|                |               | [31:8] | Reserved                                   | Unused bits           |



### VPU Configuration Registers

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


### DOS Registers

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


### Parser Registers

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
