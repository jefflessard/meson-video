#ifndef __AVC_ENCODER_HW_OPS_H__
#define __AVC_ENCODER_HW_OPS_H__

u8 get_cpu_type(void);

void WRITE_HREG(u32 reg, u32 val);
u32 READ_HREG(u32 reg);

void WRITE_VREG(u32 reg, u32 val);
u32 READ_VREG(u32 reg);

s32 hcodec_hw_reset(void);


/**
 * avc_configure_quantization_tables - Configure quantization tables for H.264 encoding
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
void avc_configure_quantization_tables(u8 quant_tbl_i4[8], u8 quant_tbl_i16[8], u8 quant_tbl_me[8]);

/**
 * avc_configure_output_buffer - Configure output bitstream buffer for AVC encoding
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
void avc_configure_output_buffer(u32 bitstreamStart, u32 bitstreamEnd);

/**
 * avc_configure_input_buffer - Configure input DCT buffer for AVC encoding
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
void avc_configure_input_buffer(u32 dct_buff_start_addr, u32 dct_buff_end_addr);

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
void avc_configure_assist_buffer(u32 assit_buffer_offset);

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
void avc_configure_encoder_params(
		u32 idr_pic_id, u32 frame_number, u32 pic_order_cnt_lsb,
		u32 log2_max_frame_num, u32 log2_max_pic_order_cnt_lsb,
		u32 init_qppicture, bool idr
	);

/**
 * avc_configure_ie_me - Configure Intra Estimation and Motion Estimation parameters
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
void avc_configure_ie_me(u32 quant);

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
void avc_configure_encoder_control(u32 pic_width);

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
void avc_configure_me_parameters(
		u32 me_step0_close_mv, u32 me_f_skip_sad,
		u32 me_f_skip_weight, u32 me_sad_range_inc,
		u32 me_sad_enough_01, u32 me_sad_enough_23,
		u32 me_mv_weight_01, u32 me_mv_weight_23
);

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
void avc_configure_mv_sad_table(u32 v3_mv_sad[64]);

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
void avc_configure_left_small_max_sad(u32 max_me_sad, u32 max_ie_sad);


/**
 * avc_configure_svc_pic_type - Configure SVC picture type
 *
 * This function configures the Scalable Video Coding (SVC) picture type register.
 * It sets whether the current picture is a reference frame or a non-reference frame
 * in the SVC hierarchy.
 *
 * When to use: Call this function in the following scenarios:
 * 1. Before encoding each frame in an SVC-enabled encoding session.
 * 2. When switching between reference and non-reference frames in the SVC structure.
 * 3. At the start of a new GOP, especially for IDR frames.
 *
 * Proper configuration of SVC picture types is crucial for maintaining the correct
 * scalable structure in SVC-enabled H.264 streams, affecting both encoding efficiency
 * and the ability to extract different quality or resolution layers from the stream.
 */
void avc_configure_svc_pic_type(bool enc_slc_ref);

/**
 * avc_configure_fixed_slice - Configure fixed slice settings
 *
 * This function configures the fixed slice settings for the encoder. It sets up
 * the number of macroblocks per slice, which is crucial for controlling the slice
 * structure of the encoded video.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization, after setting up basic encoding parameters.
 * 2. When changing slice configuration, which might occur:
 *    - At the start of encoding a new video sequence
 *    - When adapting to different network conditions or packaging requirements
 * 3. Before encoding each frame if using a dynamic slice configuration strategy
 *
 * Proper configuration of slice settings is important for balancing between
 * error resilience, parallel processing capabilities, and coding efficiency.
 */
void avc_configure_fixed_slice(u32 fixed_slice_cfg, u32 rows_per_slice, u32 pic_height);

