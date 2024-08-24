### Table III.21.2 EE Clock
 
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


### Table III.21.4 EE Domain Clock Gating (clk81)

| Address     | Bit(s) | Module Description                                                                 |
|-------------|--------|-------------------------------------------------------------------------------------|
| 0xc883c140  | 31     | Reserved                                                                            |
|             | 30     | SPI Interface                                                                       |
|             | 29     | Reserved                                                                            |
|             | 28     | acodec                                                                              |
|             | 27     | dma                                                                                 |
|             | 26     | emmc_c                                                                              |
|             | 25     | emmc_b                                                                              |
|             | 24     | emmc_a                                                                              |
|             | 23     | ASSIST_MISC                                                                         |
|             | 22     | bt656                                                                               |
|             | 21     | Reserved                                                                            |
|             | 20     | Reserved                                                                            |
|             | 19     | HIU Registers                                                                       |
|             | 18     | Reserved                                                                            |
|             | 17     | Reserved                                                                            |
|             | 16     | ASYNC_FIFO                                                                          |
|             | 15     | STREAM Interface                                                                    |
|             | 14     | Reserved                                                                            |
|             | 13     | UART0                                                                               |
|             | 12     | Random Number generator                                                             |
|             | 11     | Smart Card                                                                          |
|             | 10     | SAR ADC                                                                             |
|             | 9      | I2C Master / I2C SLAVE                                                              |
|             | 8      | SPICC                                                                               |
|             | 7      | PERIPHS module top level (there are separate register bits for internal blocks)     |
|             | 6      | PL310 (AXI Matrix) to CBUS                                                          |
|             | 5      | ISA module                                                                          |
|             | 4      | Reserved                                                                            |
|             | 3      | Reserved                                                                            |
|             | 2      | Reserved                                                                            |
|             | 1      | u_dos_top()                                                                         |
|             | 0      | DDR Interfaces and bridges                                                          |
| 0xc883c144  | 31     | ROM BOOT ROM clock                                                                  |
|             | 30     | EFUSE logic                                                                         |
|             | 29     | AHB ARB0                                                                            |
|             | 28     | Reserved                                                                            |
|             | 27     | Reserved                                                                            |
|             | 26     | USB General                                                                         |
|             | 25     | U_parser_top()                                                                      |
|             | 24     | Reserved                                                                            |
|             | 23     | RESET                                                                               |
|             | 22     | USB 1                                                                               |
|             | 21     | USB 0                                                                               |
|             | 20     | General 2D Graphics Engine                                                          |
|             | 19     | Reserved                                                                            |
|             | 18     | Reserved                                                                            |
|             | 17     | Reserved                                                                            |
|             | 16     | UART1                                                                               |
|             | 15     | AIU Top level (there is internal gating shown below)                                |
|             | 14     | Block move core logic                                                               |
|             | 13     | ADC                                                                                 |
|             | 12     | Mixer Registers: u_ai_top.u_mixer_reg.clk                                           |
|             | 11     | Mixer: u_ai_top.u_aud_mixer.clk                                                     |
|             | 10     | AIFIFO2: u_ai_top.u_aififo2.clk                                                     |
|             | 9      | AMCLK measurement circuit                                                           |
|             | 8      | I2S Out: This bit controls the clock to the logic between the DRAM control unit and the FIFO’s that transfer data to the audio clock domain. (u_ai_top.i2s_fast.clk) |
|             | 7      | IEC958: iec958_fast()                                                               |
|             | 6      | AIU – ai_top_glue u_ai_top.ai_top_glue.clk u_ai_top.fifo_async_fast_i2s.clk u_ai_top.fifo_async_fast_958.clk |
|             | 5      | Reserved                                                                            |
|             | 4      | Set top box demux module u_stb_top.clk                                              |
|             | 3      | Ethernet core logic                                                                 |
|             | 2      | I2S / SPDIF Input                                                                   |
|             | 1      | Reserved                                                                            |
|             | 0      | Reserved                                                                            |
| 0xc883c148  | 31     | Reserved                                                                            |
|             | 30     | gic                                                                                 |
|             | 29     | Reserved                                                                            |
|             | 28     | Reserved                                                                            |
|             | 27     | Reserved                                                                            |
|             | 26     | Secure AHB to APB3 Bridge                                                           |
|             | 25     | VPU Interrupt                                                                       |
|             | 24     | Reserved                                                                            |
|             | 23     | Reserved                                                                            |
|             | 22     | sar_adc                                                                             |
|             | 21     | UART3                                                                               |
|             | 20     | Reserved                                                                            |
|             | 19     | Reserved                                                                            |
|             | 18     | Reserved                                                                            |
|             | 17     | Reserved                                                                            |
|             | 16     | Reserved                                                                            |
|             | 15     | UART 2                                                                              |
|             | 14     | Reserved                                                                            |
|             | 13     | Reserved                                                                            |
|             | 12     | DVIN                                                                                 |
|             | 11     | MMC PCLK                                                                            |
|             | 10     | AIU PCLK                                                                            |
|             | 9      | USB0 to DDR bridge                                                                  |
|             | 8      | USB1 to DDR bridge                                                                  |
|             | 7      | bt656_2                                                                             |
|             | 6      | bt656                                                                               |
|             | 5      | pdm                                                                                 |
|             | 4      | HDMI PCLK                                                                           |
|             | 3      | HDMI interrupt synchronization                                                      |
|             | 2      | AHB control bus                                                                     |
|             | 1      | AHB data bus                                                                        |
|             | 0      | Reserved                                                                            |
| 0xc883c150  | 31     | Reserved                                                                            |
|             | 30     | Reserved                                                                            |
|             | 29     | Reserved                                                                            |
|             | 28     | Reserved                                                                            |
|             | 27     | Reserved                                                                            |
|             | 26     | VCLK2_OTHER                                                                         |
|             | 25     | VCLK2_VENCL                                                                         |
|             | 24     | VCLK2_VENCL MMC Clock All                                                           |
|             | 23     | VCLK2_ENCL                                                                          |
|             | 22     | VCLK2_ENCT                                                                          |
|             | 21     | Random Number Generator                                                            |
|             | 20     | ENC480P                                                                             |
|             | 19     | Reserved                                                                            |
|             | 18     | Reserved                                                                            |
|             | 17     | Reserved                                                                            |
|             | 16     | IEC958_GATE                                                                         |
|             | 15     | Reserved                                                                            |
|             | 14     | AOCLK_GATE                                                                          |
|             | 13     | Reserved                                                                            |
|             | 12     | Reserved                                                                            |
|             | 11     | Reserved                                                                            |
|             | 10     | DAC_CLK                                                                             |
|             | 9      | VCLK2_ENCP                                                                          |
|             | 8      | VCLK2_ENCI                                                                          |
|             | 7      | VCLK2_OTHER                                                                         |
|             | 6      | VCLK2_VENCT                                                                         |
|             | 5      | VCLK2_VENCT                                                                         |
|             | 4      | VCLK2_VENCP                                                                         |
|             | 3      | VCLK2_VENCP                                                                         |
|             | 2      | VCLK2_VENCI                                                                         |
|             | 1      | VCLK2_VENCI                                                                         |
|             | 0      | Reserved                                                                            |




