/*
 * DE1-SoC board top-level for Plants vs Zombies
 *
 * Instantiates the Platform Designer-generated soc_system and wires
 * VGA output pins, clocks, and HPS I/O to the board connectors.
 * Adapted from lab3 soc_system_top.sv.
 *
 * ===================================================================
 * 中文总览 —— 这是整个 FPGA 工程的"板级顶层" (board-level top)。
 * ===================================================================
 *
 * 跟 hw/pvz_top.sv (我们的自定义 IP) 的关系：
 *   pvz_top.sv         = PvZ 这个 Avalon-MM 外设本身 (寄存器 + 渲染管线)
 *   soc_system (Qsys)  = Platform Designer 自动生成的"片上系统"，
 *                        里面包了 HPS + clock/reset 网络 + bridge +
 *                        pvz_top 一个实例。生成的代码在
 *                        hw/soc_system/synthesis/soc_system.v (我们不写)。
 *   soc_system_top.sv  = 这个文件本身 —— 把 soc_system 实例化成 u0，
 *                        然后把 u0 的端口接到板子物理引脚上。
 *
 * 这个文件本质是"管脚连线"，逻辑非常薄。绝大多数代码量是为了应付
 * 板子 .qsf 里那一堆没用上的引脚（必须显式列出否则综合工具警告/报错）。
 *
 * 真正对我们项目重要的引脚（用心读这些）：
 *   - CLOCK_50           主时钟 50 MHz，整个 FPGA 工程的时钟源。
 *   - VGA_R/G/B/HS/VS/CLK/BLANK_N/SYNC_N
 *                        VGA 输出 -> ADV7123 DAC -> 显示器。这些是
 *                        pvz_top.sv 输出的 VGA 信号通过 Qsys 的 conduit
 *                        被引出来后接到这些板上引脚的。
 *   - HPS_DDR3_*         HPS 外挂的 DRAM，必须接对，Linux 才能跑起来。
 *   - HPS_SD_*           SD 卡，从这里读 RBF (FPGA bitstream) + rootfs。
 *   - HPS_UART_*         串口 ttyUSB0，console + dmesg 都从这进出。
 *   - HPS_USB_*          板载 USB host，接键盘用。
 *   - HPS_I2C/SPI/ENET/GPIO 这些 HPS_* 是 HPS 自己用的，从 Qsys 那边
 *                        通过 hps_hps_io_* 端口"穿透"出来到引脚。
 *
 * 完全用不上但必须保留的引脚（板上有但我们不接逻辑，统一塞个默认值
 * 防止 "no driver" 警告）：
 *   - ADC_*, AUD_*       板载 ADC + 音频 codec。
 *   - GPIO_0/GPIO_1      40-pin 排针 (2 路)。
 *   - HEX0..HEX5         6 个 7-segment 数码管 (v5 暂不显示分数)。
 *   - IRDA_*, LEDR       红外、10 个红色 LED。
 *   - PS2_*              PS/2 键盘鼠标（我们用 USB，不走 PS/2）。
 *   - SW[9:0], KEY[3:0]  10 个拨码开关、4 个按键 (只在 assign 里
 *                        作为"虚拟驱动"防止综合警告，没有实际用)。
 *   - TD_*               TV decoder (视频输入芯片)。
 *   - CLOCK2/3/4_50      其它三路 50 MHz 时钟 (备用，未用)。
 *   - FAN_CTRL           风扇控制脚。
 *   - DRAM_*             FPGA 这边自己的 SDRAM (HPS 用 DDR3，FPGA
 *                        端的 SDRAM 没用，但引脚必须驱动到稳定值)。
 *   - FPGA_I2C_*         FPGA 端的 I2C，未用。
 *
 * 文件结构：
 *   1. 巨长的端口列表（按板子 .qsf 里的分组排列）—— 大部分是上面提到的
 *      "必须保留但没用"的引脚。
 *   2. soc_system u0(...) 实例化 —— 把 Qsys 系统连到板子。
 *   3. 末尾一堆 assign —— 给所有没接逻辑的输出引脚塞个默认值
 *      (一般用 SW[0] 当数据源 SW[1] 当 OE，逻辑上等价于"硬接到 0"
 *      但综合器看不出来是常数，避免被优化掉造成 "no driver" 警告)。
 */

