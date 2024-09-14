/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef VDEC2_REGS_HEADER_
#define VDEC2_REGS_HEADER_

#define VDEC2_ASSIST_MMC_CTRL0           0x2001
#define VDEC2_ASSIST_MMC_CTRL1           0x2002
#define VDEC2_ASSIST_AMR1_INT0           0x2025
#define VDEC2_ASSIST_AMR1_INT1           0x2026
#define VDEC2_ASSIST_AMR1_INT2           0x2027
#define VDEC2_ASSIST_AMR1_INT3           0x2028
#define VDEC2_ASSIST_AMR1_INT4           0x2029
#define VDEC2_ASSIST_AMR1_INT5           0x202a
#define VDEC2_ASSIST_AMR1_INT6           0x202b
#define VDEC2_ASSIST_AMR1_INT7           0x202c
#define VDEC2_ASSIST_AMR1_INT8           0x202d
#define VDEC2_ASSIST_AMR1_INT9           0x202e
#define VDEC2_ASSIST_AMR1_INTA           0x202f
#define VDEC2_ASSIST_AMR1_INTB           0x2030
#define VDEC2_ASSIST_AMR1_INTC           0x2031
#define VDEC2_ASSIST_AMR1_INTD           0x2032
#define VDEC2_ASSIST_AMR1_INTE           0x2033
#define VDEC2_ASSIST_AMR1_INTF           0x2034
#define VDEC2_ASSIST_AMR2_INT0           0x2035
#define VDEC2_ASSIST_AMR2_INT1           0x2036
#define VDEC2_ASSIST_AMR2_INT2           0x2037
#define VDEC2_ASSIST_AMR2_INT3           0x2038
#define VDEC2_ASSIST_AMR2_INT4           0x2039
#define VDEC2_ASSIST_AMR2_INT5           0x203a
#define VDEC2_ASSIST_AMR2_INT6           0x203b
#define VDEC2_ASSIST_AMR2_INT7           0x203c
#define VDEC2_ASSIST_AMR2_INT8           0x203d
#define VDEC2_ASSIST_AMR2_INT9           0x203e
#define VDEC2_ASSIST_AMR2_INTA           0x203f
#define VDEC2_ASSIST_AMR2_INTB           0x2040
#define VDEC2_ASSIST_AMR2_INTC           0x2041
#define VDEC2_ASSIST_AMR2_INTD           0x2042
#define VDEC2_ASSIST_AMR2_INTE           0x2043
#define VDEC2_ASSIST_AMR2_INTF           0x2044
#define VDEC2_ASSIST_MBX_SSEL            0x2045
#define VDEC2_ASSIST_TIMER0_LO           0x2060
#define VDEC2_ASSIST_TIMER0_HI           0x2061
#define VDEC2_ASSIST_TIMER1_LO           0x2062
#define VDEC2_ASSIST_TIMER1_HI           0x2063
#define VDEC2_ASSIST_DMA_INT             0x2064
#define VDEC2_ASSIST_DMA_INT_MSK         0x2065
#define VDEC2_ASSIST_DMA_INT2            0x2066
#define VDEC2_ASSIST_DMA_INT_MSK2        0x2067
#define VDEC2_ASSIST_MBOX0_IRQ_REG       0x2070
#define VDEC2_ASSIST_MBOX0_CLR_REG       0x2071
#define VDEC2_ASSIST_MBOX0_MASK          0x2072
#define VDEC2_ASSIST_MBOX0_FIQ_SEL       0x2073
#define VDEC2_ASSIST_MBOX1_IRQ_REG       0x2074
#define VDEC2_ASSIST_MBOX1_CLR_REG       0x2075
#define VDEC2_ASSIST_MBOX1_MASK          0x2076
#define VDEC2_ASSIST_MBOX1_FIQ_SEL       0x2077
#define VDEC2_ASSIST_MBOX2_IRQ_REG       0x2078
#define VDEC2_ASSIST_MBOX2_CLR_REG       0x2079
#define VDEC2_ASSIST_MBOX2_MASK          0x207a
#define VDEC2_ASSIST_MBOX2_FIQ_SEL       0x207b

