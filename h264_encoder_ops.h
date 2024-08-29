#ifndef __AVC_ENCODER_HW_OPS_H__
#define __AVC_ENCODER_HW_OPS_H__

#include <linux/types.h>

/* Redefined constants and structures */
#define HCODEC_ASSIST_AMR1_INT0 0x2500
#define HCODEC_ASSIST_AMR1_INT1 0x2501
#define HCODEC_ASSIST_AMR1_INT3 0x2503
#define HCODEC_MPSR 0x2600
#define HCODEC_CPSR 0x2601
#define HCODEC_IMEM_DMA_CTRL 0x2602
#define HCODEC_VLC_TOTAL_BYTES 0x2603
#define HCODEC_VLC_CONFIG 0x2604
#define HCODEC_VLC_VB_START_PTR 0x2605
#define HCODEC_VLC_VB_END_PTR 0x2606
#define HCODEC_VLC_VB_WR_PTR 0x2607
#define HCODEC_VLC_VB_SW_RD_PTR 0x2608
#define HCODEC_VLC_VB_CONTROL 0x2609
#define HCODEC_VLC_VB_MEM_CTL 0x260a
#define HCODEC_QDCT_MB_START_PTR 0x260b
#define HCODEC_QDCT_MB_END_PTR 0x260c
#define HCODEC_QDCT_MB_WR_PTR 0x260d
#define HCODEC_QDCT_MB_RD_PTR 0x260e
#define HCODEC_QDCT_MB_BUFF 0x260f
#define HCODEC_ANC0_CANVAS_ADDR 0x2610
#define HCODEC_REC_CANVAS_ADDR 0x2611
#define HCODEC_DBKR_CANVAS_ADDR 0x2612
#define HCODEC_DBKW_CANVAS_ADDR 0x2613
#define HCODEC_VLC_HCMD_CONFIG 0x2614
#define HCODEC_VLC_INT_CONTROL 0x2615
#define HCODEC_IE_ME_MODE 0x2616
#define HCODEC_IE_REF_SEL 0x2617
#define HCODEC_IE_ME_MB_TYPE 0x2618
#define FIXED_SLICE_CFG 0x2619
#define IDR_PIC_ID 0x261a
#define FRAME_NUMBER 0x261b
#define PIC_ORDER_CNT_LSB 0x261c
#define LOG2_MAX_PIC_ORDER_CNT_LSB 0x261d
#define LOG2_MAX_FRAME_NUM 0x261e
#define ANC0_BUFFER_ID 0x261f
#define QPPICTURE 0x2620

struct encode_wq_s {
    struct {
        u32 buf_start;
        u32 buf_size;
        u32 BitstreamStart;
        u32 BitstreamEnd;
        u32 dct_buff_start_addr;
        u32 dct_buff_end_addr;
        u32 assit_buffer_offset;
    } mem;
    struct {
        u32 encoder_width;
        u32 encoder_height;
        u32 rows_per_slice;
        u32 idr_pic_id;
        u32 frame_number;
        u32 pic_order_cnt_lsb;
        u32 log2_max_pic_order_cnt_lsb;
        u32 log2_max_frame_num;
        u32 init_qppicture;
    } pic;
    u8 quant_tbl_i4[8];
    u8 quant_tbl_i16[8];
    u8 quant_tbl_me[8];
};

/**
 * avc_configure_quantization_tables - Configure quantization tables for H.264 encoding
 * @wq: Pointer to the encode work queue structure
 *
 * This function configures the quantization tables for intra 4x4, intra 16x16,
 * and inter (motion estimation) blocks in the H.264 encoder hardware. Proper
 * configuration of these tables is crucial for balancing compression efficiency
 * and video quality.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization, after setting up the encode work queue.
 * 2. When changing quantization parameters, which might occur:
 *    - At the start of a new GOP (Group of Pictures)
 *    - When adapting to scene changes
 *    - In response to bitrate control mechanisms
 * 3. Before encoding each frame if using adaptive quantization strategies.
 * 
 * Proper tuning of quantization tables can significantly impact both
 * the visual quality and the compression ratio of the encoded video.
 */
void avc_configure_quantization_tables(struct encode_wq_s *wq);

