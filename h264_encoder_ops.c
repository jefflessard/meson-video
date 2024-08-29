#include "h264_encoder_ops.h"
#include "vdec_reg.h"

void avc_configure_quantization_tables(struct encode_wq_s *wq)
{
    WRITE_HREG(HCODEC_Q_QUANT_CONTROL,
        (0 << 23) |  /* quant_table_addr */
        (1 << 22));  /* quant_table_addr_update */

    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_i4[0]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_i4[1]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_i4[2]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_i4[3]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_i4[4]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_i4[5]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_i4[6]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_i4[7]);

    WRITE_HREG(HCODEC_Q_QUANT_CONTROL,
        (8 << 23) |  /* quant_table_addr */
        (1 << 22));  /* quant_table_addr_update */

    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_i16[0]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_i16[1]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_i16[2]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_i16[3]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_i16[4]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_i16[5]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_i16[6]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_i16[7]);

    WRITE_HREG(HCODEC_Q_QUANT_CONTROL,
        (16 << 23) | /* quant_table_addr */
        (1 << 22));  /* quant_table_addr_update */

    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_me[0]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_me[1]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_me[2]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_me[3]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_me[4]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_me[5]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_me[6]);
    WRITE_HREG(HCODEC_QUANT_TABLE_DATA,
        wq->quant_tbl_me[7]);
}

void avc_configure_output_buffer(struct encode_wq_s *wq)
{
    WRITE_HREG(HCODEC_VLC_VB_MEM_CTL,
        ((1 << 31) | (0x3f << 24) |
        (0x20 << 16) | (2 << 0)));
    WRITE_HREG(HCODEC_VLC_VB_START_PTR,
        wq->mem.BitstreamStart);
    WRITE_HREG(HCODEC_VLC_VB_WR_PTR,
        wq->mem.BitstreamStart);
    WRITE_HREG(HCODEC_VLC_VB_SW_RD_PTR,
        wq->mem.BitstreamStart);
    WRITE_HREG(HCODEC_VLC_VB_END_PTR,
        wq->mem.BitstreamEnd);
    WRITE_HREG(HCODEC_VLC_VB_CONTROL, 1);
    WRITE_HREG(HCODEC_VLC_VB_CONTROL,
        ((0 << 14) | (7 << 3) |
        (1 << 1) | (0 << 0)));
}

void avc_configure_input_buffer(struct encode_wq_s *wq)
{
    WRITE_HREG(HCODEC_QDCT_MB_START_PTR,
        wq->mem.dct_buff_start_addr);
    WRITE_HREG(HCODEC_QDCT_MB_END_PTR,
        wq->mem.dct_buff_end_addr);
    WRITE_HREG(HCODEC_QDCT_MB_WR_PTR,
        wq->mem.dct_buff_start_addr);
    WRITE_HREG(HCODEC_QDCT_MB_RD_PTR,
        wq->mem.dct_buff_start_addr);
    WRITE_HREG(HCODEC_QDCT_MB_BUFF, 0);
}

void avc_configure_ref_buffer(int canvas)
{
    WRITE_HREG(HCODEC_ANC0_CANVAS_ADDR, canvas);
    WRITE_HREG(HCODEC_VLC_HCMD_CONFIG, 0);
}

void avc_configure_assist_buffer(struct encode_wq_s *wq)
{
    WRITE_HREG(MEM_OFFSET_REG, wq->mem.assit_buffer_offset);
}

void avc_configure_deblock_buffer(int canvas)
{
    WRITE_HREG(HCODEC_REC_CANVAS_ADDR, canvas);
    WRITE_HREG(HCODEC_DBKR_CANVAS_ADDR, canvas);
    WRITE_HREG(HCODEC_DBKW_CANVAS_ADDR, canvas);
}

