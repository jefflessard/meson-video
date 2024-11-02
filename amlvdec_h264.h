#include "amlvdec_vdec.h"

#define MB_MV_SIZE 96

#define MC_OFFSET_HEADER    0x0000
#define MC_OFFSET_DATA      0x1000
#define MC_OFFSET_MMCO      0x2000
#define MC_OFFSET_LIST      0x3000
#define MC_OFFSET_SLICE     0x4000
#define MC_OFFSET_MAIN      0x5000

#define MC_MAX_SIZE         (16 * SZ_4K)
#define MC_AMRISC_SIZE      (SZ_4K)
#define MC_TOTAL_SIZE       ((20 + 16) * SZ_1K)
#define MC_SWAP_SIZE        (4 * SZ_1K)
#define LMEM_SIZE           PAGE_SIZE
#define AUX_PREFIX_SIZE     (16 * SZ_1K)
#define AUX_SUFFIX_SIZE     (0)
#define AUX_SIZE            AUX_PREFIX_SIZE + AUX_SUFFIX_SIZE
#define WORKSPACE_SIZE      0x200000
#define WORKSPACE_OFFSET    (64 * SZ_1K)

#define ASSIST_MBOX1_CLR_REG VDEC_ASSIST_MBOX1_CLR_REG
#define ASSIST_MBOX1_MASK    VDEC_ASSIST_MBOX1_MASK
#define ASSIST_MBOX1_IRQ_REG VDEC_ASSIST_MBOX1_IRQ_REG

/* unmapped                 AV_SCRATCH_0 */
	/* (max_reference_size << 24) |
	 *     (dpb.mDPB.size << 16) |
	 *     (dpb.mDPB.size << 8) */
/* unmapped                 AV_SCRATCH_1 */
	/* param1 / seq_info2
	 * mb_width = param1 & 0xff;
	 * mb_total = (param1 >> 8) & 0xffff;
	 * mb_mv_byte = (seq_info2 & 0x80000000) ? 24 : 96;*/
	/* reserved_byte*8
	 *  reused to store the reserved bit_cnt for one packet multi-frame */
#define INIT_FLAG_REG       AV_SCRATCH_2
	/* init_flag 0-1 */
	/* param2 / seq_info
	 * frame_mbs_only_flag = (seq_info >> 15) & 0x01;
	 * chroma_format_idc = (seq_info >> 13) & 0x03; */
	/* bit 15: frame_mbs_only_flag
	 * bit 13-14: chroma_format_idc */
#define HEAD_PADDING_REG    AV_SCRATCH_3
	/* set to 0 */
#define H264_DECODE_MODE    AV_SCRATCH_4
	#define DECODE_MODE_SINGLE            0x0
	#define DECODE_MODE_MULTI_FRAMEBASE   0x1
	#define DECODE_MODE_MULTI_STREAMBASE  0x2
	#define DECODE_MODE_MULTI_DVBAL       0x3
	#define DECODE_MODE_MULTI_DVENL       0x4
#define H264_DECODE_SEQINFO	AV_SCRATCH_5
	/* set to seq_info2 */
/* unmapped                 AV_SCRATCH_6 */
	/* param3 */
	/* @AV_SCRATCH_6.31-16 = (left << 8 | right ) << 1
	 * @AV_SCRATCH_6.15-0  = (top << 8 | bottom ) << (2 - frame_mbs_only_flag) */
#define UCODE_WATCHDOG_REG  AV_SCRATCH_7
	/* (max_reference_size << 24) |
	 *      (dpb.mDPB.size << 16) |
	 *      (dpb.mDPB.size << 8) */
/* unmapped                 AV_SCRATCH_8 */
	/* workspace paddr + workspace offset */
#define ERROR_STATUS_REG	AV_SCRATCH_9
	/* set to 0 */
#define NAL_SEARCH_CTL		AV_SCRATCH_9
	/* NAL_SEARCH_CTL: bit 0, enable itu_t35
	 * NAL_SEARCH_CTL: bit 1, enable mmu
	 * NAL_SEARCH_CTL: bit 2, detect frame_mbs_only_flag whether switch resolution
	 * NAL_SEARCH_CTL: bit 7-14,level_idc
	 * NAL_SEARCH_CTL: bit 15,bitstream_restriction_flag */
#define RPM_CMD_REG         AV_SCRATCH_A
	/* related to SEND_PARAM_WITH_REG */
/* unmapped                 AV_SCRATCH_B */
	/* param4 / seq_info3
	   level_idc = param4 & 0xff
	   max_reference_size = (param4 >> 8) & 0xff */
#define H264_AUX_ADR        AV_SCRATCH_C
	/* aux_phy_addr */
/* unmapped                 AV_SCRATCH_D */
#define H264_DECODE_SIZE	AV_SCRATCH_E
	/* decode_size */
/* unmapped                 AV_SCRATCH_F */
	/* ((error_recovery_mode_in & 0x1) << 4) |
	 *    ((UCODE_IP_ONLY_PARAM & 0x1) << 6) */