### RESET_REGISTER 0xc11004404
| Bit(s) | R/W | Default | Description       |
|--------|-----|---------|-------------------|
| 31-28  | R/W | 0       | MIPI              |
| 27     | R/W | 0       | mmc               |
| 26     | R/W | 0       | Vcbus_clk81       |
| 25     | R/W | 0       | Ahb_data          |
| 24     | R/W | 0       | Ahb_cntl          |
| 23     | R/W | 0       | Cbus_capb3        |
| 22     | R/W | 0       | Sys_cpu_capb3     |
| 21     | R/W | 0       | Dos_capb3         |
| 20     | R/W | 0       | Mali_capb3        |
| 19     | R/W | 0       | Hdmitx_capb3      |
| 18     | R/W | 0       | Nand_capb3        |
| 17     | R/W | 0       | Capb3_decode      |
| 16     | R/W | 0       | Gic               |
| 15     | R/W | 0       | Reserved          |
| 14     | R/W | 0       | Reserved          |
| 13     | R/W | 0       | vcbus             |
| 12     | R/W | 0       | AFIFO2            |
| 11     | R/W | 0       | ASSIST            |
| 10     | R/W | 0       | VENC              |
| 9      | R/W | 0       | PMUX              |
| 8      | R/W | 0       | Reserved          |
| 7      | R/W | 0       | Vid_pll_div       |
| 6      | R/W | 0       | AIU               |
| 5      | R/W | 0       | VIU               |
| 4      | R/W | 0       | DCU_RESET         |
| 3      | R/W | 0       | DDR_TOP           |
| 2      | R/W | 0       | DOS_RESET         |
| 1      | R/W | 0       | Reserved          |
| 0      | R/W | 0       | HIU               |

