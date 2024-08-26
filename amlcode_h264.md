# H264 Encoder

## Summary

The driver initializes and configures these hardware components, loads microcode, sets up memory buffers, and handles the encoding process by programming registers and processing interrupts. It provides an interface for userspace applications to submit encoding jobs and retrieve results.

1. Hardware Blocks:
- HCODEC: The main hardware encoder block
- VLC: Variable Length Coding block
- QDCT: Quantization and DCT block
- ME: Motion Estimation block
- IE: Intra Estimation block

2. Clocks:
- HCODEC clock (controlled via HHI_VDEC_CLK_CNTL register)

3. Resets:
- Software reset via DOS_SW_RESET1 register

4. Interrupts:
- IRQ from HCODEC block

5. Key Registers:
- HCODEC_ASSIST_MMC_CTRL1
- ENCODER_STATUS  
- HCODEC_MPSR
- HCODEC_CPSR
- Various ME/IE/QDCT configuration registers

6. Memory:
- Input buffer (DCT buffer)
- Reference buffers  
- Output bitstream buffer
- Microcode buffer
- Various internal buffers (e.g. for ME/IE)

7. DMA:
- Uses dma_buf framework for memory management

8. Canvas:
- Uses canvas for managing frame buffers


## Encoding Process

1. Power-on and Clock Setup:
   - Enable HCODEC power domain (AO_RTI_GEN_PWR_SLEEP0)
   - Configure and enable HCODEC clock (HHI_VDEC_CLK_CNTL)
   - Enable DOS internal clock gating

2. Hardware Reset:
   - Assert and de-assert software reset (DOS_SW_RESET1)
   - Remove HCODEC isolation (AO_RTI_GEN_PWR_ISO0)

3. Memory Power-up:
   - Power up HCODEC memories (DOS_MEM_PD_HCODEC)

4. Interrupt Setup:
   - Clear mailbox interrupt (HCODEC_IRQ_MBOX_CLR)
   - Enable mailbox interrupt (HCODEC_IRQ_MBOX_MASK)

5. Initialization:
   - Configure HCODEC_ASSIST_MMC_CTRL1 (0x32)
   - Set up VLC configuration (HCODEC_VLC_CONFIG)
   - Initialize HCODEC_MPSR and HCODEC_CPSR

6. Microcode Loading:
   - Configure and start IMEM DMA for microcode loading
   - Wait for DMA completion (poll HCODEC_IMEM_DMA_CTRL)

7. Encoder Configuration:
   - Set up frame-related registers (IDR_PIC_ID, FRAME_NUMBER, PIC_ORDER_CNT_LSB, etc.)
   - Configure encoding parameters (LOG2_MAX_PIC_ORDER_CNT_LSB, LOG2_MAX_FRAME_NUM, QPPICTURE, etc.)

8. Memory Buffer Setup:
   - Configure canvas for input, reference, and output buffers
   - Set up DCT buffer (HCODEC_QDCT_MB_START_PTR, HCODEC_QDCT_MB_END_PTR, etc.)
   - Configure VLC buffer (HCODEC_VLC_VB_START_PTR, HCODEC_VLC_VB_END_PTR, etc.)
   - Set up other internal buffers

9. Quantization and VLC Setup:
   - Configure quantization tables
   - Set up VLC parameters (HCODEC_QDCT_VLC_QUANT_CTL_0, HCODEC_QDCT_VLC_QUANT_CTL_1, etc.)

10. ME/IE Parameter Setup:
    - Configure ME/IE weights, SAD controls, MV controls
    - Set up advanced ME parameters

11. QDCT Configuration:
    - Set up QDCT parameters for I and P frames
    - Configure ignore conditions (HCODEC_IGNORE_CONFIG, HCODEC_IGNORE_CONFIG_2)

12. MB Control Setup:
    - Configure HCODEC_QDCT_MB_CONTROL

13. IE/ME/VLC Control Setup:
    - Set up SAD control, IE control, ME skip lines, MV merge control
    - Configure VLC picture size and position

14. Slice Configuration (if applicable):
    - Set up FIXED_SLICE_CFG for multi-slice encoding

15. Version-specific Setup:
    - Configure V5-specific registers if applicable (HCODEC_V5_SIMPLE_MB_CTL, etc.)