void avc_configure_encoder_params(struct encode_wq_s *wq, bool idr)
{
    WRITE_HREG(HCODEC_VLC_TOTAL_BYTES, 0);
    WRITE_HREG(HCODEC_VLC_CONFIG, 0x07);
    WRITE_HREG(HCODEC_VLC_INT_CONTROL, 0);

    WRITE_HREG(HCODEC_ASSIST_AMR1_INT0, 0x15);
    WRITE_HREG(HCODEC_ASSIST_AMR1_INT1, 0x8);
    WRITE_HREG(HCODEC_ASSIST_AMR1_INT3, 0x14);

    WRITE_HREG(IDR_PIC_ID, wq->pic.idr_pic_id);
    WRITE_HREG(FRAME_NUMBER,
        (idr == true) ? 0 : wq->pic.frame_number);
    WRITE_HREG(PIC_ORDER_CNT_LSB,
        (idr == true) ? 0 : wq->pic.pic_order_cnt_lsb);

    WRITE_HREG(LOG2_MAX_PIC_ORDER_CNT_LSB,
        wq->pic.log2_max_pic_order_cnt_lsb);
    WRITE_HREG(LOG2_MAX_FRAME_NUM,
        wq->pic.log2_max_frame_num);
    WRITE_HREG(ANC0_BUFFER_ID, 0);
    WRITE_HREG(QPPICTURE, wq->pic.init_qppicture);
}

void avc_configure_ie_me(struct encode_wq_s *wq, u32 quant)
{
    u32 ie_me_mode;
    u32 ie_me_mb_type;
    u32 ie_cur_ref_sel;
    u32 ie_pippeline_block = 3;

    ie_cur_ref_sel = 0;
    ie_me_mode = (ie_pippeline_block & IE_PIPPELINE_BLOCK_MASK) << IE_PIPPELINE_BLOCK_SHIFT;

    WRITE_HREG(IE_ME_MODE, ie_me_mode);
    WRITE_HREG(IE_REF_SEL, ie_cur_ref_sel);

    if (quant == 0) { // Assuming 0 indicates IDR frame
        ie_me_mb_type = HENC_MB_Type_I4MB;
    } else {
        ie_me_mb_type = (HENC_SKIP_RUN_AUTO << 16) |
                        (HENC_MB_Type_AUTO << 4) |
                        (HENC_MB_Type_AUTO << 0);
    }
    WRITE_HREG(IE_ME_MB_TYPE, ie_me_mb_type);
}

void avc_encoder_reset(void)
{
    WRITE_HREG(HCODEC_MPSR, 0);
    WRITE_HREG(HCODEC_CPSR, 0);
}

void avc_encoder_start(void)
{
    WRITE_HREG(HCODEC_MPSR, 0x0001);
}

void avc_encoder_stop(void)
{
    WRITE_HREG(HCODEC_MPSR, 0);
    WRITE_HREG(HCODEC_CPSR, 0);
}