#if 0
/**
 * avc_configure_encoding_mode - Configure encoding mode and related parameters
 * @wq: Pointer to the encode work queue structure
 * @is_idr: Boolean indicating if the current frame is an IDR frame
 *
 * This function configures various encoding mode parameters, including:
 * - I/P frame settings
 * - Motion vector related configurations
 * - Pipeline and mixed encoding settings
 *
 * When to use: Call this function in the following scenarios:
 * 1. Before encoding each frame, as the settings may change based on frame type.
 * 2. When switching between different encoding strategies (e.g., changing motion estimation settings).
 * 3. At the start of a new GOP, especially for IDR frames.
 *
 * Proper configuration of these parameters is essential for optimizing the encoding process,
 * balancing between encoding efficiency, quality, and computational complexity.
 */
void avc_configure_encoding_mode(struct encode_wq_s *wq, bool is_idr);
#endif

/**
 * avc_configure_cbr_settings - Configure Constant Bit Rate (CBR) settings
 *
 * This function configures the Constant Bit Rate (CBR) settings for the encoder.
 * It sets up various parameters related to bitrate control and buffer management.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization when CBR mode is enabled.
 * 2. When switching between different bitrate targets or CBR strategies.
 * 3. Before starting to encode a new sequence with different CBR requirements.
 *
 * Proper configuration of CBR settings is crucial for maintaining consistent
 * bitrate output and managing buffer fullness, which is important for streaming
 * and broadcasting applications.
 */
void avc_configure_cbr_settings(
	u32 ddr_start_addr, u32 ddr_addr_length,
	u16 block_w, u16 block_h, u16 long_th,
	u8 start_tbl_id, u8 short_shift, u8 long_mb_num
);

#if 0
/**
 * avc_init_encode_weight - Initialize encoding weights
 *
 * This function initializes various weight parameters used in the encoding process,
 * including motion estimation weights, SAD (Sum of Absolute Differences) thresholds,
 * and skip-related parameters.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization, before starting to encode any frames.
 * 2. When resetting the encoder to its default state.
 * 3. Before changing to a significantly different encoding configuration that
 *    requires resetting weights to their initial values.
 *
 * Proper initialization of these weights is crucial for the encoder's decision-making
 * process, affecting both encoding quality and efficiency. These weights influence
 * various aspects of the encoding process, including motion estimation, mode decision,
 * and skip detection.
 */
void avc_init_encode_weight(void);
#endif

#if 0
/**
 * avc_configure_v3_control - Configure V3 encoding control parameters
 *
 * This function configures various control parameters specific to V3 encoding,
 * including skip control, motion vector SAD table, and intra prediction weights.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization when using V3 encoding features.
 * 2. When switching to V3 encoding mode from another mode.
 * 3. Before starting to encode a new sequence with V3-specific requirements.
 *
 * Proper configuration of V3 control parameters is important for optimizing
 * the encoding process in V3 mode, affecting both encoding efficiency and quality.
 */
void avc_configure_v3_control(void);
#endif

/**
 * avc_configure_quant_params - Configure quantization parameters
 *
 * This function configures various quantization-related parameters, including
 * the quantization tables for I and P frames, and sets up the quantization control registers.
 *
 * When to use: Call this function in the following scenarios:
 * 1. Before encoding each frame, as quantization parameters may change.
 * 2. When adapting quantization strategy based on rate control feedback.
 * 3. When switching between different quality presets that affect quantization.
 *
 * Proper configuration of quantization parameters is crucial for balancing
 * between video quality and bitrate, directly affecting the compression efficiency.
 */
void avc_configure_quant_params(u32 i_pic_qp, u32 p_pic_qp);

void avc_configure_quant_control(u32 i_pic_qp);

/**
 * avc_configure_ignore_config - Configure ignore settings for encoding
 *
 * This function configures the ignore settings for the encoder, which can be used
 * to skip or ignore certain coefficients or conditions during the encoding process.
 * This can be useful for performance optimization or specific encoding strategies.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization, to set up default ignore configurations.
 * 2. When changing encoding strategies that involve ignoring certain data.
 * 3. Before encoding frames with specific ignore requirements (e.g., for fast encoding modes).
 *
 * Proper configuration of ignore settings can help in achieving a balance between
 * encoding speed and quality, especially in scenarios where certain details can be
 * sacrificed for improved performance.
 */