16. Rate Control Setup (if CBR enabled):
    - Configure CBR-related registers (H264_ENC_CBR_TABLE_ADDR, H264_ENC_CBR_CTL, etc.)

17. Start Encoding:
    - Set ENCODER_STATUS to the current encoding command
    - Start encoding by writing to HCODEC_MPSR

18. Encoding Process:
    - Hardware performs encoding based on configuration
    - Driver monitors ENCODER_STATUS
    - Process interrupts (IRQ from HCODEC block) as encoding progresses

19. Output Retrieval:
    - Read HCODEC_VLC_TOTAL_BYTES for output size
    - Retrieve encoded data from output buffer

20. Clean-up and Reset:
    - Reset or clear registers for next encoding task
    - Optionally power down or clock gate hardware if no immediate tasks

21. Error Handling:
    - Monitor for timeout or error conditions
    - Implement light reset procedure if necessary (resetting specific blocks)

Throughout the process:
- The driver may dynamically adjust parameters based on encoding mode and enabled features.
- Interrupts are handled to manage the encoding process and data flow.
- Clock gating may be applied to unused blocks for power efficiency.
- Memory buffers are flushed or invalidated as necessary to ensure cache coherency.



## Registers Definition

Certainly! I'll provide a comprehensive documentation of all the registers mentioned in the code. This will be an extensive list, organized by hardware blocks and register categories.

# HCODEC Block

## Control Registers

### HCODEC_MPSR
Summary: Main Processor Status Register, controls the start of encoding operations.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:1 | Reserved | Reserved bits | - |
| 0 | Encoder Start | Starts the encoding process when set | 0: Idle, 1: Start Encoding |

### HCODEC_CPSR
Summary: Co-Processor Status Register, controls the co-processor operations.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | Co-Processor Status | Controls and indicates co-processor status | 0: Reset/Idle |

### HCODEC_IMEM_DMA_CTRL
Summary: Controls IMEM DMA operations.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 15 | DMA Busy | Indicates if IMEM DMA is busy | 0: Not busy, 1: Busy |
| 18:16 | DMA Operation | Controls the DMA operation | 7: Start DMA |
| 31:19 | Reserved | Reserved bits | - |
| 14:0 | Reserved | Reserved bits | - |

### HCODEC_QDCT_MB_CONTROL
Summary: Controls various aspects of macroblock encoding.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 0 | MB Read Buffer Soft Reset | Soft reset for MB read buffer | 0: No reset, 1: Reset |
| 1 | Use Q Delta Quant | Enable use of Q delta quantization | 0: Disabled, 1: Enabled |
| 2 | HCMD Use Q Info | Enable HCMD to use Q info | 0: Disabled, 1: Enabled |
| 3 | HCMD Left Use Prev Info | Enable HCMD left to use previous info | 0: Disabled, 1: Enabled |
| 4 | HCMD Intra Use Q Info | Enable HCMD intra to use Q info | 0: Disabled, 1: Enabled |
| 5 | Use Separate Int Control | Enable separate int control | 0: Disabled, 1: Enabled |
| 6 | MC HCMD Mixed Type | Enable MC HCMD mixed type | 0: Disabled, 1: Enabled |
| 7 | MV Cal Mixed Type | Enable MV calculation mixed type | 0: Disabled, 1: Enabled |
| 8 | P Top Left Mix | Enable P top left mix | 0: Disabled, 1: Enabled |
| 9 | MB Info Soft Reset | Soft reset for MB info | 0: No reset, 1: Reset |
| 10 | MB Read Enable | Enable MB read | 0: Disabled, 1: Enabled |
| 25:11 | Reserved | Reserved bits | - |
| 26 | Zero MC Out Null Non Skipped MB | Zero MC output for null non-skipped MB | 0: Disabled, 1: Enabled |
| 27 | No MC Out Null Non Skipped MB | No MC output for null non-skipped MB | 0: Disabled, 1: Enabled |
| 28 | Ignore T P8x8 | Ignore T P8x8 | 0: Don't ignore, 1: Ignore |
| 29 | IE Start Int Enable | Enable IE start interrupt | 0: Disabled, 1: Enabled |
| 31:30 | Reserved | Reserved bits | - |

