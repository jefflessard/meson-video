# DOS (Decoder/Encoder Subsystem)

## Memory Map
| Start       | End         | Region | CCI/NIC Arbitor | Description                       |
|-------------|-------------|--------|-----------------|-----------------------------------|
| 0xC8820000  | 0xC882FFFF  | DOS    |                 | Memory region for DOS operations  |
| 0xDA820000  | 0xDA82FFFF  | DOS    |                 | Additional memory region for DOS  |

## Clock Tree
| Name          | Destination | Gate     | N           | Sel         | Description                              |
|---------------|-------------|----------|-------------|-------------|------------------------------------------|
| cts_hcodec_clk| dos         | 0x78[24] | 0x78[22:16] | 0x78[27:25] | Clock for high-definition codec          |
| cts_hevc_clk  | dos         | 0x79[24] | 0x79[22:16] | 0x79[27:25] | Clock for HEVC codec                     |
| cts_vdec_clk  | dos         | 0x7c[24] | 0x7c[22:16] | 0x7c[27:25] | Clock for video decoder                  |

## Clock Gating
| Address      | Bit(s) | Module Description | Description                       |
|--------------|--------|---------------------|-----------------------------------|
| 0xc883c140   | 1      | u_dos_top()         | Clock gating for DOS top module   |

## Reset Registers
| Address      | Bit(s) | R/W | Default | Description    | Description                       |
|--------------|--------|-----|---------|----------------|-----------------------------------|
| 0xc11004404  | 2      | R/W | 0       | DOS_RESET      | Reset control for DOS             |

## Interrupt Sources
| A53 GIC Bit  | Interrupt Sources       | Description                       |
|--------------|-------------------------|-----------------------------------|
| 77           | dos_mbox_slow_irq[2]    | DOS Mailbox 2 interrupt           |
| 76           | dos_mbox_slow_irq[1]    | DOS Mailbox 1 interrupt           |
| 75           | dos_mbox_slow_irq[0]    | DOS Mailbox 0 interrupt           |

# PARSER

## Memory Map
| Start       | End         | Region  | CCI/NIC Arbitor | Description                       |
|-------------|-------------|---------|-----------------|-----------------------------------|
| 0xC080A400  | 0xC080A4FF  | PARSER  |                 | Memory region for PARSER          |

## Clock Gating
| Address      | Bit(s) | Module Description | Description                       |
|--------------|--------|---------------------|-----------------------------------|
| 0xc883c144   | 25     | U_parser_top()      | Clock gating for PARSER top module|

## Reset Registers
| Address      | Bit(s) | R/W | Default | Description    | Description                       |
|--------------|--------|-----|---------|----------------|-----------------------------------|
| 0xc1100440c  | 10     | R/W | 0       | parser_top     | Reset control for PARSER top      |
| 0xc1100440c  | 9      | R/W | 0       | parser_ctl     | Reset control for PARSER control  |
| 0xc1100440c  | 8      | R/W | 0       | Paser_fetch    | Reset control for PARSER fetch    |
| 0xc1100440c  | 7      | R/W | 0       | Parser_reg     | Reset control for PARSER register |

## Interrupt Sources
| A53 GIC Bit  | Interrupt Sources       | Description                       |
|--------------|-------------------------|-----------------------------------|
| 64           | parser_int_cpu          | PARSER interrupt                  |

# VPU (Video Processing Unit)

## Memory Map
| Start       | End         | Region | CCI/NIC Arbitor | Description                       |
|-------------|-------------|--------|-----------------|-----------------------------------|
| 0xD0100000  | 0xD013FFFF  | VPU    | VAPB3           | Memory region for VPU             |

## Clock Tree
| Name                  | Destination | Gate     | N           | Sel         | Description                              |
|-----------------------|-------------|----------|-------------|-------------|------------------------------------------|
| alt_32k_clk           | vpu         | 0x89[15] |             | 0x89[17:16] | Alternative 32k clock for VPU            |
| cts_enc1_clk          | vpu         |          |             |             | Clock for encoder 1                      |
| cts_encp_clk          | vpu         |          |             |             | Clock for encoder P                      |
| cts_hdmi_tx_pixel_clk | vpu         |          |             |             | HDMI TX pixel clock                      |
| cts_vdac_clk          | vpu         |          |             |             | VDAC clock                               |
| lcd_an_clk_ph2        | vpu         |          |             |             | LCD clock phase 2                        |
| lcd_an_clk_ph3        | vpu         |          |             |             | LCD clock phase 3                        |
| cts_hdcp22_skpclk     | vpu         | 0x7c[24] | 0x7c[22:16] | 0x7c[26:25] | HDCP 2.2 skip clock                      |
| cts_vdin_meas_clk     | vpu         | 0x94[8]  | 0x94[6:0]   | 0x94[11:9]  | VDIN measurement clock                   |
| cts_vid_lock_clk      | vpu         | 0xf2[7]  | 0xf2[6:0]   | 0xf2[9:8]   | Video lock clock                         |
| cts_vpu_clk           | vpu         | 0x6f[8]  | 0x6f[6:0]   | 0x6f[11:9]  | VPU clock                                |
| cts_hdmitx_sys_clk    | vpu         | 0x73[8]  | 0x73[6:0]   | 0x73[10:9]  | HDMI TX system clock                     |
| cts_hdcp22_esmclk     | vpu/dmc     | 0x7c[8]  | 0x7c[6:0]   | 0x7c[10:9]  | HDCP 2.2 ESM clock                       |