/**
 * avc_configure_output_buffer - Configure output bitstream buffer for AVC encoding
 * @wq: Pointer to the encode work queue structure
 *
 * This function configures the output bitstream buffer for AVC encoding,
 * setting up start, end, write, and read pointers in the hardware. It ensures
 * that the encoded bitstream is correctly stored and can be accessed by the system.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization, after allocating the output buffer.
 * 2. Before starting to encode each frame.
 * 3. If using dynamic buffer allocation, when switching to a new output buffer.
 * 
 * Proper configuration of the output buffer is essential for ensuring that
 * the encoded bitstream is correctly captured and can be accessed by the system.
 */
void avc_configure_output_buffer(struct encode_wq_s *wq);

/**
 * avc_configure_input_buffer - Configure input DCT buffer for AVC encoding
 * @wq: Pointer to the encode work queue structure
 *
 * This function configures the input DCT (Discrete Cosine Transform) buffer 
 * for AVC encoding, setting up start, end, write, and read pointers in the hardware.
 * The DCT buffer is used to store intermediate data during the encoding process.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization, after allocating the DCT buffer.
 * 2. Before starting to encode each frame.
 * 3. If using a multi-buffer strategy, when switching between input buffers.
 * 
 * Proper configuration of the input DCT buffer is crucial for the encoder's
 * internal data processing and affects the overall encoding performance.
 */
void avc_configure_input_buffer(struct encode_wq_s *wq);

/**
 * avc_configure_ref_buffer - Configure reference frame buffer for AVC encoding
 * @canvas: Canvas index for the reference frame
 *
 * This function configures the reference frame buffer for AVC encoding in the hardware.
 * The reference buffer is used for inter-frame prediction and is crucial for 
 * efficient compression of P and B frames.
 *
 * When to use: Call this function in the following scenarios:
 * 1. Before encoding each P or B frame, to set up the appropriate reference frame.
 * 2. When implementing adaptive reference frame selection strategies.
 * 3. In multi-reference encoding schemes, potentially multiple times per frame.
 * 
 * Proper management and configuration of reference buffers is key to achieving
 * good compression ratios and maintaining video quality in inter-frame coding.
 */
void avc_configure_ref_buffer(int canvas);

/**
 * avc_configure_assist_buffer - Configure assist buffer for AVC encoding
 * @wq: Pointer to the encode work queue structure
 *
 * This function configures the assist buffer for AVC encoding in the hardware.
 * The assist buffer is used for various auxiliary data and intermediate results
 * during the encoding process.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization, after allocating the assist buffer.
 * 2. Before starting to encode each frame, if the assist buffer needs to be reset.
 * 3. When switching between different encoding modes that require different
 *    assist buffer configurations.
 * 
 * Proper setup of the assist buffer ensures smooth operation of various
 * encoding sub-processes and can impact overall encoding efficiency.
 */
void avc_configure_assist_buffer(struct encode_wq_s *wq);

/**
 * avc_configure_deblock_buffer - Configure deblocking buffer for AVC encoding
 * @canvas: Canvas index for the deblocking buffer
 *
 * This function configures the deblocking buffer for AVC encoding in the hardware.
 * The deblocking buffer is used in the process of reducing blocking artifacts,
 * which is an important step in maintaining video quality.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization, after allocating the deblocking buffer.
 * 2. Before encoding each frame, to ensure the deblocking buffer is properly set.
 * 3. When changing deblocking filter parameters, which might occur adaptively
 *    based on content analysis or encoding mode changes.
 * 
 * Proper configuration of the deblocking buffer is crucial for effective
 * reduction of blocking artifacts and maintaining high visual quality.
 */
void avc_configure_deblock_buffer(int canvas);