### HCODEC_SAD_CONTROL
Summary: Controls SAD (Sum of Absolute Differences) calculations.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 0 | SAD Soft Reset | Soft reset for SAD calculation | 0: No reset, 1: Reset |
| 1 | SAD Enable | Enable SAD calculation | 0: Disabled, 1: Enabled |
| 2 | IE Result Buffer Soft Reset | Soft reset for IE result buffer | 0: No reset, 1: Reset |
| 3 | IE Result Buffer Enable | Enable IE result buffer | 0: Disabled, 1: Enabled |
| 31:4 | Reserved | Reserved bits | - |

### HCODEC_IE_CONTROL
Summary: Controls Intra Estimation operations.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 0 | IE Soft Reset | Soft reset for IE | 0: No reset, 1: Reset |
| 1 | IE Enable | Enable IE | 0: Disabled, 1: Enabled |
| 29:2 | Reserved | Reserved bits | - |
| 30 | Active UL Block | Activate upper-left block | 0: Inactive, 1: Active |
| 31 | Reserved | Reserved bit | - |

## Status Registers

### ENCODER_STATUS
Summary: Indicates the current status of the encoder.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | Encoder Status | Current state of the encoder | 0: ENCODER_IDLE<br>1: ENCODER_SEQUENCE<br>2: ENCODER_PICTURE<br>3: ENCODER_IDR<br>4: ENCODER_NON_IDR<br>5-12: Various DONE states<br>0xFF: ENCODER_ERROR |

### HCODEC_VLC_STATUS_CTRL
Summary: Provides status and control for VLC operations.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | VLC Status/Control | Various VLC status and control bits | Varies based on VLC state |

### HCODEC_QDCT_STATUS_CTRL
Summary: Provides status and control for QDCT operations.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | QDCT Status/Control | Various QDCT status and control bits | Varies based on QDCT state |

### HCODEC_ME_STATUS
Summary: Provides status information for Motion Estimation.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | ME Status | Various ME status bits | Varies based on ME state |

## I/O Registers

### HCODEC_IMEM_DMA_ADR
Summary: Specifies the address for IMEM DMA operations.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | DMA Address | Memory address for IMEM DMA | Any valid memory address |

### HCODEC_IMEM_DMA_COUNT
Summary: Specifies the count for IMEM DMA operations.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | DMA Count | Number of bytes to transfer in DMA operation | Typically set to 0x1000 |

### HCODEC_QUANT_TABLE_DATA
Summary: Used to write quantization table data.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | Quant Table Data | Quantization table values | Varies based on quantization settings |

### HCODEC_VLC_TOTAL_BYTES
Summary: Indicates the total number of bytes produced by VLC.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | Total VLC Bytes | Total number of bytes output by VLC | Varies based on encoded data |

## Configuration Registers

### HCODEC_ASSIST_MMC_CTRL1
Summary: Assists in MMC control operations.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | MMC Control | Various MMC control settings | Typically set to 0x32 |

### HCODEC_VLC_CONFIG
Summary: Configures VLC operations.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 0 | Pop Coeff Even All Zero | Enables popping coefficients even when all are zero | 0: Disabled, 1: Enabled |
| 31:1 | Reserved | Reserved bits | - |

### IDR_PIC_ID
Summary: Stores the IDR picture ID.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | IDR Picture ID | Identifier for IDR pictures | Increments for each IDR frame |

### FRAME_NUMBER
Summary: Stores the current frame number.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | Frame Number | Current frame number | Increments for each frame |

### PIC_ORDER_CNT_LSB
Summary: Stores the picture order count least significant bits.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | Picture Order Count LSB | Least significant bits of picture order count | Varies based on encoding order |

### LOG2_MAX_PIC_ORDER_CNT_LSB
Summary: Stores the log2 of max picture order count LSB.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | Log2 Max POC LSB | Log2 of maximum picture order count LSB | Typically set to 4 |

### LOG2_MAX_FRAME_NUM
Summary: Stores the log2 of max frame number.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | Log2 Max Frame Num | Log2 of maximum frame number | Typically set to 4 |

### ANC0_BUFFER_ID
Summary: Related to buffer ID for ANC0.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | ANC0 Buffer ID | Buffer identifier for ANC0 | Typically set to 0 |

### QPPICTURE
Summary: Stores the quantization parameter for the picture.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | Picture QP | Quantization parameter for the picture | Varies based on desired quality |