#define VDEC2_MSP                        0x2300
#define VDEC2_MPSR                       0x2301
#define VDEC2_MINT_VEC_BASE              0x2302
#define VDEC2_MCPU_INTR_GRP              0x2303
#define VDEC2_MCPU_INTR_MSK              0x2304
#define VDEC2_MCPU_INTR_REQ              0x2305
#define VDEC2_MPC_P                      0x2306
#define VDEC2_MPC_D                      0x2307
#define VDEC2_MPC_E                      0x2308
#define VDEC2_MPC_W                      0x2309
#define VDEC2_MINDEX0_REG                0x230a
#define VDEC2_MINDEX1_REG                0x230b
#define VDEC2_MINDEX2_REG                0x230c
#define VDEC2_MINDEX3_REG                0x230d
#define VDEC2_MINDEX4_REG                0x230e
#define VDEC2_MINDEX5_REG                0x230f
#define VDEC2_MINDEX6_REG                0x2310
#define VDEC2_MINDEX7_REG                0x2311
#define VDEC2_MMIN_REG                   0x2312
#define VDEC2_MMAX_REG                   0x2313
#define VDEC2_MBREAK0_REG                0x2314
#define VDEC2_MBREAK1_REG                0x2315
#define VDEC2_MBREAK2_REG                0x2316
#define VDEC2_MBREAK3_REG                0x2317
#define VDEC2_MBREAK_TYPE                0x2318
#define VDEC2_MBREAK_CTRL                0x2319
#define VDEC2_MBREAK_STAUTS              0x231a
#define VDEC2_MDB_ADDR_REG               0x231b
#define VDEC2_MDB_DATA_REG               0x231c
#define VDEC2_MDB_CTRL                   0x231d
#define VDEC2_MSFTINT0                   0x231e
#define VDEC2_MSFTINT1                   0x231f
#define VDEC2_CSP                        0x2320
#define VDEC2_CPSR                       0x2321
#define VDEC2_CINT_VEC_BASE              0x2322
#define VDEC2_CCPU_INTR_GRP              0x2323
#define VDEC2_CCPU_INTR_MSK              0x2324
#define VDEC2_CCPU_INTR_REQ              0x2325
#define VDEC2_CPC_P                      0x2326
#define VDEC2_CPC_D                      0x2327
#define VDEC2_CPC_E                      0x2328
#define VDEC2_CPC_W                      0x2329
#define VDEC2_CINDEX0_REG                0x232a
#define VDEC2_CINDEX1_REG                0x232b
#define VDEC2_CINDEX2_REG                0x232c
#define VDEC2_CINDEX3_REG                0x232d
#define VDEC2_CINDEX4_REG                0x232e
#define VDEC2_CINDEX5_REG                0x232f
#define VDEC2_CINDEX6_REG                0x2330
#define VDEC2_CINDEX7_REG                0x2331
#define VDEC2_CMIN_REG                   0x2332
#define VDEC2_CMAX_REG                   0x2333
#define VDEC2_CBREAK0_REG                0x2334
#define VDEC2_CBREAK1_REG                0x2335
#define VDEC2_CBREAK2_REG                0x2336
#define VDEC2_CBREAK3_REG                0x2337
#define VDEC2_CBREAK_TYPE                0x2338
#define VDEC2_CBREAK_CTRL                0x2339
#define VDEC2_CBREAK_STAUTS              0x233a
#define VDEC2_CDB_ADDR_REG               0x233b
#define VDEC2_CDB_DATA_REG               0x233c
#define VDEC2_CDB_CTRL                   0x233d
#define VDEC2_CSFTINT0                   0x233e
#define VDEC2_CSFTINT1                   0x233f
#define VDEC2_IMEM_DMA_CTRL              0x2340
#define VDEC2_IMEM_DMA_ADR               0x2341
#define VDEC2_IMEM_DMA_COUNT             0x2342
#define VDEC2_WRRSP_IMEM                 0x2343
#define VDEC2_LMEM_DMA_CTRL              0x2350
#define VDEC2_LMEM_DMA_ADR               0x2351
#define VDEC2_LMEM_DMA_COUNT             0x2352
#define VDEC2_WRRSP_LMEM                 0x2353
#define VDEC2_MAC_CTRL1                  0x2360
#define VDEC2_ACC0REG1                   0x2361
#define VDEC2_ACC1REG1                   0x2362
#define VDEC2_MAC_CTRL2                  0x2370
#define VDEC2_ACC0REG2                   0x2371
#define VDEC2_ACC1REG2                   0x2372
#define VDEC2_CPU_TRACE                  0x2380