### RESET1_REGISTER 0xc11004408
| Bit(s) | R/W | Default | Description       |
|--------|-----|---------|-------------------|
| 31-29  | R/W | 0       | Reserved          |
| 28     | R/W | 0       | Sys_cpu_mbist     |
| 27     | R/W | 0       | Sys_cpu_p         |
| 26     | R/W | 0       | Sys_cpu_l2        |
| 25     | R/W | 0       | Sys_cpu_axi       |
| 24     | R/W | 0       | Sys_pll_div       |
| 23-20  | R/W | 0       | Sys_cpu_core[3:0] |
| 19-16  | R/W | 0       | Sys_cpu[3:0]      |
| 15     | R/W | 0       | Rom_boot          |
| 14     | R/W | 0       | Sd_emmc_c         |
| 13     | R/W | 0       | Sd_emmc_b         |
| 12     | R/W | 0       | Sd_emmc_a         |
| 11     | R/W | 0       | Ethernet          |
| 10     | R/W | 0       | ISA               |
| 9      | R/W | 0       | BLKMV (NDMA)      |
| 8      | R/W | 0       | PARSER            |
| 7      | R/W | 0       | Reserved          |
| 6      | R/W | 0       | AHB_SRAM          |
| 5      | R/W | 0       | BT656             |
| 4      | R/W | 0       | AO Reset          |
| 3      | R/W | 0       | DDR               |
| 2      | R/W | 0       | USB_OTG           |
| 1      | R/W | 0       | DEMUX             |
| 0      | R/W | 0       | Cppm              |

### RESET2_REGISTER 0xc1100440c
| Bit(s) | R/W | Default | Description       |
|--------|-----|---------|-------------------|
| 15     | R/W | 0       | Hdmi system reset |
| 14     | R/W | 0       | MALI              |
| 13     | R/W | 0       | AO CPU RESET      |
| 12     | R/W | 0       | Reserved          |
| 11     | R/W | 0       | Reserved          |
| 10     | R/W | 0       | parser_top        |
| 9      | R/W | 0       | parser_ctl        |
| 8      | R/W | 0       | Paser_fetch       |
| 7      | R/W | 0       | Parser_reg        |
| 6      | R/W | 0       | GE2D              |
| 5      | R/W | 0       | Reserved          |
| 4      | R/W | 0       | Reserved          |
| 3      | R/W | 0       | Reserved          |
| 2      | R/W | 0       | HDMI_TX           |
| 1      | R/W | 0       | AUDIN             |
| 0      | R/W | 0       | VD_RMEM           |


### RESET3_REGISTER 0xc11004410
| Bit(s) | R/W | Default | Description       |
|--------|-----|---------|-------------------|
| 15     | R/W | 0       | Demux reset 2     |
| 14     | R/W | 0       | Demux reset 1     |
| 13     | R/W | 0       | Demux reset 0     |
| 12     | R/W | 0       | Demux S2P 1       |
| 11     | R/W | 0       | Demux S2p 0       |
| 10     | R/W | 0       | Demux DES         |
| 9      | R/W | 0       | Demux top         |
| 8      | R/W | 0       | Audio DAC         |
| 7      | R/W | 0       | Reserved          |
| 6      | R/W | 0       | AHB BRIDGE CNTL   |
| 5      | R/W | 0       | tvfe              |
| 4      | R/W | 0       | AIFIFO            |
| 3      | R/W | 0       | Sys_cpu_bvci      |
| 2      | R/W | 0       | EFUSE             |
| 1      | R/W | 0       | SYS CPU           |
| 0      | R/W | 0       | Ring oscillator   |