/* unmapped                 AV_SCRATCH_G */
	/* mc_dma_handle */
#define H264_AUX_DATA_SIZE  AV_SCRATCH_H
	/* ((prefix_aux_size >> 4) << 16) |
	 *        (suffix_aux_size >> 4) */
#define FRAME_COUNTER_REG   AV_SCRATCH_I
	/* decode_pic_count */
#define DPB_STATUS_REG      AV_SCRATCH_J
	#define H264_SLICE_HEAD_DONE         0x01
	#define H264_PIC_DATA_DONE           0x02
	/*#define H264_SPS_DONE              0x03*/
	/*#define H264_PPS_DONE              0x04*/
	/*#define H264_SLICE_DATA_DONE       0x05*/
	/*#define H264_DATA_END              0x06*/
	#define H264_CONFIG_REQUEST          0x11
	#define H264_DATA_REQUEST            0x12
	#define H264_WRRSP_REQUEST           0x13
	#define H264_WRRSP_DONE              0x14
	#define H264_DECODE_BUFEMPTY         0x20
	#define H264_DECODE_TIMEOUT          0x21
	#define H264_SEARCH_BUFEMPTY         0x22
	#define H264_DECODE_OVER_SIZE        0x23
	#define VIDEO_SIGNAL_LOW             0x26
	#define VIDEO_SIGNAL_HIGH            0x27
	#define H264_FIND_NEXT_PIC_NAL       0x50
	#define H264_FIND_NEXT_DVEL_NAL      0x51
	#define H264_AUX_DATA_READY          0x52
	#define H264_SEI_DATA_READY          0x53
	#define H264_SEI_DATA_DONE           0x54
	#define H264_STATE_SEARCH_AFTER_SPS  0x80
	#define H264_STATE_SEARCH_AFTER_PPS  0x81
	#define H264_STATE_PARSE_SLICE_HEAD  0x82
	#define H264_STATE_SEARCH_HEAD       0x83
	#define H264_ACTION_SEARCH_HEAD      0xf0
	#define H264_ACTION_DECODE_SLICE     0xf1
	#define H264_ACTION_CONFIG_DONE      0xf2
	#define H264_ACTION_DECODE_NEWPIC    0xf3
	#define H264_ACTION_DECODE_START     0xff
/* unmapped                 AV_SCRATCH_K */
	/* udebug_flag:
	 *     bit 0, enable ucode print
	 *     bit 1, enable ucode detail print
	 *     bit 3, disable ucode watchdog
	 *     bit [31:16] not 0, pos to dump lmem
	 *                 bit 2, pop bits to lmem
	 *     bit [11:8], pre-pop bits for alignment (when bit 2 is 1) */
#define LMEM_DUMP_ADR       AV_SCRATCH_L
	/* lmem_phy_addr */
#define DEBUG_REG1          AV_SCRATCH_M
	/* debug_tag, set to 0 */
#define DEBUG_REG2          AV_SCRATCH_N
	/* udebug_pause_val, set to 0 */
#define H264_DECODE_INFO    M4_CONTROL_REG /* 0xc29 */
	/* set to (1<<13) */
#define MBY_MBX             MB_MOTION_MODE /*0xc07*/
	/* decode_mb_count = 
	*      ((mby_mbx & 0xff) * mb_width +
	*      (((mby_mbx >> 8) & 0xff) + 1)) */

#define H264_BUFFER_INFO_INDEX    PMV3_X /* 0xc24 */
#define H264_BUFFER_INFO_DATA   PMV2_X  /* 0xc22 */
#define H264_CURRENT_POC_IDX_RESET LAST_SLICE_MV_ADDR /* 0xc30 */
#define H264_CURRENT_POC          LAST_MVY /* 0xc32 shared with conceal MV */

#define H264_CO_MB_WR_ADDR        VLD_C38 /* 0xc38 */
/* bit 31:30 -- L1[0] picture coding structure,
 *	00 - top field,	01 - bottom field,
 *	10 - frame, 11 - mbaff frame
 *   bit 29 - L1[0] top/bot for B field picture , 0 - top, 1 - bot
 *   bit 28:0 h264_co_mb_mem_rd_addr[31:3]
 *	-- only used for B Picture Direct mode [2:0] will set to 3'b000
 */
#define H264_CO_MB_RD_ADDR        VLD_C39 /* 0xc39 */

/* bit 15 -- flush co_mb_data to DDR -- W-Only
 *   bit 14 -- h264_co_mb_mem_wr_addr write Enable -- W-Only
 *   bit 13 -- h264_co_mb_info_wr_ptr write Enable -- W-Only
 *   bit 9 -- soft_reset -- W-Only
 *   bit 8 -- upgent
 *   bit 7:2 -- h264_co_mb_mem_wr_addr
 *   bit 1:0 -- h264_co_mb_info_wr_ptr
 */
#define H264_CO_MB_RW_CTL         VLD_C3D /* 0xc3d */
#define DCAC_DDR_BYTE64_CTL                   0x0e1d