/**
 * avc_configure_encoder_params - Configure encoder parameters for AVC encoding
 * @wq: Pointer to the encode work queue structure
 * @idr: Boolean flag indicating whether this is an IDR frame
 *
 * This function configures various encoder parameters for AVC encoding in the hardware,
 * including frame numbering, picture order count, and other sequence-related parameters.
 * It sets up the core encoding parameters that define the structure of the encoded bitstream.
 *
 * When to use: Call this function in the following scenarios:
 * 1. Before encoding each frame, to update frame-specific parameters.
 * 2. At the start of a new GOP (Group of Pictures), especially for IDR frames.
 * 3. When changing encoding parameters mid-sequence (e.g., in response to 
 *    scene changes or adaptive encoding strategies).
 * 
 * Proper configuration of these parameters is essential for generating a
 * valid and efficient H.264 bitstream, and for maintaining the correct
 * structure of the encoded video sequence.
 */
void avc_configure_encoder_params(struct encode_wq_s *wq, bool idr);

/**
 * avc_configure_ie_me - Configure Intra Estimation and Motion Estimation parameters
 * @wq: Pointer to the encode work queue structure
 * @quant: Quantization parameter
 *
 * This function configures parameters for Intra Estimation (IE) and Motion Estimation (ME)
 * in the AVC encoding hardware. It sets up various control registers that govern
 * how the encoder performs intra prediction and inter-frame motion estimation.
 *
 * When to use: Call this function in the following scenarios:
 * 1. Before encoding each frame, as IE and ME parameters may need adjustment
 *    based on frame type (I, P, or B) and content characteristics.
 * 2. When implementing adaptive encoding strategies that modify IE and ME
 *    behavior based on scene complexity or motion analysis.
 * 3. When changing encoding quality settings that affect quantization and
 *    consequently IE and ME decisions.
 * 
 * Fine-tuning these parameters can significantly impact both encoding efficiency
 * and the quality of the resulting video, especially in terms of detail preservation
 * and motion representation.
 */
void avc_configure_ie_me(struct encode_wq_s *wq, u32 quant);

/**
 * avc_encoder_reset - Reset the AVC encoder hardware
 *
 * This function resets the AVC encoder hardware, clearing all registers and
 * returning the encoder to its initial state. It's a crucial function for
 * ensuring the encoder is in a known, clean state before beginning new operations.
 *
 * When to use: Call this function in the following scenarios:
 * 1. At the start of encoding a new video sequence.
 * 2. After encountering an error condition, to return the encoder to a known state.
 * 3. When switching between significantly different encoding configurations
 *    or modes that require a fresh start.
 * 4. Periodically in long encoding sessions to prevent potential state corruption.
 * 
 * Proper use of the reset function is important for maintaining the stability
 * and reliability of the encoding process, especially in continuous or long-running
 * encoding scenarios.
 */
void avc_encoder_reset(void);

/**
 * avc_encoder_start - Start the AVC encoder hardware
 *
 * This function starts the AVC encoder hardware, enabling encoding operations.
 * It typically involves setting specific control registers to initiate the
 * encoding process.
 *
 * When to use: Call this function in the following scenarios:
 * 1. After all necessary configurations have been set and just before
 *    actual frame encoding should begin.
 * 2. When resuming encoding after a pause or stop operation.
 * 3. At the start of encoding each new frame, if the hardware requires
 *    explicit starting for each frame.
 * 
 * Proper timing of the start operation is crucial for ensuring that all
 * configurations are in place and the encoder is ready to process frame data.
 */
void avc_encoder_start(void);

/**
 * avc_encoder_stop - Stop the AVC encoder hardware
 *
 * This function stops the AVC encoder hardware, halting all encoding operations.
 * It typically involves clearing or setting specific control registers to
 * cease the encoding process.
 *
 * When to use: Call this function in the following scenarios:
 * 1. At the end of encoding a video sequence.
 * 2. When needing to pause encoding operations temporarily.
 * 3. Before changing major encoding parameters that require a full stop.
 * 4. In error handling scenarios where immediate cessation of encoding is necessary.
 * 
 * Proper use of the stop function is important for cleanly ending encoding sessions,
 * managing resources, and ensuring the hardware is in a known state when not actively encoding.
 */
void avc_encoder_stop(void);