### RESET4_REGISTER 0xc11004414
| Bit(s) | R/W | Default | Description       |
|--------|-----|---------|-------------------|
| 15     | R/W | 0       | I2C_Master 1      |
| 14     | R/W | 0       | I2C_Master 2      |
| 13     | R/W | 0       | VENCL             |
| 12     | R/W | 0       | VDI6              |
| 11     | R/W | 0       | Reserved          |
| 10     | R/W | 0       | RTC               |
| 9      | R/W | 0       | VDAC              |
| 8      | R/W | 0       | Reserved          |
| 7      | R/W | 0       | VENCP             |
| 6      | R/W | 0       | VENCI             |
| 5      | R/W | 0       | RDMA              |
| 4      | R/W | 0       | DVIN_RESET        |
| 3      | R/W | 0       | Reserved          |
| 2      | R/W | 0       | Reserved          |
| 1      | R/W | 0       | Reserved          |
| 0      | R/W | 0       | Reserved          |
| 1      | R/W | 0       | MISC PLL          |
| 0      | R/W | 0       | DDR PLL           |


### RESET6_REGISTER 0xc1100441c
| Bit(s) | R/W | Default | Description               |
|--------|-----|---------|---------------------------|
| 15     | R/W | 0       | Uart_slip                 |
| 14     | R/W | 0       | PERIPHS: SDHC             |
| 13     | R/W | 0       | PERIPHS: SPI 0            |
| 12     | R/W | 0       | PERIPHS: Async 1          |
| 11     | R/W | 0       | PERIPHS: Async 0          |
| 10     | R/W | 0       | PERIPHS: UART 1, 2        |
| 9      | R/W | 0       | PERIPHS: UART 0           |
| 8      | R/W | 0       | PERIPHS: SDIO             |
| 7      | R/W | 0       | PERIPHS: Stream Interface |
| 6      | R/W | 0       |                           |
| 5      | R/W | 0       | SANA                      |
| 4      | R/W | 0       | PERIPHS: I2C Master 0     |
| 3      | R/W | 0       | PERIPHS: SAR ADC          |
| 2      | R/W | 0       | PERIPHS: Smart Card       |
| 1      | R/W | 0       | PERIPHS: SPICC            |
| 0      | R/W | 0       | PERIPHS: General          |



### RESET7_REGISTER 0xc11004420
| Bit(s) | R/W | Default | Description       |
|--------|-----|---------|-------------------|
| 15     | R/W | 0       | Reserved          |
| 14     | R/W | 0       | Reserved          |
| 13     | R/W | 0       | Reserved          |
| 12     | R/W | 0       | Reserved          |
| 11     | R/W | 0       | Reserved          |
| 10     | R/W | 0       | Reserved          |
| 9      | R/W | 0       | Reserved          |
| 8      | R/W | 0       | A9_dmc_pipel      |
| 7      | R/W | 0       | vid_lock          |
| 6      | R/W | 0       | Reserved          |
| 5      | R/W | 0       | Device_mmc_arb    |
| 4      | R/W | 0       | Reserved          |
| 3~0    | R/W | 0       | Usb_ddr[3:0]      |


### Table III.23.1 EE Interrupt Source