### HCODEC_IE_WEIGHT
Summary: Stores weights for Intra Estimation.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 15:0 | I4MB Weight | Weight for I4MB mode | Varies |
| 31:16 | I16MB Weight | Weight for I16MB mode | Varies |

### HCODEC_ME_WEIGHT
Summary: Stores weights for Motion Estimation.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | ME Weight | Weight for Motion Estimation | Varies |

### HCODEC_SAD_CONTROL_0
Summary: Controls SAD calculations for Intra modes.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 15:0 | IE SAD Offset I4 | SAD offset for I4MB | Varies |
| 31:16 | IE SAD Offset I16 | SAD offset for I16MB | Varies |

### HCODEC_SAD_CONTROL_1
Summary: Controls SAD calculations for Inter modes.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 15:0 | ME SAD Offset INTER | SAD offset for INTER prediction | Varies |
| 19:16 | ME SAD Shift INTER | SAD shift for INTER prediction | Typically 1 |
| 23:20 | IE SAD Shift I4 | SAD shift for I4MB | Typically 1 |
| 27:24 | IE SAD Shift I16 | SAD shift for I16MB | Typically 1 |
| 31:28 | Reserved | Reserved bits | - |

### HCODEC_ADV_MV_CTL0
Summary: Controls advanced motion vector operations.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 15:0 | Adv MV 4x4x4 Weight | Weight for 4x4x4 advanced MV | Varies |
| 29:16 | Adv MV 8x8 Weight | Weight for 8x8 advanced MV | Varies |
| 30 | ADV MV Large 8x16 | Enable large 8x16 advanced MV | 0: Disabled, 1: Enabled |
| 31 | ADV MV Large 16x8 | Enable large 16x8 advanced MV | 0: Disabled, 1: Enabled |

### HCODEC_ADV_MV_CTL1
Summary: Additional control for advanced motion vector operations.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 14:0 | Adv MV 16 8 Weight | Weight for 16x8 advanced MV | Varies |
| 15 | ADV MV Large 16x16 | Enable large 16x16 advanced MV | 0: Disabled, 1: Enabled |
| 30:16 | Adv MV 16x16 Weight | Weight for 16x16 advanced MV | Varies |
| 31 | Reserved | Reserved bit | - |

### HCODEC_Q_QUANT_CONTROL
Summary: Controls quantization table address and updates.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 21:0 | Reserved | Reserved bits | - |
| 22 | Quant Table Addr Update | Update quantization table address | 0: No update, 1: Update |
| 31:23 | Quant Table Addr | Quantization table address | Varies |

### HCODEC_QDCT_VLC_QUANT_CTL_0
Summary: Controls VLC quantization parameters.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 5:0 | VLC Quant 0 | VLC quantization parameter 0 | Varies |
| 12:6 | VLC Delta Quant 0 | VLC delta quantization parameter 0 | Varies |
| 18:13 | VLC Quant 1 | VLC quantization parameter 1 | Varies |
| 25:19 | VLC Delta Quant 1 | VLC delta quantization parameter 1 | Varies |
| 31:26 | Reserved | Reserved bits | - |

### HCODEC_QDCT_VLC_QUANT_CTL_1
Summary: Additional control for VLC quantization.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 5:0 | VLC Max Delta Q Pos | Maximum positive delta Q for VLC | Varies |
| 11:6 | VLC Max Delta Q Neg | Maximum negative delta Q for VLC | Varies |
| 31:12 | Reserved | Reserved bits | - |

### HCODEC_VLC_PIC_SIZE
Summary: Stores the picture size for VLC.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 15:0 | Picture Width | Width of the picture in pixels | Varies |
| 31:16 | Picture Height | Height of the picture in pixels | Varies |

### HCODEC_VLC_PIC_POSITION
Summary: Stores the current macroblock position for VLC.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 7:0 | Pic MBX | Picture MB X position | Varies |
| 15:8 | Pic MBY | Picture MB Y position | Varies |
| 31:16 | Pic MB NR | Picture MB number | Varies |

Certainly. I'll continue with the remaining registers.

### HCODEC_QDCT_Q_QUANT_I
Summary: Stores various quantization parameters for I frames.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 5:0 | I Frame QP | Quantization parameter for I frames | 0-51 |
| 7:6 | I Frame QP Div 6 | I frame QP divided by 6 | 0-8 |
| 11:8 | I Frame QP Mod 6 | I frame QP modulo 6 | 0-5 |
| 15:12 | Reserved | Reserved bits | - |
| 21:16 | Chroma QP | Quantization parameter for chroma in I frames | 0-51 |
| 31:22 | Reserved | Reserved bits | - |