void avc_configure_mfdin(u32 input, u8 iformat, u8 oformat, u32 picsize_x, u32 picsize_y,
                         u8 r2y_en, u8 nr, u8 ifmt_extra)
{
    u8 dsample_en; /* Downsample Enable */
    u8 interp_en;  /* Interpolation Enable */
    u8 y_size;     /* 0:16 Pixels for y direction pickup; 1:8 pixels */
    u8 r2y_mode;   /* RGB2YUV Mode, range(0~3) */
    u8 canv_idx0_bppx;
    u8 canv_idx1_bppx;
    u8 canv_idx0_bppy;
    u8 canv_idx1_bppy;
    u8 ifmt444, ifmt422, ifmt420, linear_bytes4p;
    u8 nr_enable;
    u8 cfg_y_snr_en;
    u8 cfg_y_tnr_en;
    u8 cfg_c_snr_en;
    u8 cfg_c_tnr_en;
    u32 linear_bytesperline;
    s32 reg_offset;
    bool linear_enable = false;
    bool format_err = false;

    if (get_cpu_type() >= MESON_CPU_MAJOR_ID_TXL) {
        if ((iformat == 7) && (ifmt_extra > 2))
            format_err = true;
    } else if (iformat == 7)
        format_err = true;

    if (format_err) {
        enc_pr(LOG_ERROR,
            "mfdin format err, iformat:%d, ifmt_extra:%d\n",
            iformat, ifmt_extra);
        return;
    }
    if (iformat != 7)
        ifmt_extra = 0;

    ifmt444 = ((iformat == 1) || (iformat == 5) || (iformat == 8) ||
        (iformat == 9) || (iformat == 12)) ? 1 : 0;
    if (iformat == 7 && ifmt_extra == 1)
        ifmt444 = 1;
    ifmt422 = ((iformat == 0) || (iformat == 10)) ? 1 : 0;
    if (iformat == 7 && ifmt_extra != 1)
        ifmt422 = 1;
    ifmt420 = ((iformat == 2) || (iformat == 3) || (iformat == 4) ||
        (iformat == 11)) ? 1 : 0;
    dsample_en = ((ifmt444 && (oformat != 2)) ||
        (ifmt422 && (oformat == 0))) ? 1 : 0;
    interp_en = ((ifmt422 && (oformat == 2)) ||
        (ifmt420 && (oformat != 0))) ? 1 : 0;
    y_size = (oformat != 0) ? 1 : 0;
    if (iformat == 12)
        y_size = 0;
    r2y_mode = (r2y_en == 1) ? 1 : 0; /* Fixed to 1 (TODO) */
    canv_idx0_bppx = (iformat == 1) ? 3 : (iformat == 0) ? 2 : 1;
    canv_idx1_bppx = (iformat == 4) ? 0 : 1;
    canv_idx0_bppy = 1;
    canv_idx1_bppy = (iformat == 5) ? 1 : 0;

    if ((iformat == 8) || (iformat == 9) || (iformat == 12))
        linear_bytes4p = 3;
    else if (iformat == 10)
        linear_bytes4p = 2;
    else if (iformat == 11)
        linear_bytes4p = 1;
    else
        linear_bytes4p = 0;
    if (iformat == 12)
        linear_bytesperline = picsize_x * 4;
    else
        linear_bytesperline = picsize_x * linear_bytes4p;

    if (iformat < 8)
        linear_enable = false;
    else
        linear_enable = true;

    if (get_cpu_type() >= MESON_CPU_MAJOR_ID_GXBB) {
        reg_offset = -8;
        /* nr_mode: 0:Disabled 1:SNR Only 2:TNR Only 3:3DNR */
        nr_enable = (nr) ? 1 : 0;
        cfg_y_snr_en = ((nr == 1) || (nr == 3)) ? 1 : 0;
        cfg_y_tnr_en = ((nr == 2) || (nr == 3)) ? 1 : 0;
        cfg_c_snr_en = cfg_y_snr_en;
        /* cfg_c_tnr_en = cfg_y_tnr_en; */
        cfg_c_tnr_en = 0;

        /* NR For Y */
        WRITE_HREG((HCODEC_MFDIN_REG0D + reg_offset),
            ((cfg_y_snr_en << 0) |
            (y_snr_err_norm << 1) |
            (y_snr_gau_bld_core << 2) |
            (((y_snr_gau_bld_ofst) & 0xff) << 6) |
            (y_snr_gau_bld_rate << 14) |
            (y_snr_gau_alp0_min << 20) |
            (y_snr_gau_alp0_max << 26)));
        WRITE_HREG((HCODEC_MFDIN_REG0E + reg_offset),
            ((cfg_y_tnr_en << 0) |
            (y_tnr_mc_en << 1) |
            (y_tnr_txt_mode << 2) |
            (y_tnr_mot_sad_margin << 3) |
            (y_tnr_alpha_min << 7) |
            (y_tnr_alpha_max << 13) |
            (y_tnr_deghost_os << 19)));
        WRITE_HREG((HCODEC_MFDIN_REG0F + reg_offset),
            ((y_tnr_mot_cortxt_rate << 0) |
            (y_tnr_mot_distxt_ofst << 8) |
            (y_tnr_mot_distxt_rate << 4) |
            (y_tnr_mot_dismot_ofst << 16) |
            (y_tnr_mot_frcsad_lock << 24)));
        WRITE_HREG((HCODEC_MFDIN_REG10 + reg_offset),
            ((y_tnr_mot2alp_frc_gain << 0) |
            (y_tnr_mot2alp_nrm_gain << 8) |
            (y_tnr_mot2alp_dis_gain << 16) |
            (y_tnr_mot2alp_dis_ofst << 24)));
        WRITE_HREG((HCODEC_MFDIN_REG11 + reg_offset),
            ((y_bld_beta2alp_rate << 0) |
            (y_bld_beta_min << 8) |
            (y_bld_beta_max << 14)));

        /* NR For C */
        WRITE_HREG((HCODEC_MFDIN_REG12 + reg_offset),
            ((cfg_y_snr_en << 0) |
            (c_snr_err_norm << 1) |
            (c_snr_gau_bld_core << 2) |
            (((c_snr_gau_bld_ofst) & 0xff) << 6) |
            (c_snr_gau_bld_rate << 14) |
            (c_snr_gau_alp0_min << 20) |
            (c_snr_gau_alp0_max << 26)));

        WRITE_HREG((HCODEC_MFDIN_REG13 + reg_offset),
            ((cfg_c_tnr_en << 0) |
            (c_tnr_mc_en << 1) |
            (c_tnr_txt_mode << 2) |
            (c_tnr_mot_sad_margin << 3) |
            (c_tnr_alpha_min << 7) |
            (c_tnr_alpha_max << 13) |
            (c_tnr_deghost_os << 19)));
        WRITE_HREG((HCODEC_MFDIN_REG14 + reg_offset),
            ((c_tnr_mot_cortxt_rate << 0) |
            (c_tnr_mot_distxt_ofst << 8) |
            (c_tnr_mot_distxt_rate << 4) |
            (c_tnr_mot_dismot_ofst << 16) |
            (c_tnr_mot_frcsad_lock << 24)));
        WRITE_HREG((HCODEC_MFDIN_REG15 + reg_offset),
            ((c_tnr_mot2alp_frc_gain << 0) |
            (c_tnr_mot2alp_nrm_gain << 8) |
            (c_tnr_mot2alp_dis_gain << 16) |
            (c_tnr_mot2alp_dis_ofst << 24)));

        WRITE_HREG((HCODEC_MFDIN_REG16 + reg_offset),
            ((c_bld_beta2alp_rate << 0) |
            (c_bld_beta_min << 8) |
            (c_bld_beta_max << 14)));

        WRITE_HREG((HCODEC_MFDIN_REG1_CTRL + reg_offset),
            (iformat << 0) | (oformat << 4) |
            (dsample_en << 6) | (y_size << 8) |
            (interp_en << 9) | (r2y_en << 12) |
            (r2y_mode << 13) | (ifmt_extra << 16) |
            (nr_enable << 19));

        if (get_cpu_type() >= MESON_CPU_MAJOR_ID_SC2) {
            WRITE_HREG((HCODEC_MFDIN_REG8_DMBL + reg_offset),
                (picsize_x << 16) | (picsize_y << 0));
        } else {
            WRITE_HREG((HCODEC_MFDIN_REG8_DMBL + reg_offset),
                (picsize_x << 14) | (picsize_y << 0));
        }
    } else {
        reg_offset = 0;
        WRITE_HREG((HCODEC_MFDIN_REG1_CTRL + reg_offset),
            (iformat << 0) | (oformat << 4) |
            (dsample_en << 6) | (y_size << 8) |
            (interp_en << 9) | (r2y_en << 12) |
            (r2y_mode << 13));

        WRITE_HREG((HCODEC_MFDIN_REG8_DMBL + reg_offset),
            (picsize_x << 12) | (picsize_y << 0));
    }

    if (linear_enable == false) {
        WRITE_HREG((HCODEC_MFDIN_REG3_CANV + reg_offset),
            (input & 0xffffff) |
            (canv_idx1_bppy << 30) |
            (canv_idx0_bppy << 28) |
            (canv_idx1_bppx << 26) |
            (canv_idx0_bppx << 24));
        WRITE_HREG((HCODEC_MFDIN_REG4_LNR0 + reg_offset),
            (0 << 16) | (0 << 0));
        WRITE_HREG((HCODEC_MFDIN_REG5_LNR1 + reg_offset), 0);
    } else {
        WRITE_HREG((HCODEC_MFDIN_REG3_CANV + reg_offset),
            (canv_idx1_bppy << 30) |
            (canv_idx0_bppy << 28) |
            (canv_idx1_bppx << 26) |
            (canv_idx0_bppx << 24));
        WRITE_HREG((HCODEC_MFDIN_REG4_LNR0 + reg_offset),
            (linear_bytes4p << 16) | (linear_bytesperline << 0));
        WRITE_HREG((HCODEC_MFDIN_REG5_LNR1 + reg_offset), input);
    }

    if (iformat == 12)
        WRITE_HREG((HCODEC_MFDIN_REG9_ENDN + reg_offset),
            (2 << 0) | (1 << 3) | (0 << 6) |
            (3 << 9) | (6 << 12) | (5 << 15) |
            (4 << 18) | (7 << 21));
    else
        WRITE_HREG((HCODEC_MFDIN_REG9_ENDN + reg_offset),
            (7 << 0) | (6 << 3) | (5 << 6) |
            (4 << 9) | (3 << 12) | (2 << 15) |
            (1 << 18) | (0 << 21));
}