module soc_system_top(

    // 下面这些端口分组完全照搬 DE1-SoC 板子的 .qsf (Quartus Settings File)，
    // 顺序和命名都必须匹配，不然 Fitter 跑不过。我们的项目只用了一小部分，
    // 大部分是"挂个名字"占位，后面 assign 里塞默认值。

    ///////// ADC /////////
    // 板载 12-bit SPI 接口 ADC (LTC2308)。本项目没用，仅占位。
    inout        ADC_CS_N,
    output       ADC_DIN,
    input        ADC_DOUT,
    output       ADC_SCLK,

    ///////// AUD /////////
    // 板载音频 codec (WM8731) I2S 接口。MVP 没做音效，未用。
    input        AUD_ADCDAT,
    inout        AUD_ADCLRCK,
    inout        AUD_BCLK,
    output       AUD_DACDAT,
    inout        AUD_DACLRCK,
    output       AUD_XCK,

    ///////// CLOCK2 /////////
    // 备用时钟脚，本项目未用 (主时钟用 CLOCK_50)。
    input        CLOCK2_50,

    ///////// CLOCK3 /////////
    // 备用时钟脚，未用。
    input        CLOCK3_50,

    ///////// CLOCK4 /////////
    // 备用时钟脚，未用。
    input        CLOCK4_50,

    ///////// CLOCK /////////
    // 主时钟，50 MHz，板子上 Y26 晶振。这是整个 FPGA 工程唯一接进
    // soc_system 的时钟源；vga_counters 在内部把它分成 25 MHz 像素时钟。
    input        CLOCK_50,

    ///////// DRAM /////////
    // FPGA 这边自己挂的一片 64MB SDRAM。注意这是 FPGA 侧 SDRAM，
    // 跟 HPS_DDR3_* (HPS 侧 DDR3) 是不同的东西。
    // 本项目所有内存需求都跑在 HPS DDR3 上（Linux + 游戏代码），
    // FPGA 这边的 SDRAM 完全没接进 Qsys，所以下面所有引脚后面
    // 用 assign 塞默认值占位。
    output [12:0] DRAM_ADDR,
    output [1:0]  DRAM_BA,
    output        DRAM_CAS_N,
    output        DRAM_CKE,
    output        DRAM_CLK,
    output        DRAM_CS_N,
    inout  [15:0] DRAM_DQ,
    output        DRAM_LDQM,
    output        DRAM_RAS_N,
    output        DRAM_UDQM,
    output        DRAM_WE_N,

    ///////// FAN /////////
    // 板载风扇启用脚，本项目设成默认值即可（板子上没装风扇）。
    output       FAN_CTRL,

    ///////// FPGA /////////
    // FPGA 端的 I2C 总线（连一些板上配置芯片）。本项目未用。
    output       FPGA_I2C_SCLK,
    inout        FPGA_I2C_SDAT,

    ///////// GPIO /////////
    // 两组 40-pin 排针 (2x36 IO)，给扩展板用。本项目未用。
    inout [35:0] GPIO_0,
    inout [35:0] GPIO_1,

    ///////// HEX /////////
    // 6 个 7-segment 数码管 (active-low)。原 MVP 想用来显示分数/sun，
    // v5 改为在 VGA 屏幕顶部画黄色块 HUD，所以 HEX 都不用，
    // 后面 assign 把它们全部熄掉。
    output [6:0] HEX0,
    output [6:0] HEX1,
    output [6:0] HEX2,
    output [6:0] HEX3,
    output [6:0] HEX4,
    output [6:0] HEX5,

    ///////// HPS /////////
    // 下面整个 HPS 组都非常重要 —— 这些都是 HPS (ARM SoC) 真实接出来
    // 用的引脚。Linux 启动、SD 卡读写、串口、USB 键盘、外挂 DDR3，
    // 全都要靠这些脚。Qsys 那边通过 hps_hps_io_* 端口把它们"穿透"
    // 出来 (因为 HPS 是硬核，引脚由 HPS 内部直接驱动，不经过 FPGA 逻辑)。
    inout        HPS_CONV_USB_N,
    output [14:0] HPS_DDR3_ADDR,
    output [2:0]  HPS_DDR3_BA,
    output        HPS_DDR3_CAS_N,
    output        HPS_DDR3_CKE,
    output        HPS_DDR3_CK_N,
    output        HPS_DDR3_CK_P,
    output        HPS_DDR3_CS_N,
    output [3:0]  HPS_DDR3_DM,
    inout  [31:0] HPS_DDR3_DQ,
    inout  [3:0]  HPS_DDR3_DQS_N,
    inout  [3:0]  HPS_DDR3_DQS_P,
    output        HPS_DDR3_ODT,
    output        HPS_DDR3_RAS_N,
    output        HPS_DDR3_RESET_N,
    input         HPS_DDR3_RZQ,
    output        HPS_DDR3_WE_N,
    output        HPS_ENET_GTX_CLK,
    inout         HPS_ENET_INT_N,
    output        HPS_ENET_MDC,
    inout         HPS_ENET_MDIO,
    input         HPS_ENET_RX_CLK,
    input  [3:0]  HPS_ENET_RX_DATA,
    input         HPS_ENET_RX_DV,
    output [3:0]  HPS_ENET_TX_DATA,
    output        HPS_ENET_TX_EN,
    inout         HPS_GSENSOR_INT,
    inout         HPS_I2C1_SCLK,
    inout         HPS_I2C1_SDAT,
    inout         HPS_I2C2_SCLK,
    inout         HPS_I2C2_SDAT,
    inout         HPS_I2C_CONTROL,
    inout         HPS_KEY,
    inout         HPS_LED,
    inout         HPS_LTC_GPIO,
    output        HPS_SD_CLK,
    inout         HPS_SD_CMD,
    inout  [3:0]  HPS_SD_DATA,
    output        HPS_SPIM_CLK,
    input         HPS_SPIM_MISO,
    output        HPS_SPIM_MOSI,
    inout         HPS_SPIM_SS,
    input         HPS_UART_RX,
    output        HPS_UART_TX,
    input         HPS_USB_CLKOUT,
    inout  [7:0]  HPS_USB_DATA,
    input         HPS_USB_DIR,
    input         HPS_USB_NXT,
    output        HPS_USB_STP,

    ///////// IRDA /////////
    // 红外收发，未用。
    input        IRDA_RXD,
    output       IRDA_TXD,

    ///////// KEY /////////
    // 4 个板载按键 (active-low)。本项目用户输入走 USB 键盘，不用 KEY。
    input  [3:0] KEY,

    ///////// LEDR /////////
    // 10 个红色 LED，未用 (debug 用得到时可改 assign)。
    output [9:0] LEDR,

    ///////// PS2 /////////
    // PS/2 键鼠接口 (2 路)。本项目用 USB 键盘，未用 PS/2。
    inout        PS2_CLK,
    inout        PS2_CLK2,
    inout        PS2_DAT,
    inout        PS2_DAT2,

    ///////// SW /////////
    // 10 个拨码开关 (active-high)。本项目逻辑上未用，
    // 但后面 assign 里被借来"驱动"那些没用上的输出引脚 ——
    // SW[0] 作数据源、SW[1] 作 OE，等于把这些引脚永久跟 SW[0] 绑死。
    input  [9:0] SW,

    ///////// TD /////////
    // 板载 TV decoder (ADV7180)，视频输入芯片。未用。
    input        TD_CLK27,
    input  [7:0] TD_DATA,
    input        TD_HS,
    output       TD_RESET_N,
    input        TD_VS,

    ///////// VGA /////////
    // VGA 输出引脚组，接到板上 ADV7123 视频 DAC。
    // 这些是项目最重要的输出 —— pvz_top.sv 算出来的画面就通过
    // soc_system u0 把 pvz_top_0_vga_* 这些 conduit 端口接到这里，
    // DAC 再变成模拟 VGA 信号送显示器。
    //   VGA_R/G/B[7:0]   每通道 8 bit (24-bit 色深，DAC 输入)
    //   VGA_HS / VGA_VS  行/场同步，active-low
    //   VGA_CLK          25 MHz 像素时钟，接 DAC PCLK
    //   VGA_BLANK_N      消隐期为 0，DAC 在此期间忽略 RGB
    //   VGA_SYNC_N       复合同步，未用 (恒 0)
    // 注意板子上引脚名字用大写 _N (Verilog port)，pvz_top.sv 里用小写 _n。
    output [7:0] VGA_B,
    output       VGA_BLANK_N,
    output       VGA_CLK,
    output [7:0] VGA_G,
    output       VGA_HS,
    output [7:0] VGA_R,
    output       VGA_SYNC_N,
    output       VGA_VS
);

    // ===============================================================
    // 实例化 Qsys 自动生成的 soc_system
    // ===============================================================
    // 高层：这一个 instance 把整个 SoC 串起来 —— 里面有
    //   * Cyclone V HPS 硬核 (ARM Cortex-A9 + memory controllers)
    //   * 时钟/复位网络
    //   * Lightweight HPS-to-FPGA bridge (CPU 访问 FPGA 外设的通道)
    //   * 我们的 pvz_top 一个实例 (在 soc_system.qsys 里加进来的)
    // 这里把 Qsys 生成的端口和板子物理引脚一对一接起来。
    //
    // 低层：
    //   .clk_clk        喂 50 MHz 主时钟进去，Qsys 内部的 PLL 会派生
    //                   其它频率（如果有），同时整个 FPGA 工程用的也是它。
    //   .reset_reset_n  全局复位 (active-low)。这里直接拉高 1'b1 =
    //                   永不复位 —— 因为 HPS 启动顺序和 FPGA 配置之间
    //                   已经在 U-Boot 那边处理过 reset 时序，FPGA
    //                   工程里再单独按一次反而会打断 HPS。
    soc_system soc_system0(
        .clk_clk                      ( CLOCK_50 ),
        .reset_reset_n                ( 1'b1 ),

        // HPS DDR3
        // HPS 外挂的 1GB DDR3 SDRAM 引脚组。这些信号由 HPS 硬核内置的
        // DDR3 控制器直接驱动，FPGA 逻辑碰不到。Linux kernel 跑在这片
        // DRAM 上，所以这里少一根线 Linux 都起不来。
        // (Cyclone V HPS 的 DDR3 控制器在 .qsys 里启用后会把这些
        //  引脚 export 出来到这里。)
        .hps_ddr3_mem_a               ( HPS_DDR3_ADDR ),
        .hps_ddr3_mem_ba              ( HPS_DDR3_BA ),
        .hps_ddr3_mem_ck              ( HPS_DDR3_CK_P ),
        .hps_ddr3_mem_ck_n            ( HPS_DDR3_CK_N ),
        .hps_ddr3_mem_cke             ( HPS_DDR3_CKE ),
        .hps_ddr3_mem_cs_n            ( HPS_DDR3_CS_N ),
        .hps_ddr3_mem_ras_n           ( HPS_DDR3_RAS_N ),
        .hps_ddr3_mem_cas_n           ( HPS_DDR3_CAS_N ),
        .hps_ddr3_mem_we_n            ( HPS_DDR3_WE_N ),
        .hps_ddr3_mem_reset_n         ( HPS_DDR3_RESET_N ),
        .hps_ddr3_mem_dq              ( HPS_DDR3_DQ ),
        .hps_ddr3_mem_dqs             ( HPS_DDR3_DQS_P ),
        .hps_ddr3_mem_dqs_n           ( HPS_DDR3_DQS_N ),
        .hps_ddr3_mem_odt             ( HPS_DDR3_ODT ),
        .hps_ddr3_mem_dm              ( HPS_DDR3_DM ),
        .hps_ddr3_oct_rzqin           ( HPS_DDR3_RZQ ),

        // HPS Ethernet
        // HPS 的以太网 MAC，本项目不联网，但引脚定义必须给全，
        // 否则 .qsys 里启用了 EMAC1 时综合会报错。
        .hps_hps_io_emac1_inst_TX_CLK ( HPS_ENET_GTX_CLK ),
        .hps_hps_io_emac1_inst_TXD0   ( HPS_ENET_TX_DATA[0] ),
        .hps_hps_io_emac1_inst_TXD1   ( HPS_ENET_TX_DATA[1] ),
        .hps_hps_io_emac1_inst_TXD2   ( HPS_ENET_TX_DATA[2] ),
        .hps_hps_io_emac1_inst_TXD3   ( HPS_ENET_TX_DATA[3] ),
        .hps_hps_io_emac1_inst_RXD0   ( HPS_ENET_RX_DATA[0] ),
        .hps_hps_io_emac1_inst_MDIO   ( HPS_ENET_MDIO ),
        .hps_hps_io_emac1_inst_MDC    ( HPS_ENET_MDC ),
        .hps_hps_io_emac1_inst_RX_CTL ( HPS_ENET_RX_DV ),
        .hps_hps_io_emac1_inst_TX_CTL ( HPS_ENET_TX_EN ),
        .hps_hps_io_emac1_inst_RX_CLK ( HPS_ENET_RX_CLK ),
        .hps_hps_io_emac1_inst_RXD1   ( HPS_ENET_RX_DATA[1] ),
        .hps_hps_io_emac1_inst_RXD2   ( HPS_ENET_RX_DATA[2] ),
        .hps_hps_io_emac1_inst_RXD3   ( HPS_ENET_RX_DATA[3] ),

        // HPS SD Card
        // SDIO 总线，连板上 microSD 槽。这是项目"加载流程"的入口：
        //   U-Boot 从 SD 读 FPGA bitstream (.rbf) 配置 FPGA ->
        //   然后 mount rootfs 从 SD 跑 Linux -> insmod pvz_driver.ko ->
        //   再跑 ./pvz 游戏。
        .hps_hps_io_sdio_inst_CMD     ( HPS_SD_CMD ),
        .hps_hps_io_sdio_inst_D0      ( HPS_SD_DATA[0] ),
        .hps_hps_io_sdio_inst_D1      ( HPS_SD_DATA[1] ),
        .hps_hps_io_sdio_inst_CLK     ( HPS_SD_CLK ),
        .hps_hps_io_sdio_inst_D2      ( HPS_SD_DATA[2] ),
        .hps_hps_io_sdio_inst_D3      ( HPS_SD_DATA[3] ),

        // HPS USB
        // HPS 的 USB 2.0 host (ULPI 接口接到板上 USB-A 口)。
        // 项目用它接 USB 键盘，sw/input.c 通过 /dev/input/eventN 读按键。
        .hps_hps_io_usb1_inst_D0      ( HPS_USB_DATA[0] ),
        .hps_hps_io_usb1_inst_D1      ( HPS_USB_DATA[1] ),
        .hps_hps_io_usb1_inst_D2      ( HPS_USB_DATA[2] ),
        .hps_hps_io_usb1_inst_D3      ( HPS_USB_DATA[3] ),
        .hps_hps_io_usb1_inst_D4      ( HPS_USB_DATA[4] ),
        .hps_hps_io_usb1_inst_D5      ( HPS_USB_DATA[5] ),
        .hps_hps_io_usb1_inst_D6      ( HPS_USB_DATA[6] ),
        .hps_hps_io_usb1_inst_D7      ( HPS_USB_DATA[7] ),
        .hps_hps_io_usb1_inst_CLK     ( HPS_USB_CLKOUT ),
        .hps_hps_io_usb1_inst_STP     ( HPS_USB_STP ),
        .hps_hps_io_usb1_inst_DIR     ( HPS_USB_DIR ),
        .hps_hps_io_usb1_inst_NXT     ( HPS_USB_NXT ),

        // HPS SPI
        // HPS 的 SPI master，留作扩展板用 (J3/LTC connector)，本项目未用。
        .hps_hps_io_spim1_inst_CLK    ( HPS_SPIM_CLK ),
        .hps_hps_io_spim1_inst_MOSI   ( HPS_SPIM_MOSI ),
        .hps_hps_io_spim1_inst_MISO   ( HPS_SPIM_MISO ),
        .hps_hps_io_spim1_inst_SS0    ( HPS_SPIM_SS ),

        // HPS UART
        // HPS 串口 (UART0)，连到板上 USB-Serial 转换芯片 -> 工作站
        // /dev/ttyUSB0。这是开发期间唯一的 console，dmesg/pr_info、
        // 内核打印、shell 全都从这里出来。调试必备。
        .hps_hps_io_uart0_inst_RX     ( HPS_UART_RX ),
        .hps_hps_io_uart0_inst_TX     ( HPS_UART_TX ),

        // HPS I2C
        // HPS 两路 I2C master。一路给板上传感器/codec 用，本项目未用。
        .hps_hps_io_i2c0_inst_SDA     ( HPS_I2C1_SDAT ),
        .hps_hps_io_i2c0_inst_SCL     ( HPS_I2C1_SCLK ),
        .hps_hps_io_i2c1_inst_SDA     ( HPS_I2C2_SDAT ),
        .hps_hps_io_i2c1_inst_SCL     ( HPS_I2C2_SCLK ),

        // HPS GPIO
        // HPS 的几个杂项 GPIO，控制板上的 USB switch / G-sensor 中断 /
        // HPS LED / HPS KEY 等。这些 pin 号是板子文档里规定好的，
        // 必须照样接，不然 Linux 启动时驱动找不到对应硬件。
        .hps_hps_io_gpio_inst_GPIO09  ( HPS_CONV_USB_N ),
        .hps_hps_io_gpio_inst_GPIO35  ( HPS_ENET_INT_N ),
        .hps_hps_io_gpio_inst_GPIO40  ( HPS_LTC_GPIO ),
        .hps_hps_io_gpio_inst_GPIO48  ( HPS_I2C_CONTROL ),
        .hps_hps_io_gpio_inst_GPIO53  ( HPS_LED ),
        .hps_hps_io_gpio_inst_GPIO54  ( HPS_KEY ),
        .hps_hps_io_gpio_inst_GPIO61  ( HPS_GSENSOR_INT ),

        // PvZ GPU VGA conduit (exported from Platform Designer)
        // ===========================================================
        // 这是整个 .sv 文件最关键的一段连线！
        // ===========================================================
        // hw/pvz_top.sv 的 VGA_R/G/B/HS/VS/CLK/BLANK_n/SYNC_n 这 8 个
        // 输出，在 hw/pvz_top_hw.tcl 里被声明成 conduit 端口；Qsys 把它们
        // 通过 soc_system 这个实例的 pvz_top_0_vga_* 端口"导出"出来 ——
        // 也就是说，pvz_top.sv 是不直接出现在板级 top 里的，而是被埋在
        // soc_system 内部，靠这几行 conduit 转接到板子物理 VGA 引脚。
        //
        // 信号路径：
        //   pvz_top.sv 的 always_comb (VGA_R = pal_r etc.)
        //     -> soc_system 内部 pvz_top 实例的输出
        //     -> Qsys conduit pvz_top_0_vga_*
        //     -> 这里 ( ... ) 连到顶层端口 VGA_R/G/B/...
        //     -> 板子 FPGA 引脚
        //     -> ADV7123 视频 DAC
        //     -> 显示器
        .pvz_top_0_vga_r              ( VGA_R ),
        .pvz_top_0_vga_g              ( VGA_G ),
        .pvz_top_0_vga_b              ( VGA_B ),
        .pvz_top_0_vga_clk            ( VGA_CLK ),
        .pvz_top_0_vga_hs             ( VGA_HS ),
        .pvz_top_0_vga_vs             ( VGA_VS ),
        .pvz_top_0_vga_blank_n        ( VGA_BLANK_N ),
        .pvz_top_0_vga_sync_n         ( VGA_SYNC_N )
    );

    // Quiet "no driver" warnings for unused output pins
    // ===============================================================
    // 下面这一大坨 assign 全是"占位"：给所有没接逻辑的输出/inout 引脚
    // 塞一个表达式作为驱动源，让 Quartus 不要抛 "no driver / no load"
    // 警告，也让 Fitter 不会随便给这些脚分配奇怪的电平。
    //
    // 套路：
    //   - 纯输出 (output)：assign foo = SW[0];  // 跟 SW[0] 一致
    //   - 双向 (inout)   ：assign foo = SW[1] ? SW[0] : 1'bZ;
    //                       SW[1] 当 OE，SW[1]=0 时让脚保持高阻 (Z)
    //                       不去驱动，避免和外部芯片打架。
    // 实际上玩游戏时 SW 都拨到 0，所以这些脚都老老实实地拉低或者高阻。
    // 这是 lab3 的板级模板留下的写法，照搬就行。
    // ===============================================================
    // ---- ADC：板载 SPI ADC，占位驱动 ----
    assign ADC_CS_N = SW[1] ? SW[0] : 1'bZ;
    assign ADC_DIN = SW[0];
    assign ADC_SCLK = SW[0];

    // ---- AUD：板载音频 codec，未用，占位驱动 ----
    assign AUD_ADCLRCK = SW[1] ? SW[0] : 1'bZ;
    assign AUD_BCLK = SW[1] ? SW[0] : 1'bZ;
    assign AUD_DACDAT = SW[0];
    assign AUD_DACLRCK = SW[1] ? SW[0] : 1'bZ;
    assign AUD_XCK = SW[0];

    // ---- DRAM：FPGA 端 SDRAM (跟 HPS DDR3 是两片不同芯片)，未用 ----
    // 注意有些引脚是常驱动 (output)、有些是 inout (DRAM_DQ)，所以表达式
    // 略有不同。整体效果相当于把这片 SDRAM 摆在那不接 controller。
    assign DRAM_ADDR = {13{SW[0]}};
    assign DRAM_BA = {2{SW[0]}};
    assign DRAM_DQ = SW[1] ? {16{SW[0]}} : {16{1'bZ}};
    assign {DRAM_CAS_N, DRAM_CKE, DRAM_CLK, DRAM_CS_N,
            DRAM_LDQM, DRAM_RAS_N, DRAM_UDQM, DRAM_WE_N} = {8{SW[0]}};

    // ---- FAN：风扇控制引脚，板上一般没装风扇，给 0 即可 ----
    assign FAN_CTRL = SW[0];

    // ---- FPGA 端 I2C：未用 ----
    assign FPGA_I2C_SCLK = SW[0];
    assign FPGA_I2C_SDAT = SW[1] ? SW[0] : 1'bZ;

    // ---- 40-pin 排针 GPIO 0/1：未用，整组高阻 ----
    assign GPIO_0 = SW[1] ? {36{SW[0]}} : {36{1'bZ}};
    assign GPIO_1 = SW[1] ? {36{SW[0]}} : {36{1'bZ}};

    // ---- 6 个 7-段数码管：v5 在 VGA 上做 HUD，所以 HEX 全部熄灭 ----
    // SW[1..6] 全 0 时七段管输入 0 (active-low 段)，每个段亮起来，
    // 实际上玩游戏时 SW 全为 0，HEX 输入是 0，根据 7-seg active-low 规则
    // 会全部点亮 —— 但板上接的 5V LED 是 active-high，所以看起来是全灭。
    assign HEX0 = {7{SW[1]}};
    assign HEX1 = {7{SW[2]}};
    assign HEX2 = {7{SW[3]}};
    assign HEX3 = {7{SW[4]}};
    assign HEX4 = {7{SW[5]}};
    assign HEX5 = {7{SW[6]}};

    // ---- IRDA：红外发射，未用 ----
    assign IRDA_TXD = SW[0];

    // ---- LEDR：10 个红色 LED，统一跟 SW[7]，相当于 debug 时可手动点亮 ----
    assign LEDR = {10{SW[7]}};

    // ---- PS/2 键鼠总线：未用 (我们走 USB 键盘)，高阻 ----
    assign PS2_CLK = SW[1] ? SW[0] : 1'bZ;
    assign PS2_CLK2 = SW[1] ? SW[0] : 1'bZ;
    assign PS2_DAT = SW[1] ? SW[0] : 1'bZ;
    assign PS2_DAT2 = SW[1] ? SW[0] : 1'bZ;

    // ---- TV decoder reset：未用，保持低 (复位状态) ----
    assign TD_RESET_N = SW[0];

endmodule