### HCODEC_QDCT_Q_QUANT_P
Summary: Stores various quantization parameters for P frames.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 5:0 | P Frame QP | Quantization parameter for P frames | 0-51 |
| 7:6 | P Frame QP Div 6 | P frame QP divided by 6 | 0-8 |
| 11:8 | P Frame QP Mod 6 | P frame QP modulo 6 | 0-5 |
| 15:12 | Reserved | Reserved bits | - |
| 21:16 | Chroma QP | Quantization parameter for chroma in P frames | 0-51 |
| 31:22 | Reserved | Reserved bits | - |

### HCODEC_IGNORE_CONFIG
Summary: Controls various ignore conditions for encoding.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 4:0 | Ignore CAC Coeff 1 | Ignore condition for CAC coefficient 1 | Varies |
| 9:5 | Ignore CAC Coeff 2 | Ignore condition for CAC coefficient 2 | Varies |
| 14:10 | Ignore CAC Coeff Else | Ignore condition for other CAC coefficients | Varies |
| 15 | Ignore CAC Coeff Enable | Enable ignoring CAC coefficients | 0: Disabled, 1: Enabled |
| 20:16 | Ignore LAC Coeff 1 | Ignore condition for LAC coefficient 1 | Varies |
| 25:21 | Ignore LAC Coeff 2 | Ignore condition for LAC coefficient 2 | Varies |
| 30:26 | Ignore LAC Coeff Else | Ignore condition for other LAC coefficients | Varies |
| 31 | Ignore LAC Coeff Enable | Enable ignoring LAC coefficients | 0: Disabled, 1: Enabled |

### HCODEC_IGNORE_CONFIG_2
Summary: Additional ignore configurations for encoding.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 1:0 | Ignore CDC Abs Max Intra | Ignore CDC absolute max for intra | 0-3 |
| 3:2 | Ignore CDC Range Max Intra | Ignore CDC range max for intra | 0-3 |
| 4 | Ignore CDC Only When One Empty Intra | Ignore CDC when one is empty (intra) | 0: Disabled, 1: Enabled |
| 5 | Ignore CDC Only When Empty CAC Intra | Ignore CDC when CAC is empty (intra) | 0: Disabled, 1: Enabled |
| 7:6 | Ignore CDC Abs Max Inter | Ignore CDC absolute max for inter | 0-3 |
| 9:8 | Ignore CDC Range Max Inter | Ignore CDC range max for inter | 0-3 |
| 11 | Ignore CDC Only When One Empty Inter | Ignore CDC when one is empty (inter) | 0: Disabled, 1: Enabled |
| 12 | Ignore CDC Only When Empty CAC Inter | Ignore CDC when CAC is empty (inter) | 0: Disabled, 1: Enabled |
| 13 | Ignore T LAC Coeff Else LE 4 | Ignore T LAC coefficient else <= 4 | 0: Disabled, 1: Enabled |
| 14 | Ignore T LAC Coeff Else LE 3 | Ignore T LAC coefficient else <= 3 | 0: Disabled, 1: Enabled |
| 15 | Ignore CDC Coeff Enable | Enable ignoring CDC coefficients | 0: Disabled, 1: Enabled |
| 20:16 | Ignore T LAC Coeff 1 | Ignore condition for T LAC coefficient 1 | Varies |
| 25:21 | Ignore T LAC Coeff 2 | Ignore condition for T LAC coefficient 2 | Varies |
| 30:26 | Ignore T LAC Coeff Else | Ignore condition for other T LAC coefficients | Varies |
| 31 | Ignore T LAC Coeff Enable | Enable ignoring T LAC coefficients | 0: Disabled, 1: Enabled |

### HCODEC_ME_SKIP_LINE
Summary: Controls line skipping for Motion Estimation.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 5:0 | Step 0 Skip Line | Number of lines to skip in ME step 0 | Varies |
| 11:6 | Step 1 Skip Line | Number of lines to skip in ME step 1 | Varies |
| 17:12 | Step 2 Skip Line | Number of lines to skip in ME step 2 | Varies |
| 23:18 | Step 3 Skip Line | Number of lines to skip in ME step 3 | Varies |
| 31:24 | Reserved | Reserved bits | - |