void avc_configure_encoder_control(void)
{
    WRITE_HREG(HCODEC_HDEC_MC_OMEM_AUTO,
        (1 << 31) | /* use_omem_mb_xy */
        ((pic_width_in_mb - 1) << 16)); /* omem_max_mb_x */

    WRITE_HREG(HCODEC_VLC_ADV_CONFIG,
        (0 << 10) | /* early_mix_mc_hcmd */
        (1 << 9) | /* update_top_left_mix */
        (1 << 8) | /* p_top_left_mix */
        (0 << 7) | /* mv_cal_mixed_type */
        (0 << 6) | /* mc_hcmd_mixed_type */
        (1 << 5) | /* use_separate_int_control */
        (1 << 4) | /* hcmd_intra_use_q_info */
        (1 << 3) | /* hcmd_left_use_prev_info */
        (1 << 2) | /* hcmd_use_q_info */
        (1 << 1) | /* use_q_delta_quant */
        (0 << 0)); /* detect_I16_from_I4 use qdct detected mb_type */

    WRITE_HREG(HCODEC_QDCT_ADV_CONFIG,
        (1 << 29) | /* mb_info_latch_no_I16_pred_mode */
        (1 << 28) | /* ie_dma_mbxy_use_i_pred */
        (1 << 27) | /* ie_dma_read_write_use_ip_idx */
        (1 << 26) | /* ie_start_use_top_dma_count */
        (1 << 25) | /* i_pred_top_dma_rd_mbbot */
        (1 << 24) | /* i_pred_top_dma_wr_disable */
        (0 << 23) | /* i_pred_mix */
        (1 << 22) | /* me_ab_rd_when_intra_in_p */
        (1 << 21) | /* force_mb_skip_run_when_intra */
        (0 << 20) | /* mc_out_mixed_type */
        (1 << 19) | /* ie_start_when_quant_not_full */
        (1 << 18) | /* mb_info_state_mix */
        (0 << 17) | /* mb_type_use_mix_result */
        (0 << 16) | /* me_cb_ie_read_enable */
        (0 << 15) | /* ie_cur_data_from_me */
        (1 << 14) | /* rem_per_use_table */
        (0 << 13) | /* q_latch_int_enable */
        (1 << 12) | /* q_use_table */
        (0 << 11) | /* q_start_wait */
        (1 << 10) | /* LUMA_16_LEFT_use_cur */
        (1 << 9) | /* DC_16_LEFT_SUM_use_cur */
        (1 << 8) | /* c_ref_ie_sel_cur */
        (0 << 7) | /* c_ipred_perfect_mode */
        (1 << 6) | /* ref_ie_ul_sel */
        (1 << 5) | /* mb_type_use_ie_result */
        (1 << 4) | /* detect_I16_from_I4 */
        (1 << 3) | /* ie_not_wait_ref_busy */
        (1 << 2) | /* ie_I16_enable */
        (3 << 0)); /* ie_done_sel  // fastest when waiting */
}

