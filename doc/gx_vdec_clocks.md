# GX VDEC Clocks Control Configuration

## cts_vdec_clk Configuration

| Register         | Field         | Bit Range   | Values/Description                                                                          |
|------------------|--------------|-------------|---------------------------------------------------------------------------------------------|
| HHI_VDEC_CLK_CNTL (0x78) | CLK_SEL       | 11–9        | 0: fclk_div4<br>1: fclk_div3<br>2: fclk_div5<br>3: fclk_div7<br>4: mp1<br>5: mp2<br>6: gp0<br>7: xtal   |
| HHI_VDEC_CLK_CNTL (0x78) | CLK_DIV       | 6–0         | Divider N: output = source / (N+1); N = 0–127                                               |
| HHI_VDEC_CLK_CNTL (0x78) | CLK_EN        | 8           | 0: Disabled<br>1: Enabled                                                                   |
| HHI_VDEC3_CLK_CNTL (0x7A) | ALT_CLK_SEL   | 11–9        | Same as above                                                                               |
| HHI_VDEC3_CLK_CNTL (0x7A) | ALT_CLK_DIV   | 6–0         | Same as above                                                                               |
| HHI_VDEC3_CLK_CNTL (0x7A) | ALT_CLK_EN    | 8           | 0: Disabled<br>1: Enabled                                                                   |
| HHI_VDEC3_CLK_CNTL (0x7A) | ALT_ENABLE    | 15          | 0: Use primary (0x78) config<br>1: Use alternate (0x7A) config                              |

***

## cts_hcodec_clk Configuration

| Register         | Field         | Bit Range   | Values/Description                                                                          |
|------------------|--------------|-------------|---------------------------------------------------------------------------------------------|
| HHI_VDEC_CLK_CNTL (0x78) | CLK_SEL       | 27–25       | 0: fclk_div4<br>1: fclk_div3<br>2: fclk_div5<br>3: fclk_div7<br>4: mp1<br>5: mp2<br>6: gp0<br>7: xtal   |
| HHI_VDEC_CLK_CNTL (0x78) | CLK_DIV       | 22–16       | Divider N: output = source / (N+1); N = 0–127                                               |
| HHI_VDEC_CLK_CNTL (0x78) | CLK_EN        | 24          | 0: Disabled<br>1: Enabled                                                                   |
| HHI_VDEC3_CLK_CNTL (0x7A) | ALT_CLK_SEL   | 27–25       | Same as above                                                                               |
| HHI_VDEC3_CLK_CNTL (0x7A) | ALT_CLK_DIV   | 22–16       | Same as above                                                                               |
| HHI_VDEC3_CLK_CNTL (0x7A) | ALT_CLK_EN    | 24          | 0: Disabled<br>1: Enabled                                                                   |
| HHI_VDEC3_CLK_CNTL (0x7A) | ALT_ENABLE    | 31          | 0: Use primary (0x78) config<br>1: Use alternate (0x7A) config                              |

***

## cts_vdec2_clk Configuration

| Register         | Field         | Bit Range   | Values/Description                                                                          |
|------------------|--------------|-------------|---------------------------------------------------------------------------------------------|
| HHI_VDEC2_CLK_CNTL (0x79) | CLK_SEL       | 11–9        | 0: fclk_div4<br>1: fclk_div3<br>2: fclk_div5<br>3: fclk_div7<br>4: mp1<br>5: mp2<br>6: gp0<br>7: xtal   |
| HHI_VDEC2_CLK_CNTL (0x79) | CLK_DIV       | 6–0         | Divider N: output = source / (N+1); N = 0–127                                               |
| HHI_VDEC2_CLK_CNTL (0x79) | CLK_EN        | 8           | 0: Disabled<br>1: Enabled                                                                   |
| HHI_VDEC4_CLK_CNTL (0x7B) | ALT_CLK_SEL   | 11–9        | Same as above                                                                               |
| HHI_VDEC4_CLK_CNTL (0x7B) | ALT_CLK_DIV   | 6–0         | Same as above                                                                               |
| HHI_VDEC4_CLK_CNTL (0x7B) | ALT_CLK_EN    | 8           | 0: Disabled<br>1: Enabled                                                                   |
| HHI_VDEC4_CLK_CNTL (0x7B) | ALT_ENABLE    | 15          | 0: Use primary (0x79) config<br>1: Use alternate (0x7B) config                              |

***

## cts_hevc_clk Configuration

| Register         | Field         | Bit Range   | Values/Description                                                                          |
|------------------|--------------|-------------|---------------------------------------------------------------------------------------------|
| HHI_VDEC2_CLK_CNTL (0x79) | CLK_SEL       | 27–25       | 0: fclk_div4<br>1: fclk_div3<br>2: fclk_div5<br>3: fclk_div7<br>4: mp1<br>5: mp2<br>6: gp0<br>7: xtal   |
| HHI_VDEC2_CLK_CNTL (0x79) | CLK_DIV       | 22–16       | Divider N: output = source / (N+1); N = 0–127                                               |
| HHI_VDEC2_CLK_CNTL (0x79) | CLK_EN        | 24          | 0: Disabled<br>1: Enabled                                                                   |
| HHI_VDEC4_CLK_CNTL (0x7B) | ALT_CLK_SEL   | 27–25       | Same as above                                                                               |
| HHI_VDEC4_CLK_CNTL (0x7B) | ALT_CLK_DIV   | 22–16       | Same as above                                                                               |
| HHI_VDEC4_CLK_CNTL (0x7B) | ALT_CLK_EN    | 24          | 0: Disabled<br>1: Enabled                                                                   |
| HHI_VDEC4_CLK_CNTL (0x7B) | ALT_ENABLE    | 31          | 0: Use primary (0x79) config<br>1: Use alternate (0x7B) config                              |