#define VDEC2_MC_CTRL_REG                0x2900
#define VDEC2_MC_MB_INFO                 0x2901
#define VDEC2_MC_PIC_INFO                0x2902
#define VDEC2_MC_HALF_PEL_ONE            0x2903
#define VDEC2_MC_HALF_PEL_TWO            0x2904
#define VDEC2_POWER_CTL_MC               0x2905
#define VDEC2_MC_CMD                     0x2906
#define VDEC2_MC_CTRL0                   0x2907
#define VDEC2_MC_PIC_W_H                 0x2908
#define VDEC2_MC_STATUS0                 0x2909
#define VDEC2_MC_STATUS1                 0x290a
#define VDEC2_MC_CTRL1                   0x290b
#define VDEC2_MC_MIX_RATIO0              0x290c
#define VDEC2_MC_MIX_RATIO1              0x290d
#define VDEC2_MC_DP_MB_XY                0x290e
#define VDEC2_MC_OM_MB_XY                0x290f
#define VDEC2_PSCALE_RST                 0x2910
#define VDEC2_PSCALE_CTRL                0x2911
#define VDEC2_PSCALE_PICI_W              0x2912
#define VDEC2_PSCALE_PICI_H              0x2913
#define VDEC2_PSCALE_PICO_W              0x2914
#define VDEC2_PSCALE_PICO_H              0x2915
#define VDEC2_PSCALE_PICO_START_X        0x2916
#define VDEC2_PSCALE_PICO_START_Y        0x2917
#define VDEC2_PSCALE_DUMMY               0x2918
#define VDEC2_PSCALE_FILT0_COEF0         0x2919
#define VDEC2_PSCALE_FILT0_COEF1         0x291a
#define VDEC2_PSCALE_CMD_CTRL            0x291b
#define VDEC2_PSCALE_CMD_BLK_X           0x291c
#define VDEC2_PSCALE_CMD_BLK_Y           0x291d
#define VDEC2_PSCALE_STATUS              0x291e
#define VDEC2_PSCALE_BMEM_ADDR           0x291f
#define VDEC2_PSCALE_BMEM_DAT            0x2920
#define VDEC2_PSCALE_DRAM_BUF_CTRL       0x2921
#define VDEC2_PSCALE_MCMD_CTRL           0x2922
#define VDEC2_PSCALE_MCMD_XSIZE          0x2923
#define VDEC2_PSCALE_MCMD_YSIZE          0x2924
#define VDEC2_PSCALE_RBUF_START_BLKX     0x2925
#define VDEC2_PSCALE_RBUF_START_BLKY     0x2926
#define VDEC2_PSCALE_PICO_SHIFT_XY       0x2928
#define VDEC2_PSCALE_CTRL1               0x2929
#define VDEC2_PSCALE_SRCKEY_CTRL0        0x292a
#define VDEC2_PSCALE_SRCKEY_CTRL1        0x292b
#define VDEC2_PSCALE_CANVAS_RD_ADDR      0x292c
#define VDEC2_PSCALE_CANVAS_WR_ADDR      0x292d
#define VDEC2_PSCALE_CTRL2               0x292e
/*add from M8M2*/
#define VDEC2_HDEC_MC_OMEM_AUTO          0x2930
#define VDEC2_HDEC_MC_MBRIGHT_IDX        0x2931
#define VDEC2_HDEC_MC_MBRIGHT_RD         0x2932
/**/
#define VDEC2_MC_MPORT_CTRL              0x2940
#define VDEC2_MC_MPORT_DAT               0x2941
#define VDEC2_MC_WT_PRED_CTRL            0x2942
#define VDEC2_MC_MBBOT_ST_EVEN_ADDR      0x2944
#define VDEC2_MC_MBBOT_ST_ODD_ADDR       0x2945
#define VDEC2_MC_DPDN_MB_XY              0x2946
#define VDEC2_MC_OMDN_MB_XY              0x2947
#define VDEC2_MC_HCMDBUF_H               0x2948
#define VDEC2_MC_HCMDBUF_L               0x2949
#define VDEC2_MC_HCMD_H                  0x294a
#define VDEC2_MC_HCMD_L                  0x294b
#define VDEC2_MC_IDCT_DAT                0x294c
#define VDEC2_MC_CTRL_GCLK_CTRL          0x294d
#define VDEC2_MC_OTHER_GCLK_CTRL         0x294e
#define VDEC2_MC_CTRL2                   0x294f
#define VDEC2_MDEC_PIC_DC_CTRL           0x298e
#define VDEC2_MDEC_PIC_DC_STATUS         0x298f
#define VDEC2_ANC0_CANVAS_ADDR           0x2990
#define VDEC2_ANC1_CANVAS_ADDR           0x2991
#define VDEC2_ANC2_CANVAS_ADDR           0x2992
#define VDEC2_ANC3_CANVAS_ADDR           0x2993
#define VDEC2_ANC4_CANVAS_ADDR           0x2994
#define VDEC2_ANC5_CANVAS_ADDR           0x2995
#define VDEC2_ANC6_CANVAS_ADDR           0x2996
#define VDEC2_ANC7_CANVAS_ADDR           0x2997
#define VDEC2_ANC8_CANVAS_ADDR           0x2998
#define VDEC2_ANC9_CANVAS_ADDR           0x2999
#define VDEC2_ANC10_CANVAS_ADDR          0x299a
#define VDEC2_ANC11_CANVAS_ADDR          0x299b
#define VDEC2_ANC12_CANVAS_ADDR          0x299c
#define VDEC2_ANC13_CANVAS_ADDR          0x299d
#define VDEC2_ANC14_CANVAS_ADDR          0x299e
#define VDEC2_ANC15_CANVAS_ADDR          0x299f
#define VDEC2_ANC16_CANVAS_ADDR          0x29a0
#define VDEC2_ANC17_CANVAS_ADDR          0x29a1
#define VDEC2_ANC18_CANVAS_ADDR          0x29a2
#define VDEC2_ANC19_CANVAS_ADDR          0x29a3
#define VDEC2_ANC20_CANVAS_ADDR          0x29a4
#define VDEC2_ANC21_CANVAS_ADDR          0x29a5
#define VDEC2_ANC22_CANVAS_ADDR          0x29a6
#define VDEC2_ANC23_CANVAS_ADDR          0x29a7
#define VDEC2_ANC24_CANVAS_ADDR          0x29a8
#define VDEC2_ANC25_CANVAS_ADDR          0x29a9
#define VDEC2_ANC26_CANVAS_ADDR          0x29aa
#define VDEC2_ANC27_CANVAS_ADDR          0x29ab
#define VDEC2_ANC28_CANVAS_ADDR          0x29ac
#define VDEC2_ANC29_CANVAS_ADDR          0x29ad
#define VDEC2_ANC30_CANVAS_ADDR          0x29ae
#define VDEC2_ANC31_CANVAS_ADDR          0x29af
#define VDEC2_DBKR_CANVAS_ADDR           0x29b0
#define VDEC2_DBKW_CANVAS_ADDR           0x29b1
#define VDEC2_REC_CANVAS_ADDR            0x29b2
#define VDEC2_CURR_CANVAS_CTRL           0x29b3
#define VDEC2_MDEC_PIC_DC_THRESH         0x29b8
#define VDEC2_MDEC_PICR_BUF_STATUS       0x29b9
#define VDEC2_MDEC_PICW_BUF_STATUS       0x29ba
#define VDEC2_MCW_DBLK_WRRSP_CNT         0x29bb
#define VDEC2_MC_MBBOT_WRRSP_CNT         0x29bc
#define VDEC2_MDEC_PICW_BUF2_STATUS      0x29bd
#define VDEC2_WRRSP_FIFO_PICW_DBK        0x29be
#define VDEC2_WRRSP_FIFO_PICW_MC         0x29bf
#define VDEC2_AV_SCRATCH_0               0x29c0
#define VDEC2_AV_SCRATCH_1               0x29c1
#define VDEC2_AV_SCRATCH_2               0x29c2
#define VDEC2_AV_SCRATCH_3               0x29c3
#define VDEC2_AV_SCRATCH_4               0x29c4
#define VDEC2_AV_SCRATCH_5               0x29c5
#define VDEC2_AV_SCRATCH_6               0x29c6
#define VDEC2_AV_SCRATCH_7               0x29c7
#define VDEC2_AV_SCRATCH_8               0x29c8
#define VDEC2_AV_SCRATCH_9               0x29c9
#define VDEC2_AV_SCRATCH_A               0x29ca
#define VDEC2_AV_SCRATCH_B               0x29cb
#define VDEC2_AV_SCRATCH_C               0x29cc
#define VDEC2_AV_SCRATCH_D               0x29cd
#define VDEC2_AV_SCRATCH_E               0x29ce
#define VDEC2_AV_SCRATCH_F               0x29cf
#define VDEC2_AV_SCRATCH_G               0x29d0
#define VDEC2_AV_SCRATCH_H               0x29d1
#define VDEC2_AV_SCRATCH_I               0x29d2
#define VDEC2_AV_SCRATCH_J               0x29d3
#define VDEC2_AV_SCRATCH_K               0x29d4
#define VDEC2_AV_SCRATCH_L               0x29d5
#define VDEC2_AV_SCRATCH_M               0x29d6
#define VDEC2_AV_SCRATCH_N               0x29d7
#define VDEC2_WRRSP_CO_MB                0x29d8
#define VDEC2_WRRSP_DCAC                 0x29d9
/*add from M8M2*/
#define VDEC2_WRRSP_VLD                  0x29da
#define VDEC2_MDEC_DOUBLEW_CFG0          0x29db
#define VDEC2_MDEC_DOUBLEW_CFG1          0x29dc
#define VDEC2_MDEC_DOUBLEW_CFG2          0x29dd
#define VDEC2_MDEC_DOUBLEW_CFG3          0x29de
#define VDEC2_MDEC_DOUBLEW_CFG4          0x29df
#define VDEC2_MDEC_DOUBLEW_CFG5          0x29e0
#define VDEC2_MDEC_DOUBLEW_CFG6          0x29e1
#define VDEC2_MDEC_DOUBLEW_CFG7          0x29e2
#define VDEC2_MDEC_DOUBLEW_STATUS        0x29e3
/**/
#define VDEC2_DBLK_RST                   0x2950
#define VDEC2_DBLK_CTRL                  0x2951
#define VDEC2_DBLK_MB_WID_HEIGHT         0x2952
#define VDEC2_DBLK_STATUS                0x2953
#define VDEC2_DBLK_CMD_CTRL              0x2954
#define VDEC2_DBLK_MB_XY                 0x2955
#define VDEC2_DBLK_QP                    0x2956
#define VDEC2_DBLK_Y_BHFILT              0x2957
#define VDEC2_DBLK_Y_BHFILT_HIGH         0x2958
#define VDEC2_DBLK_Y_BVFILT              0x2959
#define VDEC2_DBLK_CB_BFILT              0x295a
#define VDEC2_DBLK_CR_BFILT              0x295b
#define VDEC2_DBLK_Y_HFILT               0x295c
#define VDEC2_DBLK_Y_HFILT_HIGH          0x295d
#define VDEC2_DBLK_Y_VFILT               0x295e
#define VDEC2_DBLK_CB_FILT               0x295f
#define VDEC2_DBLK_CR_FILT               0x2960
#define VDEC2_DBLK_BETAX_QP_SEL          0x2961
#define VDEC2_DBLK_CLIP_CTRL0            0x2962
#define VDEC2_DBLK_CLIP_CTRL1            0x2963
#define VDEC2_DBLK_CLIP_CTRL2            0x2964
#define VDEC2_DBLK_CLIP_CTRL3            0x2965
#define VDEC2_DBLK_CLIP_CTRL4            0x2966
#define VDEC2_DBLK_CLIP_CTRL5            0x2967
#define VDEC2_DBLK_CLIP_CTRL6            0x2968
#define VDEC2_DBLK_CLIP_CTRL7            0x2969
#define VDEC2_DBLK_CLIP_CTRL8            0x296a
#define VDEC2_DBLK_STATUS1               0x296b
#define VDEC2_DBLK_GCLK_FREE             0x296c
#define VDEC2_DBLK_GCLK_OFF              0x296d
#define VDEC2_DBLK_AVSFLAGS              0x296e
#define VDEC2_DBLK_CBPY                  0x2970
#define VDEC2_DBLK_CBPY_ADJ              0x2971
#define VDEC2_DBLK_CBPC                  0x2972
#define VDEC2_DBLK_CBPC_ADJ              0x2973
#define VDEC2_DBLK_VHMVD                 0x2974
#define VDEC2_DBLK_STRONG                0x2975
#define VDEC2_DBLK_RV8_QUANT             0x2976
#define VDEC2_DBLK_CBUS_HCMD2            0x2977
#define VDEC2_DBLK_CBUS_HCMD1            0x2978
#define VDEC2_DBLK_CBUS_HCMD0            0x2979
#define VDEC2_DBLK_VLD_HCMD2             0x297a
#define VDEC2_DBLK_VLD_HCMD1             0x297b
#define VDEC2_DBLK_VLD_HCMD0             0x297c
#define VDEC2_DBLK_OST_YBASE             0x297d
#define VDEC2_DBLK_OST_CBCRDIFF          0x297e
#define VDEC2_DBLK_CTRL1                 0x297f
#define VDEC2_MCRCC_CTL1                 0x2980
#define VDEC2_MCRCC_CTL2                 0x2981
#define VDEC2_MCRCC_CTL3                 0x2982
#define VDEC2_GCLK_EN                    0x2983
#define VDEC2_MDEC_SW_RESET              0x2984