void avc_configure_ignore_config(bool ignore);

/**
 * avc_configure_sad_control - Configure SAD control parameters
 *
 * This function configures the Sum of Absolute Differences (SAD) control parameters,
 * which are crucial for motion estimation and mode decision processes in the encoder.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization, to set up default SAD configurations.
 * 2. Before encoding each frame, if SAD parameters need to be adjusted dynamically.
 * 3. When switching between different encoding quality presets that affect SAD thresholds.
 *
 * Proper configuration of SAD parameters is essential for balancing encoding speed
 * and quality, particularly in motion estimation and intra prediction processes.
 */
void avc_configure_sad_control(void);

/**
 * avc_configure_ie_control - Configure Intra Estimation control parameters
 *
 * This function sets up the Intra Estimation (IE) control parameters, which
 * are important for intra prediction and coding decisions within the encoder.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization, to set up default IE configurations.
 * 2. Before encoding I-frames or intra blocks in P-frames.
 * 3. When adjusting encoding strategies that affect intra prediction.
 *
 * Proper configuration of IE control is crucial for efficient intra coding,
 * which directly impacts the quality and compression efficiency of I-frames
 * and intra-coded areas in P-frames.
 */
void avc_configure_ie_control(void);

/**
 * avc_configure_me_skip_line - Configure Motion Estimation skip line parameters
 *
 * This function configures the skip line parameters for Motion Estimation (ME).
 * Skip lines are used to reduce the computational complexity of ME by skipping
 * certain lines during the search process.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization, to set up default ME skip line configurations.
 * 2. Before encoding P-frames or B-frames where ME is performed.
 * 3. When adjusting encoding speed/quality trade-offs that involve ME complexity.
 *
 * Proper configuration of ME skip lines can significantly affect encoding speed
 * and the accuracy of motion estimation, thus impacting both performance and
 * compression efficiency.
 */
void avc_configure_me_skip_line(void);

/**
 * avc_configure_v5_simple_mb - Configure V5 simple macroblock control
 *
 * This function configures the simple macroblock control parameters for V5 encoding.
 * It sets up various thresholds and enables/disables different macroblock coding options.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization when using V5 encoding features.
 * 2. Before encoding frames in V5 mode.
 * 3. When adjusting encoding complexity or quality settings in V5 mode.
 *
 * Proper configuration of V5 simple MB control can help in optimizing the
 * encoding process for specific content types or performance requirements.
 */
void avc_configure_v5_simple_mb(u32 qp_mode, u32 dq_setting, u32 me_weight_setting);

#if 0
/**
 * avc_configure_canvas - Configure canvas for input and reference frames
 * @wq: Pointer to the encode work queue structure
 *
 * This function sets up the canvas configuration for input frames and reference buffers.
 * It configures the memory layout and addressing for Y, U, and V components of the video frames.
 *
 * When to use: Call this function in the following scenarios:
 * 1. During encoder initialization, after memory allocation for frame buffers.
 * 2. When changing input frame format or dimensions.
 * 3. Before starting to encode a new sequence with different buffer requirements.
 *
 * Proper canvas configuration is crucial for correct frame buffer management and
 * efficient memory access during the encoding process.
 */
void avc_configure_canvas(struct encode_wq_s *wq);
#endif

#if 0
/**
 * avc_convert_cbr_table - Convert CBR table for hardware usage
 * @table: Pointer to the CBR table
 * @len: Length of the CBR table
 *
 * This function converts the Constant Bit Rate (CBR) table for use by the hardware encoder.
 * It performs necessary byte swapping to ensure the table is in the correct format for hardware consumption.
 *
 * When to use: Call this function in the following scenarios:
 * 1. After initializing or updating the CBR table in software.
 * 2. Before enabling CBR encoding mode.
 * 3. When switching between different CBR configurations.
 *
 * Proper conversion of the CBR table is essential for accurate bitrate control in CBR encoding mode.
 */
void avc_convert_cbr_table(void *table, u32 len);
#endif

#endif /* __AVC_ENCODER_HW_OPS_H__ */