void avc_configure_ie_weight(u32 i16_weight, u32 i4_weight, u32 me_weight)
{
    WRITE_HREG(HCODEC_IE_WEIGHT,
        (i16_weight << 16) |
        (i4_weight << 0));
    WRITE_HREG(HCODEC_ME_WEIGHT,
        (me_weight << 0));
    WRITE_HREG(HCODEC_SAD_CONTROL_0,
        /* ie_sad_offset_I16 */
        (i16_weight << 16) |
        /* ie_sad_offset_I4 */
        (i4_weight << 0));
    WRITE_HREG(HCODEC_SAD_CONTROL_1,
        /* ie_sad_shift_I16 */
        (IE_SAD_SHIFT_I16 << 24) |
        /* ie_sad_shift_I4 */
        (IE_SAD_SHIFT_I4 << 20) |
        /* me_sad_shift_INTER */
        (ME_SAD_SHIFT_INTER << 16) |
        /* me_sad_offset_INTER */
        (me_weight << 0));
}

void avc_configure_mv_merge(u32 me_mv_merge_ctl)
{
    WRITE_HREG(HCODEC_ME_MV_MERGE_CTL, me_mv_merge_ctl);
}

void avc_configure_me_parameters(u32 me_step0_close_mv, u32 me_f_skip_sad, 
                                 u32 me_f_skip_weight, u32 me_sad_range_inc,
                                 u32 me_sad_enough_01, u32 me_sad_enough_23)
{
    WRITE_HREG(HCODEC_ME_STEP0_CLOSE_MV, me_step0_close_mv);
    WRITE_HREG(HCODEC_ME_F_SKIP_SAD, me_f_skip_sad);
    WRITE_HREG(HCODEC_ME_F_SKIP_WEIGHT, me_f_skip_weight);
    WRITE_HREG(HCODEC_ME_SAD_RANGE_INC, me_sad_range_inc);
    WRITE_HREG(HCODEC_ME_SAD_ENOUGH_01, me_sad_enough_01);
    WRITE_HREG(HCODEC_ME_SAD_ENOUGH_23, me_sad_enough_23);
}