### HCODEC_ME_MV_MERGE_CTL
Summary: Controls MV (Motion Vector) merging for ME.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 11:0 | ME Merge Min SAD | Minimum SAD for MV merging | Varies |
| 17:12 | ME Merge MV Diff 8 | MV difference threshold for 8x8 blocks | Varies |
| 23:18 | ME Merge MV Diff 16 | MV difference threshold for 16x16 blocks | Varies |
| 24 | ME Merge SAD Enable 8 | Enable SAD-based merging for 8x8 blocks | 0: Disabled, 1: Enabled |
| 25 | ME Merge Flex Enable 8 | Enable flexible merging for 8x8 blocks | 0: Disabled, 1: Enabled |
| 26 | ME Merge Small MV Enable 8 | Enable small MV merging for 8x8 blocks | 0: Disabled, 1: Enabled |
| 27 | ME Merge MV Enable 8 | Enable MV merging for 8x8 blocks | 0: Disabled, 1: Enabled |
| 28 | ME Merge SAD Enable 16 | Enable SAD-based merging for 16x16 blocks | 0: Disabled, 1: Enabled |
| 29 | ME Merge Flex Enable 16 | Enable flexible merging for 16x16 blocks | 0: Disabled, 1: Enabled |
| 30 | ME Merge Small MV Enable 16 | Enable small MV merging for 16x16 blocks | 0: Disabled, 1: Enabled |
| 31 | ME Merge MV Enable 16 | Enable MV merging for 16x16 blocks | 0: Disabled, 1: Enabled |

### HCODEC_ME_STEP0_CLOSE_MV
Summary: Controls close MV detection for ME step 0.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 4:0 | ME Step0 Close MV X | Close MV X threshold for step 0 | Varies |
| 9:5 | ME Step0 Close MV Y | Close MV Y threshold for step 0 | Varies |
| 25:10 | ME Step0 Big SAD | SAD threshold for big motion in step 0 | Varies |
| 31:26 | Reserved | Reserved bits | - |

### HCODEC_ME_SAD_ENOUGH_01
Summary: Sets SAD thresholds for ME steps 0 and 1.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 11:0 | ME SAD Enough 0 | SAD threshold for step 0 | Varies |
| 23:12 | ME SAD Enough 1 | SAD threshold for step 1 | Varies |
| 31:24 | Reserved | Reserved bits | - |

### HCODEC_ME_SAD_ENOUGH_23
Summary: Sets SAD thresholds for ME step 2 and 8x8 advanced MV.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 11:0 | ME SAD Enough 2 | SAD threshold for step 2 | Varies |
| 23:12 | ADV MV 8x8 Enough | SAD threshold for 8x8 advanced MV | Varies |
| 31:24 | Reserved | Reserved bits | - |

### HCODEC_ME_F_SKIP_SAD
Summary: Sets SAD thresholds for forced skip in ME.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 7:0 | Force Skip SAD 0 | SAD threshold for forced skip in step 0 | Varies |
| 15:8 | Force Skip SAD 1 | SAD threshold for forced skip in step 1 | Varies |
| 23:16 | Force Skip SAD 2 | SAD threshold for forced skip in step 2 | Varies |
| 31:24 | Force Skip SAD 3 | SAD threshold for forced skip in step 3 | Varies |

### HCODEC_ME_F_SKIP_WEIGHT
Summary: Sets weights for forced skip in ME.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 7:0 | Force Skip Weight 0 | Weight for forced skip in step 0 | Varies |
| 15:8 | Force Skip Weight 1 | Weight for forced skip in step 1 | Varies |
| 23:16 | Force Skip Weight 2 | Weight for forced skip in step 2 | Varies |
| 31:24 | Force Skip Weight 3 | Weight for forced skip in step 3 | Varies |

### HCODEC_ME_MV_WEIGHT_01
Summary: Sets MV weights for ME steps 0 and 1.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 7:0 | ME MV Pre Weight 0 | MV pre-weight for step 0 | Varies |
| 15:8 | ME MV Step Weight 0 | MV step weight for step 0 | Varies |
| 23:16 | ME MV Pre Weight 1 | MV pre-weight for step 1 | Varies |
| 31:24 | ME MV Step Weight 1 | MV step weight for step 1 | Varies |