/**
 * avc_configure_mfdin - Configure MFDIN parameters for AVC encoding
 * @input: Input buffer address
 * @iformat: Input format
 * @oformat: Output format
 * @picsize_x: Picture width
 * @picsize_y: Picture height
 * @r2y_en: Enable/disable RGB to YUV conversion
 * @nr: Noise reduction level
 * @ifmt_extra: Extra input format information
 *
 * This function configures the Multi-Format Decoder Input (MFDIN) parameters
 * for AVC encoding in the hardware. It sets up various aspects of input processing,
 * including format conversion, picture dimensions, and optional features like
 * RGB to YUV conversion and noise reduction.
 *
 * When to use: Call this function in the following scenarios:
 * 1. Before starting to encode each frame, especially if frame parameters
 *    (like dimensions or format) can change.
 * 2. When switching between different input sources or formats.
 * 3. When enabling or adjusting features like noise reduction or color space conversion.
 * 4. In pipelines where the input configuration may vary from frame to frame.
 * 
 * Proper configuration of MFDIN is crucial for correctly processing the input
 * video data and preparing it for encoding. It directly affects the quality and
 * efficiency of the encoding process, especially when dealing with various
 * input formats and preprocessing requirements.
 */
void avc_configure_mfdin(u32 input, u8 iformat, u8 oformat, u32 picsize_x, u32 picsize_y,
                         u8 r2y_en, u8 nr, u8 ifmt_extra);

/**
 * avc_configure_encoder_control - Configure advanced encoder control settings
 *
 * This function sets up various advanced control registers for the AVC encoder.
 * It configures:
 * - Memory management for macroblock processing
 * - VLC (Variable Length Coding) advanced settings
 * - QDCT (Quantization and DCT) advanced settings
 * - Intra prediction and motion estimation controls
 * 
 * When to use: Call this function once during the encoder initialization phase,
 * after setting up basic encoder parameters but before configuring frame-specific
 * settings. This function sets up the overall behavior of the encoder and should
 * be called before any frame encoding begins.
 */
void avc_configure_encoder_control(void);

/**
 * avc_configure_ie_weight - Configure intra estimation and motion estimation weights
 * @i16_weight: Weight for I16MB (Intra 16x16 MacroBlock)
 * @i4_weight: Weight for I4MB (Intra 4x4 MacroBlock)
 * @me_weight: Weight for motion estimation
 *
 * This function sets up the weights for intra estimation (I16MB and I4MB) and
 * motion estimation. It also configures SAD (Sum of Absolute Differences) control
 * registers for these estimations. These weights influence the encoder's decision
 * making process for choosing between different coding modes.
 *
 * When to use: Call this function at the start of encoding each frame. The weights
 * may need to be adjusted based on:
 * - Frame type (I-frame, P-frame, or B-frame)
 * - Encoding quality requirements
 * - Scene complexity
 * Proper tuning of these weights can significantly impact encoding efficiency and quality.
 */
void avc_configure_ie_weight(u32 i16_weight, u32 i4_weight, u32 me_weight);

/**
 * avc_configure_mv_merge - Configure motion vector merge control
 * @me_mv_merge_ctl: Motion vector merge control value
 *
 * This function configures the motion vector merge control register. It affects how
 * motion vectors are merged during the encoding process, which can impact both
 * encoding efficiency and the quality of motion estimation.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization, to set the initial merge control strategy.
 * 2. Before encoding P-frames or B-frames, where motion estimation is performed.
 * 3. When adapting to changes in video content (e.g., high motion scenes vs. static scenes).
 * 
 * The merge control may need adjustment based on the nature of the video content
 * and the desired trade-off between encoding speed and motion estimation accuracy.
 */
void avc_configure_mv_merge(u32 me_mv_merge_ctl);

/**
 * avc_configure_me_parameters - Configure motion estimation parameters
 * @me_step0_close_mv: Step 0 close MV parameter
 * @me_f_skip_sad: ME f_skip SAD parameter
 * @me_f_skip_weight: ME f_skip weight parameter
 * @me_sad_range_inc: ME SAD range increment parameter
 * @me_sad_enough_01: ME SAD enough 0-1 parameter
 * @me_sad_enough_23: ME SAD enough 2-3 parameter
 *
 * This function configures various parameters related to motion estimation,
 * including SAD thresholds, weights, and control values. These parameters
 * fine-tune the motion estimation process, affecting encoding speed and efficiency.
 *
 * When to use: Call this function in the following scenarios:
 * 1. Before encoding each P-frame or B-frame.
 * 2. When adapting to changes in video content characteristics.
 * 3. When adjusting encoding quality or performance trade-offs.
 * 
 * These parameters may need frequent adjustment based on:
 * - The complexity of motion in the current scene
 * - Encoding quality requirements
 * - Available computational resources
 * - Specific requirements of the encoding profile being used
 */