void avc_configure_skip_control(void)
{
    WRITE_HREG(HCODEC_V3_SKIP_CONTROL,
        (1 << 31) | /* v3_skip_enable */
        (0 << 30) | /* v3_step_1_weight_enable */
        (1 << 28) | /* v3_mv_sad_weight_enable */
        (1 << 27) | /* v3_ipred_type_enable */
        (V3_FORCE_SKIP_SAD_1 << 12) |
        (V3_FORCE_SKIP_SAD_0 << 0));
    WRITE_HREG(HCODEC_V3_SKIP_WEIGHT,
        (V3_SKIP_WEIGHT_1 << 16) |
        (V3_SKIP_WEIGHT_0 << 0));
    WRITE_HREG(HCODEC_V3_L1_SKIP_MAX_SAD,
        (V3_LEVEL_1_F_SKIP_MAX_SAD << 16) |
        (V3_LEVEL_1_SKIP_MAX_SAD << 0));
    WRITE_HREG(HCODEC_V3_L2_SKIP_WEIGHT,
        (V3_FORCE_SKIP_SAD_2 << 16) |
        (V3_SKIP_WEIGHT_2 << 0));
}

void avc_configure_mv_sad_table(u32 *v3_mv_sad)
{
    int i;
    for (i = 0; i < 64; i++)
        WRITE_HREG(HCODEC_V3_MV_SAD_TABLE, v3_mv_sad[i]);
}

void avc_configure_ipred_weight(void)
{
    WRITE_HREG(HCODEC_V3_IPRED_TYPE_WEIGHT_0,
        (C_ipred_weight_H << 24) |
        (C_ipred_weight_V << 16) |
        (I4_ipred_weight_else << 8) |
        (I4_ipred_weight_most << 0));
    WRITE_HREG(HCODEC_V3_IPRED_TYPE_WEIGHT_1,
        (I16_ipred_weight_DC << 24) |
        (I16_ipred_weight_H << 16) |
        (I16_ipred_weight_V << 8) |
        (C_ipred_weight_DC << 0));
}

void avc_configure_left_small_max_sad(u32 v3_left_small_max_me_sad, u32 v3_left_small_max_ie_sad)
{
    WRITE_HREG(HCODEC_V3_LEFT_SMALL_MAX_SAD,
        (v3_left_small_max_me_sad << 16) |
        (v3_left_small_max_ie_sad << 0));
}

void avc_configure_svc_pic_type(struct encode_wq_s *wq, bool is_idr) {
    if (wq->pic.enable_svc && wq->pic.non_ref_cnt) {
        WRITE_HREG(H264_ENC_SVC_PIC_TYPE, ENC_SLC_NON_REF);
    } else {
        WRITE_HREG(H264_ENC_SVC_PIC_TYPE, ENC_SLC_REF);
    }
}

void avc_configure_fixed_slice(struct encode_wq_s *wq) {
    if (fixed_slice_cfg) {
        WRITE_HREG(FIXED_SLICE_CFG, fixed_slice_cfg);
    } else if (wq->pic.rows_per_slice != (wq->pic.encoder_height + 15) >> 4) {
        u32 mb_per_slice = (wq->pic.encoder_height + 15) >> 4;
        mb_per_slice = mb_per_slice * wq->pic.rows_per_slice;
        WRITE_HREG(FIXED_SLICE_CFG, mb_per_slice);
    } else {
        WRITE_HREG(FIXED_SLICE_CFG, 0);
    }
}