### HCODEC_ME_MV_WEIGHT_23
Summary: Sets MV weights for ME steps 2 and 3.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 7:0 | ME MV Pre Weight 2 | MV pre-weight for step 2 | Varies |
| 15:8 | ME MV Step Weight 2 | MV step weight for step 2 | Varies |
| 23:16 | ME MV Pre Weight 3 | MV pre-weight for step 3 | Varies |
| 31:24 | ME MV Step Weight 3 | MV step weight for step 3 | Varies |

### HCODEC_ME_SAD_RANGE_INC
Summary: Sets SAD range increments for ME.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 7:0 | ME SAD Range 0 | SAD range increment for step 0 | Varies |
| 15:8 | ME SAD Range 1 | SAD range increment for step 1 | Varies |
| 23:16 | ME SAD Range 2 | SAD range increment for step 2 | Varies |
| 31:24 | ME SAD Range 3 | SAD range increment for step 3 | Varies |

### HCODEC_V5_SIMPLE_MB_CTL
Summary: Controls simple MB mode in version 5.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 0 | V5 Simple MB Y Enable | Enable simple MB mode for Y | 0: Disabled, 1: Enabled |
| 1 | V5 Simple MB C Enable | Enable simple MB mode for C | 0: Disabled, 1: Enabled |
| 2 | V5 Simple MB Intra Enable | Enable simple MB mode for Intra | 0: Disabled, 1: Enabled |
| 3 | V5 Simple MB Inter 16x16 Enable | Enable simple MB mode for Inter 16x16 | 0: Disabled, 1: Enabled |
| 4 | V5 Simple MB Inter 16 8 Enable | Enable simple MB mode for Inter 16x8/8x16 | 0: Disabled, 1: Enabled |
| 5 | V5 Simple MB Inter 8x8 Enable | Enable simple MB mode for Inter 8x8 | 0: Disabled, 1: Enabled |
| 6 | V5 Simple MB Inter All Enable | Enable simple MB mode for all Inter modes | 0: Disabled, 1: Enabled |
| 7 | V5 Use Small Diff CNT | Use small difference count | 0: Disabled, 1: Enabled |
| 31:8 | Reserved | Reserved bits | - |

### HCODEC_V5_MB_DIFF_SUM
Summary: Stores MB difference sum in version 5.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | MB Diff Sum | Sum of MB differences | Varies |

### HCODEC_V5_SMALL_DIFF_CNT
Summary: Sets small difference count thresholds in version 5.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 15:0 | V5 Small Diff Y | Small difference threshold for Y | Varies |
| 31:16 | V5 Small Diff C | Small difference threshold for C | Varies |

### HCODEC_V5_SIMPLE_MB_DQUANT
Summary: Controls simple MB dequantization in version 5.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | Simple MB Dequant Setting | Dequantization settings for simple MB mode | Varies |

### HCODEC_V5_SIMPLE_MB_ME_WEIGHT
Summary: Sets ME weights for simple MB mode in version 5.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | Simple MB ME Weight | ME weight settings for simple MB mode | Varies |

### HCODEC_QDCT_CONFIG
Summary: General configuration for QDCT (Quantization and DCT).

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 0 | QDCT Enable | Enable QDCT operations | 0: Disabled, 1: Enabled |
| 31:1 | Reserved | Reserved bits | - |

# AO (Always On) Block

## Control Registers

### AO_RTI_GEN_PWR_SLEEP0
Summary: Controls power sleep states for various blocks including HCODEC.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 1:0 | HCODEC Power Sleep | Controls HCODEC power sleep state | 0: Power on, 3: Power off |
| 31:2 | Other Blocks | Controls for other hardware blocks | Varies |

### AO_RTI_GEN_PWR_ISO0
Summary: Controls power isolation for various blocks including HCODEC.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 4 | HCODEC ISO | Controls HCODEC power isolation | 0: No isolation, 1: Isolated |
| 31:0 | Other Blocks ISO | Isolation control for other blocks | Varies |

# HHI Block

## Clock Control Registers