void avc_configure_me_parameters(u32 me_step0_close_mv, u32 me_f_skip_sad, 
                                 u32 me_f_skip_weight, u32 me_sad_range_inc,
                                 u32 me_sad_enough_01, u32 me_sad_enough_23);

/**
 * avc_configure_skip_control - Configure skip control for V3 encoding
 *
 * This function sets up the skip control registers for V3 encoding, including:
 * - Enabling various skip features
 * - Setting up skip weights
 * - Configuring SAD thresholds for skip decisions
 * 
 * Skip control is crucial for encoding efficiency, especially in areas of the frame
 * with little change or motion.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization when using V3 encoding features.
 * 2. Before encoding each P-frame or B-frame.
 * 3. When dynamically adjusting encoding strategy based on content analysis.
 * 
 * Proper configuration of skip control can significantly reduce bitrate in static
 * or low-motion areas of the video while maintaining quality.
 */
void avc_configure_skip_control(void);

/**
 * avc_configure_mv_sad_table - Configure motion vector SAD table
 * @v3_mv_sad: Pointer to the v3_mv_sad array containing 64 SAD values
 *
 * This function populates the motion vector SAD (Sum of Absolute Differences) table
 * used in the encoding process. The table contains 64 SAD values that are used as
 * thresholds or reference points in motion estimation decisions.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization, after the SAD values have been calculated or predetermined.
 * 2. When switching between different encoding modes or profiles that require different SAD tables.
 * 3. In adaptive encoding scenarios where the SAD table is dynamically updated based on
 *    content analysis or encoding performance metrics.
 * 
 * While typically set once during initialization, advanced encoding schemes might
 * update this table periodically to adapt to changing video characteristics.
 */
void avc_configure_mv_sad_table(u32 *v3_mv_sad);

/**
 * avc_configure_ipred_weight - Configure intra prediction weights
 *
 * This function sets up the weights for various intra prediction modes, including:
 * - Weights for Chroma prediction modes
 * - Weights for Luma Intra 4x4 prediction modes
 * - Weights for Luma Intra 16x16 prediction modes
 * 
 * These weights influence the encoder's choice of intra prediction modes, affecting
 * both encoding efficiency and quality of intra-coded macroblocks.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization, before starting to encode frames.
 * 2. When switching between different encoding profiles or quality settings.
 * 3. Optionally, before encoding I-frames if adaptive intra prediction is implemented.
 * 
 * While these weights typically remain constant throughout the encoding process,
 * they may be adjusted in advanced implementations to optimize for specific content types
 * or to meet varying quality/bitrate targets.
 */
void avc_configure_ipred_weight(void);

/**
 * avc_configure_left_small_max_sad - Configure max SAD for left small area
 * @v3_left_small_max_me_sad: Max ME SAD for left small area
 * @v3_left_small_max_ie_sad: Max IE SAD for left small area
 *
 * This function sets the maximum SAD (Sum of Absolute Differences) values
 * for the left small area in both motion estimation (ME) and intra estimation (IE).
 * These values act as thresholds in the decision-making process for encoding
 * the left edge of macroblocks, which can have special importance in prediction.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization, to set the initial thresholds.
 * 2. Before starting to encode a new sequence or scene.
 * 3. When adapting encoding parameters based on content analysis.
 * 
 * These values may need adjustment based on:
 * - The nature of the video content (e.g., high detail vs. low detail on frame edges)
 * - Encoding quality requirements
 * - Specific requirements of the encoding profile being used
 * 
 * Fine-tuning these parameters can help in achieving a balance between edge quality
 * and overall encoding efficiency.
 */
void avc_configure_left_small_max_sad(u32 v3_left_small_max_me_sad, u32 v3_left_small_max_ie_sad);

#endif /* __AVC_ENCODER_HW_OPS_H__ */