void avc_configure_encoding_mode(struct encode_wq_s *wq, bool is_idr) {
    u32 data32;

    data32 = READ_HREG(HCODEC_VLC_ADV_CONFIG);
    if (!is_idr) {
        // Configure for P frame
        data32 |= (1 << 10) | (1 << 7) | (1 << 6);
    } else {
        // Configure for I frame
        data32 &= ~((1 << 10) | (1 << 7) | (1 << 6));
    }
    WRITE_HREG(HCODEC_VLC_ADV_CONFIG, data32);

    data32 = READ_HREG(HCODEC_QDCT_ADV_CONFIG);
    if (!is_idr) {
        // Configure for P frame
        data32 |= (1 << 23) | (1 << 20) | (1 << 19) | (1 << 16);
    } else {
        // Configure for I frame
        data32 &= ~((1 << 23) | (1 << 20) | (1 << 19) | (1 << 16));
    }
    WRITE_HREG(HCODEC_QDCT_ADV_CONFIG, data32);

    if (is_idr) {
        WRITE_HREG(HCODEC_IE_ME_MODE, ie_me_mode);
        WRITE_HREG(HCODEC_IE_ME_MB_TYPE, HENC_MB_Type_I4MB);
    } else {
        WRITE_HREG(HCODEC_IE_ME_MODE, ie_me_mode |
                   (HENC_SKIP_RUN_AUTO << 16) |
                   (HENC_MB_TYPE_AUTO << 18) |
                   (HENC_COEFF_TOKEN_AUTO << 20));
        WRITE_HREG(HCODEC_IE_ME_MB_TYPE,
            (HENC_SKIP_RUN_AUTO << 16) |
            (HENC_MB_TYPE_AUTO << 4) |
            (HENC_MB_TYPE_AUTO << 0));
    }
}

void avc_configure_cbr_settings(struct encode_wq_s *wq) {
    if (get_cpu_type() >= MESON_CPU_MAJOR_ID_GXTVBB) {
        WRITE_HREG(H264_ENC_CBR_TABLE_ADDR, wq->mem.cbr_info_ddr_start_addr);
        WRITE_HREG(H264_ENC_CBR_MB_SIZE_ADDR, wq->mem.cbr_info_ddr_start_addr + CBR_TABLE_SIZE);
        WRITE_HREG(H264_ENC_CBR_CTL,
            (wq->cbr_info.start_tbl_id << 28) |
            (wq->cbr_info.short_shift << 24) |
            (wq->cbr_info.long_mb_num << 16) |
            (wq->cbr_info.long_th << 0));
        WRITE_HREG(H264_ENC_CBR_REGION_SIZE,
            (wq->cbr_info.block_w << 16) |
            (wq->cbr_info.block_h << 0));
    }
}

void avc_init_encode_weight(void) {
    me_mv_merge_ctl =
        (0x1 << 31) | (0x1 << 30) | (0x1 << 29) | (0x1 << 28) |
        (0x1 << 27) | (0x1 << 26) | (0x1 << 25) | (0x1 << 24) |
        (0x12 << 18) | (0x2b << 12) | (0x80 << 0);

    me_mv_weight_01 = (0x40 << 24) | (0x30 << 16) | (0x20 << 8) | 0x30;
    me_mv_weight_23 = (0x40 << 8) | 0x30;
    me_sad_range_inc = 0x03030303;
    me_step0_close_mv = 0x003ffc21;
    me_f_skip_sad = 0;
    me_f_skip_weight = 0;
    me_sad_enough_01 = 0x00018010;
    me_sad_enough_23 = 0x00000020;

    /* Additional weight initializations can be added here */
}

void avc_configure_v3_control(struct encode_wq_s *wq) {
    int i;

    /* Configure V3 skip control */
    WRITE_HREG(HCODEC_V3_SKIP_CONTROL,
        (1 << 31) | (0 << 30) | (1 << 28) | (1 << 27) |
        (V3_FORCE_SKIP_SAD_1 << 12) | (V3_FORCE_SKIP_SAD_0 << 0));
    WRITE_HREG(HCODEC_V3_SKIP_WEIGHT,
        (V3_SKIP_WEIGHT_1 << 16) | (V3_SKIP_WEIGHT_0 << 0));
    WRITE_HREG(HCODEC_V3_L1_SKIP_MAX_SAD,
        (V3_LEVEL_1_F_SKIP_MAX_SAD << 16) | (V3_LEVEL_1_SKIP_MAX_SAD << 0));
    WRITE_HREG(HCODEC_V3_L2_SKIP_WEIGHT,
        (V3_FORCE_SKIP_SAD_2 << 16) | (V3_SKIP_WEIGHT_2 << 0));

    /* Configure motion vector SAD table */
    for (i = 0; i < 64; i++)
        WRITE_HREG(HCODEC_V3_MV_SAD_TABLE, v3_mv_sad[i]);

    /* Configure intra prediction weights */
    WRITE_HREG(HCODEC_V3_IPRED_TYPE_WEIGHT_0,
        (C_ipred_weight_H << 24) | (C_ipred_weight_V << 16) |
        (I4_ipred_weight_else << 8) | (I4_ipred_weight_most << 0));
    WRITE_HREG(HCODEC_V3_IPRED_TYPE_WEIGHT_1,
        (I16_ipred_weight_DC << 24) | (I16_ipred_weight_H << 16) |
        (I16_ipred_weight_V << 8) | (C_ipred_weight_DC << 0));

    /* Configure left small max SAD */
    WRITE_HREG(HCODEC_V3_LEFT_SMALL_MAX_SAD,
        (v3_left_small_max_me_sad << 16) | (v3_left_small_max_ie_sad << 0));
}