## Clock Gating
| Address      | Bit(s) | Module Description | Description                       |
|--------------|--------|---------------------|-----------------------------------|
| 0xc883c148   | 25     | VPU Interrupt       | Clock gating for VPU interrupt    |

## Reset Registers
| Address      | Bit(s) | R/W | Default | Description    | Description                       |
|--------------|--------|-----|---------|----------------|-----------------------------------|
| 0xc11004404  | 10     | R/W | 0       | VENC           | Reset control for VENC            |
| 0xc11004414  | 13     | R/W | 0       | VENCL          | Reset control for VENCL           |
| 0xc11004414  | 7      | R/W | 0       | VENCP          | Reset control for VENCP           |
| 0xc11004414  | 6      | R/W | 0       | VENCI          | Reset control for VENCI           |

## Interrupt Sources
| A53 GIC Bit  | Interrupt Sources       | Description                       |
|--------------|-------------------------|-----------------------------------|
| 218          | vp9dec_irq              | VP9 decoder interrupt (unused)    |
| 118          | vid1_wr_irq             | Video 1 write interrupt           |
| 111          | vid0_wr_irq             | Video 0 write interrupt           |
| 7            | vid_lock                | Video lock interrupt              |
| 35           | viu1_vsync_int          | VSYNC interrupt                   |
| 34           | viu1_hsync_int          | HSYNC interrupt                   |

# Other Components Involved in Video Operations

## VDIN (Video Input)
### Memory Map
| Start       | End         | Region | CCI/NIC Arbitor | Description                       |
|-------------|-------------|--------|-----------------|-----------------------------------|
| 0xC0804800  | 0xC08048FF  | VDIN   |                 | Memory region for VDIN            |

### Clock Gating
| Address      | Bit(s) | Module Description | Description                       |
|--------------|--------|---------------------|-----------------------------------|
| 0xc883c148   | 12     | DVIN                | Clock gating for DVIN             |

### Reset Registers
| Address      | Bit(s) | R/W | Default | Description    | Description                       |
|--------------|--------|-----|---------|----------------|-----------------------------------|
| 0xc11004414  | 4      | R/W | 0       | DVIN_RESET     | Reset control for DVIN            |

### Interrupt Sources
| A53 GIC Bit  | Interrupt Sources       | Description                       |
|--------------|-------------------------|-----------------------------------|
| 117          | vdin1_vsync_int         | VDIN 1 VSYNC interrupt            |
| 116          | vdin1_hsync_int         | VDIN 1 HSYNC interrupt            |
| 115          | vdin0_vsync_int         | VDIN 0 VSYNC interrupt            |
| 114          | vdin0_hsync_int         | VDIN 0 HSYNC interrupt            |

## GE2D (2D Graphics Engine)
### Memory Map
| Start       | End         | Region | CCI/NIC Arbitor | Description                       |
|-------------|-------------|--------|-----------------|-----------------------------------|
| 0xD0160000  | 0xD016FFFF  | GE2D   |                 | Memory region for GE2D            |

### Clock Tree
| Name          | Destination | Gate     | N           | Sel         | Description                              |
|---------------|-------------|----------|-------------|-------------|------------------------------------------|
| cts_ge2d_clk  | ge2d/dmc    | 0x7d[30] |             |             | Clock for GE2D                           |

### Reset Registers
| Address      | Bit(s) | R/W | Default | Description    | Description                       |
|--------------|--------|-----|---------|----------------|-----------------------------------|
| 0xc1100440c  | 6      | R/W | 0       | GE2D           | Reset control for GE2D            |

### Interrupt Sources
| A53 GIC Bit  | Interrupt Sources       | Description                       |
|--------------|-------------------------|-----------------------------------|
| 182          | ge2d_irq                | GE2D interrupt                    |

## HDMITX (HDMI Transmitter)
### Memory Map
| Start       | End         | Region | CCI/NIC Arbitor | Description                       |
|-------------|-------------|--------|-----------------|-----------------------------------|
| 0xC883A000  | 0xC883BFFF  | HDMITX |                 | Memory region for HDMITX          |
| 0xDA83A000  | 0xDA83BFFF  | HDMITX |                 | Additional memory region for HDMITX|

### Clock Gating
| Address      | Bit(s) | Module Description | Description                       |
|--------------|--------|---------------------|-----------------------------------|
| 0xc11004404  | 19     | Hdmitx_capb3        | Clock gating for HDMITX CAPB3     |

### Reset Registers
| Address      | Bit(s) | R/W | Default | Description    | Description                       |
|--------------|--------|-----|---------|----------------|-----------------------------------|
| 0xc1100440c  | 2      | R/W | 0       | HDMI_TX        | Reset control for HDMI TX         |
| 0xc883c148   | 4      | R/W | 0       | HDMI PCLK      | Reset control for HDMI PCLK       |
| 0xc883c148   | 3      | R/W | 0       | HDMI interrupt synchronization | Reset control for HDMI interrupt synchronization |

### Interrupt Sources
| A53 GIC Bit  | Interrupt Sources       | Description                       |
|--------------|-------------------------|-----------------------------------|
| 89           | hdmi_tx_interrupt       | HDMI TX interrupt                 |
| 87           | hdmi_cec_interrupt      | HDMI CEC interrupt                |
| 90           | hdcp22_irq              | HDCP 2.2 interrupt                |