#define VDEC2_VLD_STATUS_CTRL            0x2c00
#define VDEC2_MPEG1_2_REG                0x2c01
#define VDEC2_F_CODE_REG                 0x2c02
#define VDEC2_PIC_HEAD_INFO              0x2c03
#define VDEC2_SLICE_VER_POS_PIC_TYPE     0x2c04
#define VDEC2_QP_VALUE_REG               0x2c05
#define VDEC2_MBA_INC                    0x2c06
#define VDEC2_MB_MOTION_MODE             0x2c07
#define VDEC2_POWER_CTL_VLD              0x2c08
#define VDEC2_MB_WIDTH                   0x2c09
#define VDEC2_SLICE_QP                   0x2c0a
#define VDEC2_PRE_START_CODE             0x2c0b
#define VDEC2_SLICE_START_BYTE_01        0x2c0c
#define VDEC2_SLICE_START_BYTE_23        0x2c0d
#define VDEC2_RESYNC_MARKER_LENGTH       0x2c0e
#define VDEC2_DECODER_BUFFER_INFO        0x2c0f
#define VDEC2_FST_FOR_MV_X               0x2c10
#define VDEC2_FST_FOR_MV_Y               0x2c11
#define VDEC2_SCD_FOR_MV_X               0x2c12
#define VDEC2_SCD_FOR_MV_Y               0x2c13
#define VDEC2_FST_BAK_MV_X               0x2c14
#define VDEC2_FST_BAK_MV_Y               0x2c15
#define VDEC2_SCD_BAK_MV_X               0x2c16
#define VDEC2_SCD_BAK_MV_Y               0x2c17
#define VDEC2_VLD_DECODE_CONTROL         0x2c18
#define VDEC2_VLD_REVERVED_19            0x2c19
#define VDEC2_VIFF_BIT_CNT               0x2c1a
#define VDEC2_BYTE_ALIGN_PEAK_HI         0x2c1b
#define VDEC2_BYTE_ALIGN_PEAK_LO         0x2c1c
#define VDEC2_NEXT_ALIGN_PEAK            0x2c1d
#define VDEC2_VC1_CONTROL_REG            0x2c1e
#define VDEC2_PMV1_X                     0x2c20
#define VDEC2_PMV1_Y                     0x2c21
#define VDEC2_PMV2_X                     0x2c22
#define VDEC2_PMV2_Y                     0x2c23
#define VDEC2_PMV3_X                     0x2c24
#define VDEC2_PMV3_Y                     0x2c25
#define VDEC2_PMV4_X                     0x2c26
#define VDEC2_PMV4_Y                     0x2c27
#define VDEC2_M4_TABLE_SELECT            0x2c28
#define VDEC2_M4_CONTROL_REG             0x2c29
#define VDEC2_BLOCK_NUM                  0x2c2a
#define VDEC2_PATTERN_CODE               0x2c2b
#define VDEC2_MB_INFO                    0x2c2c
#define VDEC2_VLD_DC_PRED                0x2c2d
#define VDEC2_VLD_ERROR_MASK             0x2c2e
#define VDEC2_VLD_DC_PRED_C              0x2c2f
#define VDEC2_LAST_SLICE_MV_ADDR         0x2c30
#define VDEC2_LAST_MVX                   0x2c31
#define VDEC2_LAST_MVY                   0x2c32
#define VDEC2_VLD_C38                    0x2c38
#define VDEC2_VLD_C39                    0x2c39
#define VDEC2_VLD_STATUS                 0x2c3a
#define VDEC2_VLD_SHIFT_STATUS           0x2c3b
#define VDEC2_VOFF_STATUS                0x2c3c
#define VDEC2_VLD_C3D                    0x2c3d
#define VDEC2_VLD_DBG_INDEX              0x2c3e
#define VDEC2_VLD_DBG_DATA               0x2c3f
#define VDEC2_VLD_MEM_VIFIFO_START_PTR   0x2c40
#define VDEC2_VLD_MEM_VIFIFO_CURR_PTR    0x2c41
#define VDEC2_VLD_MEM_VIFIFO_END_PTR     0x2c42
#define VDEC2_VLD_MEM_VIFIFO_BYTES_AVAIL 0x2c43
#define VDEC2_VLD_MEM_VIFIFO_CONTROL     0x2c44
#define VDEC2_VLD_MEM_VIFIFO_WP          0x2c45
#define VDEC2_VLD_MEM_VIFIFO_RP          0x2c46
#define VDEC2_VLD_MEM_VIFIFO_LEVEL       0x2c47
#define VDEC2_VLD_MEM_VIFIFO_BUF_CNTL    0x2c48
#define VDEC2_VLD_TIME_STAMP_CNTL        0x2c49
#define VDEC2_VLD_TIME_STAMP_SYNC_0      0x2c4a
#define VDEC2_VLD_TIME_STAMP_SYNC_1      0x2c4b
#define VDEC2_VLD_TIME_STAMP_0           0x2c4c
#define VDEC2_VLD_TIME_STAMP_1           0x2c4d
#define VDEC2_VLD_TIME_STAMP_2           0x2c4e
#define VDEC2_VLD_TIME_STAMP_3           0x2c4f
#define VDEC2_VLD_TIME_STAMP_LENGTH      0x2c50
#define VDEC2_VLD_MEM_VIFIFO_WRAP_COUNT  0x2c51
#define VDEC2_VLD_MEM_VIFIFO_MEM_CTL     0x2c52
#define VDEC2_VLD_MEM_VBUF_RD_PTR        0x2c53
#define VDEC2_VLD_MEM_VBUF2_RD_PTR       0x2c54
#define VDEC2_VLD_MEM_SWAP_ADDR          0x2c55
#define VDEC2_VLD_MEM_SWAP_CTL           0x2c56

