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
 * and inter (motion estimation) blocks in the H.264 encoder hardware.
 */
void avc_configure_quantization_tables(struct encode_wq_s *wq);

/**
 * avc_configure_output_buffer - Configure output bitstream buffer for AVC encoding
 * @wq: Pointer to the encode work queue structure
 *
 * This function configures the output bitstream buffer for AVC encoding,
 * setting up start, end, write, and read pointers in the hardware.
 */
void avc_configure_output_buffer(struct encode_wq_s *wq);

/**
 * avc_configure_input_buffer - Configure input DCT buffer for AVC encoding
 * @wq: Pointer to the encode work queue structure
 *
 * This function configures the input DCT buffer for AVC encoding,
 * setting up start, end, write, and read pointers in the hardware.
 */
void avc_configure_input_buffer(struct encode_wq_s *wq);

/**
 * avc_configure_ref_buffer - Configure reference frame buffer for AVC encoding
 * @canvas: Canvas index for the reference frame
 *
 * This function configures the reference frame buffer for AVC encoding in the hardware.
 */
void avc_configure_ref_buffer(int canvas);

/**
 * avc_configure_assist_buffer - Configure assist buffer for AVC encoding
 * @wq: Pointer to the encode work queue structure
 *
 * This function configures the assist buffer for AVC encoding in the hardware.
 */
void avc_configure_assist_buffer(struct encode_wq_s *wq);

/**
 * avc_configure_deblock_buffer - Configure deblocking buffer for AVC encoding
 * @canvas: Canvas index for the deblocking buffer
 *
 * This function configures the deblocking buffer for AVC encoding in the hardware.
 */
void avc_configure_deblock_buffer(int canvas);

/**
 * avc_configure_encoder_params - Configure encoder parameters for AVC encoding
 * @wq: Pointer to the encode work queue structure
 * @idr: Boolean flag indicating whether this is an IDR frame
 *
 * This function configures various encoder parameters for AVC encoding in the hardware,
 * including frame numbering and picture order count.
 */
void avc_configure_encoder_params(struct encode_wq_s *wq, bool idr);

/**
 * avc_configure_ie_me - Configure Intra Estimation and Motion Estimation parameters
 * @wq: Pointer to the encode work queue structure
 * @quant: Quantization parameter
 *
 * This function configures parameters for Intra Estimation and Motion Estimation
 * in the AVC encoding hardware.
 */
void avc_configure_ie_me(struct encode_wq_s *wq, u32 quant);

/**
 * avc_encoder_reset - Reset the AVC encoder hardware
 *
 * This function resets the AVC encoder hardware, clearing all registers.
 */
void avc_encoder_reset(void);

/**
 * avc_encoder_start - Start the AVC encoder hardware
 *
 * This function starts the AVC encoder hardware, enabling encoding operations.
 */
void avc_encoder_start(void);

/**
 * avc_encoder_stop - Stop the AVC encoder hardware
 *
 * This function stops the AVC encoder hardware, disabling encoding operations.
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
 * for AVC encoding in the hardware, including input/output formats and picture dimensions.
 */
void avc_configure_mfdin(u32 input, u8 iformat, u8 oformat, u32 picsize_x, u32 picsize_y,
                         u8 r2y_en, u8 nr, u8 ifmt_extra);

#endif /* __AVC_ENCODER_HW_OPS_H__ */