| A53 GIC Bit | Interrupt Sources       | Description            |
|-------------|-------------------------|------------------------|
| 255         | 1'b0                    | unused                 |
| 254         | 1'b0                    | unused                 |
| 253         | 1'b0                    | unused                 |
| 252         | 1'b0                    | unused                 |
| 251         | 1'b0                    | unused                 |
| 250         | sd_emmc_C_irq           |                        |
| 249         | sd_emmc_B_irq           |                        |
| 248         | sd_emmc_A_irq           |                        |
| 247         | m_i2c_2_irq             |                        |
| 246         | m_i2c_1_irq             |                        |
| 245         | mbox_irq_send5          |                        |
| 244         | mbox_irq_send4          |                        |
| 243         | mbox_irq_send3          |                        |
| 242         | mbox_irq_receiv2        |                        |
| 241         | mbox_irq_receiv1        |                        |
| 240         | mbox_irq_receiv0        |                        |
| 239         | 1'b0                    | unused                 |
| 238         | ao_timerA_irq           |                        |
| 237         | 1'b0                    | unused                 |
| 236         | ao_watchdog_irq         |                        |
| 235         | ao_jtag_pwd_fast_irq    |                        |
| 234         | 1'b0                    | unused                 |
| 233         | ao_gpio_irq1            |                        |
| 232         | ao_gpio_irq0            |                        |
| 231         | ao_cec_irq              |                        |
| 230         | ao_ir_blaster_irq       |                        |
| 229         | ao_uart2_irq            |                        |
| 228         | ao_ir_dec_irq           |                        |
| 227         | ao_i2c_m_irq            |                        |
| 226         | ao_i2c_s_irq            |                        |
| 225         | ao_uart_irq             |                        |
| 224         | 1'b0                    | unused                 |
| 223         | dma_irq[3]              |                        |
| 222         | dma_irq[2]              |                        |
| 221         | dma_irq[1]              |                        |
| 220         | dma_irq[0]              |                        |
| 219         | 1'b0                    | unused                 |
| 218         | vp9dec_irq              | unused                 |
| 217         | 1'b0                    | unused                 |
| 216         | 1'b0                    | unused                 |
| 215         | 1'b0                    | unused                 |
| 214         | 1'b0                    | unused                 |
| 213         | 1'b0                    | unused                 |
| 212         | 1'b0                    | unused                 |
| 201         | mali_irq_ppmmu2         |                        |
| 200         | mali_irq_pp2            |                        |
| 199         | mali_irq_ppmmu1         |                        |
| 198         | mali_irq_pp1            |                        |
| 197         | mali_irq_ppmmu0         |                        |
| 196         | mali_irq_pp0            |                        |
| 195         | mali_irq_pmu            |                        |
| 194         | mali_irq_pp             |                        |
| 193         | mali_irq_gpmmu          |                        |
| 192         | mali_irq_gp             |                        |
| 184         | viu1_line_n_irq         |                        |
| 183         | 1'b0                    |                        |
| 182         | ge2d_irq                |                        |
| 181         | cusad_irq               |                        |
| 180         | asssit_mbox_irq3        |                        |
| 179         | asssit_mbox_irq2        |                        |
| 178         | asssit_mbox_irq1        |                        |
| 177         | asssit_mbox_irq0        |                        |
| 176         | det3d_int               |                        |
| 175         | viu1_wm_int             |                        |
| 173         | A53irq[4]               | EXTERRIRQ_a            |
| 172         | A53irq[3]               | CTIIRQ[3:0]            |
| 171         | A53irq[2]               | VCPUMNTIRQ_a[3:0]      |
| 170         | A53irq[1]               | COMMIRQ_a[3:0]         |
| 169         | A53irq[0]               | PMUIRQ_a[3:0]          |
| 168         | 1'b0                    |                        |
| 167         | 1'b0                    |                        |
| 166         | 1'b0                    |                        |
| 165         | 1'b0                    |                        |
| 164         | 1'b0                    |                        |
| 127         | 1'b0                    | unused                 |
| 126         | uart3_slip_irq          | UART slip              |
| 125         | uart2_irq               | UART 2                 |
| 124         | 1'b0                    | unused                 |
| 123         | 1'b0                    | unused                 |
| 122         | 1'b0                    | unused                 |
| 121         | rdma_done_int           | RDMA                   |
| 120         | i2s_cbus_ddr_irq        | Audio I2S CBUS IRQ     |
| 119         | 1'b0                    | unused                 |
| 118         | vid1_wr_irq             |                        |
| 117         | vdin1_vsync_int         |                        |
| 116         | vdin1_hsync_int         |                        |
| 115         | vdin0_vsync_int         |                        |
| 114         | vdin0_hsync_int         |                        |
| 113         | spi2_int                |                        |
| 112         | spi_int                 |                        |
| 111         | vid0_wr_irq             |                        |
| 110         | 1'b0                    | unused                 |
| 109         | 1'b0                    | unused                 |
| 108         | 1'b0                    | unused                 |
| 107         | uart1_irq               |                        |
| 106         | 1'b0                    | unused                 |
| 105         | sar_adc_irq             | SAR ADC                |
| 104         | 1'b0                    | unused                 |
| 103         | gpio_irq[7]             | GPIO Interrupt         |
| 102         | gpio_irq[6]             | GPIO Interrupt         |
| 101         | gpio_irq[5]             | GPIO Interrupt         |
| 100         | gpio_irq[4]             | GPIO Interrupt         |
| 99          | gpio_irq[3]             | GPIO Interrupt         |
| 98          | gpio_irq[2]             | GPIO Interrupt         |
| 97          | gpio_irq[1]             | GPIO Interrupt         |
| 96          | gpio_irq[0]             | GPIO Interrupt         |
| 95          | TimerI                  | TimerI                 |
| 94          | TimerH                  | TimerH                 |
| 93          | TimerG                  | TimerG                 |
| 92          | TimerF                  | TimerF                 |
| 91          | 1'b0                    | unused                 |
| 90          | hdcp22_irq              |                        |
| 89          | hdmi_tx_interrupt       |                        |
| 88          | 1'b0                    | unused                 |
| 87          | hdmi_cec_interrupt      |                        |
| 86          | dmc_test_irq            |                        |
| 85          | demux_int_2             |                        |
| 84          | dmc_irq                 |                        |
| 83          | dmc_sec_irq             |                        |
| 82          | ai_iec958_int           | IEC958 interrupt       |
| 81          | iec958_ddr_irq          | IEC958 DDR interrupt   |
| 80          | i2s_irq                 | I2S DDR Interrupt      |
| 79          | crc_done                | From AIU CRC done      |
| 78          | deint_irq               | Reserved for Deinterlacer |
| 77          | dos_mbox_slow_irq[2]    | DOS Mailbox 2          |
| 76          | dos_mbox_slow_irq[1]    | DOS Mailbox 1          |
| 75          | dos_mbox_slow_irq[0]    | DOS Mailbox 0          |
| 74          | 1'b0                    | unused                 |
| 73          | 1'b0                    | unused                 |
| 72          | 1'b0                    | unused                 |
| 71          | m_i2c_3_irq             | I2C Master #3          |
| 70          | 1'b0                    | unused                 |
| 69          | smartcard_irq           |                        |
| 67          | spdif_irq               |                        |
| 66          | nand_irq                |                        |
| 65          | viff_empty_int_cpu      |                        |
| 64          | parser_int_cpu          |                        |
| 63          | U2d_interrupt           | USB                    |
| 62          | U3h_interrupt           | USB                    |
| 61          | Timer D                 | Timer D                |
| 60          | bus_mon1_fast_irq       |                        |
| 59          | bus_mon0_fast_irq       |                        |
| 58          | uart0_irq               |                        |
| 57          | async_fifo2_flush_irq   |                        |
| 56          | async_fifo2_fill_irq    |                        |
| 55          | demux_int               |                        |
| 54          | encif_irq               |                        |
| 53          | m_i2c_0_irq             |                        |
| 52          | bt656_irq               |                        |
| 51          | async_fifo_flush_irq    |                        |
| 50          | async_fifo_fill_irq     |                        |
| 49          | bt656_2_rq              | bt656_B                |
| 48          | usb_iddig_irq           |                        |
| 47          | 1'b0                    | unused                 |
| 46          | eth_lip_intro_o         |                        |
| 45          | 1'b0                    | unused                 |
| 44          | 1'b0                    | unused                 |
| 43          | Timer B                 | Timer B                |
| 42          | Timer A                 | Timer A                |
| 41          | eth_phy_irq             | unused                 |
| 40          | eth_gmac_int            |                        |
| 39          | audin_irq               |                        |
| 38          | Timer C                 | Timer C                |
| 37          | demux_int_1             |                        |
| 36          | eth_pmt_intr_o          |                        |
| 35          | viu1_vsync_int          | VSYNC                  |
| 34          | viu1_hsync_int          | HSYNC                  |
| 33          | 1'b0                    | HIU Mailbox            |
| 32          | ee_wd_irq               | Watchdog Timer         |


