 | Conditionals | name | destination | gate | N | sel |
|--------------|------|-------------|------|---|-----|
| | cts_slow_oscin_clk | a53 | | | 0x5d[9] |
| | amclk_o_int | aiu | | 0x1516[11] | |
| if 0x1519[5:0]=0, div = 0x1516[3:2] | cts_aoclk_int | aiu | 0x1516[0] | 0x1519[5:1] | |
| if 0x1519[5:0]=0, div = 0x1516[3:2] | cts_moclk32_int | aiu | 0x1516[0] | 0x1519[5:1] | |
| 64[27] = 0, will use the same as cts au | cts_mclk_1958 | aiu | 0x64[24] | 0x64[23:16] | 0x64[26:25] |
| if 0x1516[12] 0, will div 1/2/3/4;<br>if 0x1516[12] 1, will div 2/4/6/8; | cts_clk_1958 | aiu | 0x1516[1] | 0x1516[5:4] | |
| | cts_pcm_mclk | aiu | 0x96[9] | 0x96[8:0] | 0x96[11:10] |
| | cts_amclk | aiu/audin | 0x5e[8] | 0x5e[7:0] | 0x5e[10:9] |
| | cts_pcm_sclk | aiu/audin | 0x96[22] | 0x96[21:16] | |
| div = 0x5d[31]?0x5d[30:16]:0x5d[6:0] | mpeg_pll_clk | aiu | 0x5d[7] | 0x5d[31:0] | 0x5d[14:12] |
| | clk81_ee_o | ao | | | 0x5d[8] |
| | oscin_clk_to_ao | ao | | | |
| | cts_msr_clk | audin | 0x69[16] | 0x69[15:0] | 0x69[18:17] |
| | cts_bt656_clk0 | bt656 | 0xf5[7] | 0xf5[6:0] | 0xf5[10:9] |
| if 0x7a[31] = 0, use 0x78;<br>if 0x7a[31] = 1, use 0x7a;<br>if 0x7c[31] = 0, use 0x79;<br>if 0x7c[31] = 1, use 0x7c;<br>if 0x7a[15] = 0, use 0x78;<br>if 0x7a[15] = 1, use 0x7a; | cts_hcodec_clk | dos | 0x78[24] | 0x78[22:16] | 0x78[27:25] |
| | cts_hevc_clk | dos | 0x79[24] | 0x79[22:16] | 0x79[27:25] |
| | cts_vdec_clk | dos | 0x7c[24] | 0x7c[22:16] | 0x7c[27:25] |
| | cts_ge2d_clk | ge2d/dmc | 0x7d[30] | | |
| 0x8a[15] = 0 , use up 8 src;<br>0x8a[15] = 1 , use low 8 src;<br>all fixed div32 | gen_clk_out | gpio | 0x8a[11] | 0x8a[10:0] | 0x8a[15:12] |
| | tst_clk_out[10:0] | gpio | | | /32 |
| if 0x6c[31] = 0, use 0x6c[11:0];<br>if 0x6c[31] = 1, use 0x6c[27:16] | cts_mali_clk | mali | 0x6c[8] | 0x6c[6:0] | 0x6c[11:9] |
| | cts_nand_core_clk | nand | 0x97[7] | 0x97[6:0] | 0x97[11:9] |
| | cts_msr_clk | periphs | 0x21d7[19] | 0x21d8[7:0] | 0x21d7[26:20] |
| | cts_mst_clk_hs | periphs | 0x21d8[28] | | |
| | cts_pwm_A_clk | periphs | 0x2156[15] | 0x2156[14:8] | 0x2156[5:4] |
| | cts_pwm_B_clk | periphs | 0x2156[23] | 0x2156[22:16] | 0x2156[7:6] |
| | cts_pwm_C_clk | periphs | 0x2192[15] | 0x2192[14:8] | 0x2192[5:4] |
| | cts_pwm_D_clk | periphs | 0x2192[23] | 0x2192[22:16] | 0x2192[7:6] |
| | cts_pwm_E_clk | periphs | 0x21b2[15] | 0x21b2[14:8] | 0x21b2[5:4] |
| | cts_pwm_F_clk | periphs | 0x21b2[23] | 0x21b2[22:16] | 0x21b2[7:6] |
| | cts_sar_adc_clk | periphs | 0xf6[8] | 0xf6[7:0] | 0xf6[10:9] |
| | sc_clk | periphs | 0x2110[24] | 0x2112[19:12] | 0x2112[31:28] |
| | cts_sd_emmc_clk_A | sd_emmc_A | 0x99[7] | 0x99[6:0] | 0x99[11:9] |
| | cts_sd_emmc_clk_B | sd_emmc_B | 0x99[23] | 0x99[22:16] | 0x99[27:25] |
| | clk81 | all module | | | 0x5d[8] |
| | usb_32k_alt | usb | 0x88[9] | 0x88[8:0] | 0x88[11:10] |
| if 0x7d[31] = 0, use 0x7d[11:0];<br>if 0x7d[31] = 1, use 0x7d[27:16]; | cts_vapbclk | vapb | 0x7d[8] | 0x7d[6:0] | 0x7d[11:9] |
| | alt_32k_clk | vpu | 0x89[15] | | 0x89[17:16] |
| | cts_enc1_clk | vpu | | | |
| | cts_enc1_clk | vpu | | | |
| | cts_encp_clk | vpu | | | |
| see video | cts_hdmi_tx_pixel_clk | vpu | | | see video |
| | cts_vdac_clk | vpu | | | |
| | lcd_an_clk_ph2 | vpu | | | |
| | lcd_an_clk_ph3 | vpu | | | |
| | cts_hdcp22_skpclk | vpu | 0x7c[24] | 0x7c[22:16] | 0x7c[26:25] |
| | cts_vdin_meas_clk | vpu | 0x94[8] | 0x94[6:0] | 0x94[11:9] |
| | cts_vid_lock_clk | vpu | 0xf2[7] | 0xf2[6:0] | 0xf2[9:8] |
| | cts_vpu_clk | vpu | 0x6f[8] | 0x6f[6:0] | 0x6f[11:9] |
| | cts_hdmitx_sys_clk | vpu | 0x73[8] | 0x73[6:0] | 0x73[10:9] |
| | cts_hdcp22_esmclk | vpu/dmc | 0x7c[8] | 0x7c[6:0] | 0x7c[10:9] |
| | cts_oscin_clk | watchdog/periphs | | | |