void avc_configure_quant_params(struct encode_wq_s *wq, u32 quant) {
    u32 i_pic_qp, p_pic_qp, i_pic_qp_c, p_pic_qp_c;

    /* Calculate I and P frame QP values */
    i_pic_qp = p_pic_qp = quant;
    i_pic_qp_c = p_pic_qp_c = quant_table_c[quant];

    /* Configure quantization control registers */
    WRITE_HREG(HCODEC_QDCT_Q_QUANT_I,
        (i_pic_qp_c << 22) | (i_pic_qp << 16) |
        ((i_pic_qp_c % 6) << 12) | ((i_pic_qp_c / 6) << 8) |
        ((i_pic_qp % 6) << 4) | ((i_pic_qp / 6) << 0));

    WRITE_HREG(HCODEC_QDCT_Q_QUANT_P,
        (p_pic_qp_c << 22) | (p_pic_qp << 16) |
        ((p_pic_qp_c % 6) << 12) | ((p_pic_qp_c / 6) << 8) |
        ((p_pic_qp % 6) << 4) | ((p_pic_qp / 6) << 0));

    /* Configure VLC quantization control */
    WRITE_HREG(HCODEC_QDCT_VLC_QUANT_CTL_0,
        (0 << 19) | /* vlc_delta_quant_1 */
        (i_pic_qp << 13) | /* vlc_quant_1 */
        (0 << 6) | /* vlc_delta_quant_0 */
        (i_pic_qp << 0)); /* vlc_quant_0 */
    WRITE_HREG(HCODEC_QDCT_VLC_QUANT_CTL_1,
        (14 << 6) | /* vlc_max_delta_q_neg */
        (13 << 0)); /* vlc_max_delta_q_pos */
}

void avc_configure_ignore_config(void) {
#ifdef ENABLE_IGNORE_FUNCTION
    WRITE_HREG(HCODEC_IGNORE_CONFIG,
        (1 << 31) | /* ignore_lac_coeff_en */
        (1 << 26) | /* ignore_lac_coeff_else (<1) */
        (1 << 21) | /* ignore_lac_coeff_2 (<1) */
        (2 << 16) | /* ignore_lac_coeff_1 (<2) */
        (1 << 15) | /* ignore_cac_coeff_en */
        (1 << 10) | /* ignore_cac_coeff_else (<1) */
        (1 << 5)  | /* ignore_cac_coeff_2 (<1) */
        (3 << 0));  /* ignore_cac_coeff_1 (<2) */

    if (get_cpu_type() >= MESON_CPU_MAJOR_ID_GXTVBB)
        WRITE_HREG(HCODEC_IGNORE_CONFIG_2,
            (1 << 31) | /* ignore_t_lac_coeff_en */
            (1 << 26) | /* ignore_t_lac_coeff_else (<1) */
            (2 << 21) | /* ignore_t_lac_coeff_2 (<2) */
            (6 << 16) | /* ignore_t_lac_coeff_1 (<6) */
            (1 << 15) | /* ignore_cdc_coeff_en */
            /* ... additional configuration ... */
            (0 << 0));
    else
        WRITE_HREG(HCODEC_IGNORE_CONFIG_2,
            (1 << 31) | /* ignore_t_lac_coeff_en */
            (1 << 26) | /* ignore_t_lac_coeff_else (<1) */
            (1 << 21) | /* ignore_t_lac_coeff_2 (<1) */
            (5 << 16) | /* ignore_t_lac_coeff_1 (<5) */
            (0 << 0));
#else
    WRITE_HREG(HCODEC_IGNORE_CONFIG, 0);
    WRITE_HREG(HCODEC_IGNORE_CONFIG_2, 0);
#endif
}