### HHI_VDEC_CLK_CNTL
Summary: Controls the clock for video decoding/encoding operations.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 7:0 | Clock Divider | Sets the clock division factor | Varies based on desired frequency |
| 8 | Clock Enable | Enables/disables the clock | 0: Disabled, 1: Enabled |
| 15:9 | Reserved | Reserved bits | - |
| 23:16 | Clock Source Select | Selects the clock source | Varies based on available sources |
| 24 | VDEC Clock Enable | Enables/disables VDEC clock | 0: Disabled, 1: Enabled |
| 25 | HCODEC Clock Enable | Enables/disables HCODEC clock | 0: Disabled, 1: Enabled |
| 31:26 | Reserved | Reserved bits | - |

# DOS Block

## Configuration Registers

### DOS_SW_RESET1
Summary: Software reset control for various encoder components.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 2 | IQIDCT Reset | Resets IQIDCT module | 0: No reset, 1: Reset |
| 6 | MC Reset | Resets Motion Compensation module | 0: No reset, 1: Reset |
| 7 | DBLK Reset | Resets Deblocking filter module | 0: No reset, 1: Reset |
| 8 | HCODEC Reset | Resets HCODEC module | 0: No reset, 1: Reset |
| 14 | MDEC Reset | Resets MDEC module | 0: No reset, 1: Reset |
| 16 | VLD Reset | Resets VLD module | 0: No reset, 1: Reset |
| 17 | MPEG Reset | Resets MPEG module | 0: No reset, 1: Reset |
| 31:18 | Reserved | Reserved bits | - |
| 1:0 | Reserved | Reserved bits | - |
| 5:3 | Reserved | Reserved bits | - |
| 13:9 | Reserved | Reserved bits | - |
| 15 | Reserved | Reserved bit | - |

### DOS_GCLK_EN0
Summary: Global clock enable control.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 11:0 | Reserved | Reserved bits | - |
| 26:12 | Various Clock Enables | Enables clocks for different modules | 0: Disabled, 1: Enabled for each bit |
| 31:27 | Reserved | Reserved bits | - |

### DOS_MEM_PD_HCODEC
Summary: Controls power down of HCODEC memories.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | Memory Power Down | Controls power down of various HCODEC memories | 0: Powered on, 1: Powered down for each bit |

# CANVAS Block

## Configuration Registers

### CANVAS_INDEX
Summary: Configures the canvas index for various buffers.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | Canvas Index | Index value for canvas configuration | Varies based on buffer type and usage |

# Interrupt Registers

### HCODEC_IRQ_MBOX_CLR
Summary: Clears the mailbox interrupt for HCODEC.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 0 | Clear Interrupt | Clears the mailbox interrupt when written | 0: No effect, 1: Clear interrupt |
| 31:1 | Reserved | Reserved bits | - |

### HCODEC_IRQ_MBOX_MASK
Summary: Masks the mailbox interrupt for HCODEC.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 0 | Interrupt Mask | Masks the mailbox interrupt | 0: Interrupt disabled, 1: Interrupt enabled |
| 31:1 | Reserved | Reserved bits | - |

# H264 Encoder Specific Registers

### H264_ENC_CBR_TABLE_ADDR
Summary: Specifies the address of the CBR (Constant Bit Rate) table.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | CBR Table Address | Physical address of the CBR table | Valid memory address |

### H264_ENC_CBR_MB_SIZE_ADDR
Summary: Specifies the address for CBR macroblock size information.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | CBR MB Size Address | Physical address of CBR MB size data | Valid memory address |

### H264_ENC_CBR_CTL
Summary: Controls CBR encoding parameters.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 15:0 | CBR Long Threshold | Long-term CBR threshold | Varies |
| 23:16 | CBR Long MB Number | Number of MBs for long-term CBR | Varies |
| 27:24 | CBR Short Shift | Short-term CBR shift value | Varies |
| 31:28 | CBR Start Table ID | Starting table ID for CBR | Varies |

### H264_ENC_CBR_REGION_SIZE
Summary: Defines the region size for CBR calculations.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 15:0 | CBR Block Height | Height of CBR block | Varies |
| 31:16 | CBR Block Width | Width of CBR block | Varies |

### FIXED_SLICE_CFG
Summary: Configures fixed slice parameters.

| Bits | Field | Description | Values |
|------|-------|-------------|--------|
| 31:0 | Fixed Slice Configuration | Configuration for fixed slice encoding | Varies based on slice settings |