#define VDEC2_VCOP_CTRL_REG              0x2e00
#define VDEC2_QP_CTRL_REG                0x2e01
#define VDEC2_INTRA_QUANT_MATRIX         0x2e02
#define VDEC2_NON_I_QUANT_MATRIX         0x2e03
#define VDEC2_DC_SCALER                  0x2e04
#define VDEC2_DC_AC_CTRL                 0x2e05
#define VDEC2_DC_AC_SCALE_MUL            0x2e06
#define VDEC2_DC_AC_SCALE_DIV            0x2e07
#define VDEC2_POWER_CTL_IQIDCT           0x2e08
#define VDEC2_RV_AI_Y_X                  0x2e09
#define VDEC2_RV_AI_U_X                  0x2e0a
#define VDEC2_RV_AI_V_X                  0x2e0b
#define VDEC2_RV_AI_MB_COUNT             0x2e0c
#define VDEC2_NEXT_INTRA_DMA_ADDRESS     0x2e0d
#define VDEC2_IQIDCT_CONTROL             0x2e0e
#define VDEC2_IQIDCT_DEBUG_INFO_0        0x2e0f
#define VDEC2_DEBLK_CMD                  0x2e10
#define VDEC2_IQIDCT_DEBUG_IDCT          0x2e11
#define VDEC2_DCAC_DMA_CTRL              0x2e12
#define VDEC2_DCAC_DMA_ADDRESS           0x2e13
#define VDEC2_DCAC_CPU_ADDRESS           0x2e14
#define VDEC2_DCAC_CPU_DATA              0x2e15
#define VDEC2_DCAC_MB_COUNT              0x2e16
#define VDEC2_IQ_QUANT                   0x2e17
#define VDEC2_VC1_BITPLANE_CTL           0x2e18
#endif
