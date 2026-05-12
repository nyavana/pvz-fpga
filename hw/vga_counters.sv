/*
 * VGA 640x480@60Hz timing generator
 *
 * Extracted from lab3 vga_ball.sv (Stephen A. Edwards, Columbia University).
 * Uses a 50 MHz input clock; one pixel every other cycle (25 MHz pixel clock).
 *
 * hcount[10:1] = pixel column (0-639 active)
 * vcount[9:0]  = pixel row   (0-479 active)
 *
 * ===================================================================
 * 中文说明 —— 整个渲染流水线的"心跳"就在这个文件里。
 * ===================================================================
 *
 * 这个模块就是用来产生 VGA 640x480@60 Hz 标准时序的，从 DE1-SoC 板上
 * CLOCK_50 那根 50 MHz 引脚拿到时钟，然后输出：
 *   - hcount / vcount：当前正在扫描哪个像素 (px = hcount[10:1], py = vcount)
 *   - VGA_CLK：25 MHz 的像素时钟（接到 ADV7123 DAC 的 PCLK 引脚）
 *   - VGA_HS / VGA_VS：行同步、场同步（active-low 脉冲）
 *   - VGA_BLANK_n：消隐期为 0，可见区域为 1（pvz_top.sv 在 blanking 时强制黑屏）
 *   - VGA_SYNC_n：恒 0，因为没有用 sync-on-green 复合同步
 *
 * 高层：v5 里没有帧缓存、没有行缓存，整个图像是"racing the beam"
 *      一边扫一边算。entity_drawer.sv 接收这里的 (px, py)，组合逻辑
 *      算出颜色再喂回 VGA_R/G/B。所以这个模块决定了所有下游模块的
 *      时间基准。
 *
 * 低层：50 MHz 进来后，每个 50 MHz cycle 都让 hcount + 1，VGA_CLK
 *      直接拿 hcount[0]，自然就变成 25 MHz 方波（一个高一个低）。
 *      所以一行其实占 50 MHz 上的 1600 个 cycle = 25 MHz 上的 800 个
 *      像素时钟，但只有 640 个是可见像素，剩下 160 个是 front porch /
 *      sync / back porch。详见下面 HACTIVE 等参数。
 */

module vga_counters(
    input  logic        clk50,        // 板上 CLOCK_50，50 MHz
    input  logic        reset,        // 高电平复位（来自 KEY 或 HPS）
    output logic [10:0] hcount,       // 水平计数器，范围 0..1599（50 MHz cycle 计数）
    output logic [9:0]  vcount,       // 垂直计数器，范围 0..524（一行计一次）
    output logic        VGA_CLK,      // 25 MHz 像素时钟，接 DAC PCLK
    output logic        VGA_HS,       // 水平同步，active-low
    output logic        VGA_VS,       // 垂直同步，active-low
    output logic        VGA_BLANK_n,  // 消隐控制，1=可见 0=消隐
    output logic        VGA_SYNC_n    // 复合同步，恒 0（未使用）
);

    // 水平时序参数（单位是 50 MHz cycle，所以像素数 = 参数 / 2）
    // 高层：标准 VGA 640x480@60 Hz 要求水平方向 800 个像素时钟，对应
    //       50 MHz 下 1600 个 cycle。
    // 低层：HACTIVE=1280 -> 可见 640 像素；剩下 320 cycle (160 像素)
    //       分给 front porch / hsync / back porch，加起来正好 1600。
    parameter HACTIVE      = 11'd1280,  // 可见区，640 像素 * 2 = 1280 cycle
              HFRONT_PORCH = 11'd32,    // 行可见区结束后的留白
              HSYNC        = 11'd192,   // 行同步脉冲宽度
              HBACK_PORCH  = 11'd96,    // 行同步后到下一行开始的留白
              HTOTAL       = HACTIVE + HFRONT_PORCH + HSYNC + HBACK_PORCH; // 1600

    // 垂直时序参数（单位是"行"，一行 = HTOTAL 个 50 MHz cycle）
    // 高层：标准 VGA 一帧 525 行，60 Hz 刷新 -> 31.5 kHz 行频。
    // 低层：VACTIVE 480 行可见 + 10 行前肩 + 2 行 VS + 33 行后肩 = 525。
    parameter VACTIVE      = 10'd480,   // 可见行数
              VFRONT_PORCH = 10'd10,    // 场可见区结束后的留白
              VSYNC        = 10'd2,     // 场同步脉冲（只有 2 行）
              VBACK_PORCH  = 10'd33,    // 场同步后的留白
              VTOTAL       = VACTIVE + VFRONT_PORCH + VSYNC + VBACK_PORCH; // 525

    // endOfLine：本行最后一个 cycle，下一拍 hcount 要绕回 0
    logic endOfLine;
    assign endOfLine = hcount == HTOTAL - 1;

    // endOfField：本场最后一行，下一拍 vcount 要绕回 0
    logic endOfField;
    assign endOfField = vcount == VTOTAL - 1;

    // hcount 计数器：每个 50 MHz 时钟 + 1，到行末就清零。
    // 这是最基础的"扫描列地址"，VGA_CLK / hcount[0] 由它派生。
    always_ff @(posedge clk50 or posedge reset)
        if (reset)          hcount <= 0;
        else if (endOfLine) hcount <= 0;
        else                hcount <= hcount + 11'd1;

    // vcount 计数器：只有在行末那一拍才 + 1（每行加 1 次），
    // 场末再绕回 0。这样 hcount/vcount 一起就完整定位了某个像素。
    always_ff @(posedge clk50 or posedge reset)
        if (reset)          vcount <= 0;
        else if (endOfLine)
            if (endOfField) vcount <= 0;
            else            vcount <= vcount + 10'd1;

    // Horizontal sync: active low during sync pulse
    // 高层：HS 在 active 区之后的 front porch 走完才拉低，持续 HSYNC=192 个 cycle。
    // 低层：用位切片技巧而不是比较器 —— hcount[10:8]==3'b101 表示 hcount
    //      在 [1280..1535] 这段（即可见区 1280 之后），再排除 hcount[7:5]==3'b111
    //      （即 [1504..1535]）就得到 [1312..1503]，正好 192 个 cycle 的脉冲。
    //      这样 LUT 资源比直接写 (hcount>=1312 && hcount<1504) 省得多。
    assign VGA_HS = !( (hcount[10:8] == 3'b101) &
                       !(hcount[7:5] == 3'b111) );

    // Vertical sync: active low during sync pulse
    // 高层：VS 在 480 行可见 + 10 行 front porch 之后拉低 2 行。
    // 低层：vcount[9:1]==245 同时覆盖 vcount=490 和 491 这两行
    //      （右移 1 等于除 2 取整），等价于 (vcount==490 || vcount==491)。
    assign VGA_VS = !( vcount[9:1] == 9'd245 ); // (VACTIVE+VFRONT_PORCH)/2

    // 复合同步线没用上（DE1-SoC 的 ADV7123 走分离同步），直接置 0。
    assign VGA_SYNC_n = 1'b0; // Unused — composite sync on green

    // Blanking: active high when in visible area
    // 高层：消隐线 = 当前像素是不是在 640x480 可见区里。pvz_top.sv 里
    //       用它做最后一关：消隐期强制输出 RGB=0，否则有些显示器会乱画 porch。
    // 低层：水平方向 hcount<1280 表示可见，等价于 hcount[10] 为 0
    //       或 (hcount[10]=1 且 hcount[9:8]==00) —— 那一段排除掉就行。
    //       垂直方向 vcount<480 表示可见，等价于 vcount[9]=0 且
    //       vcount[8:5] != 4'b1111 （也就是 vcount 不在 [480..511] 这段）。
    assign VGA_BLANK_n = !( hcount[10] & (hcount[9] | hcount[8]) ) &
                         !( vcount[9] | (vcount[8:5] == 4'b1111) );

    // 25 MHz pixel clock from 50 MHz input
    // hcount[0] 在每个 50 MHz 周期翻转一次 -> 25 MHz 方波。
    // 一个像素对应连续两个 50 MHz 时钟（一个 VGA_CLK 高一个低），
    // 所以 px = hcount[10:1] 这种"右移 1"的算法就是在恢复像素索引。
    assign VGA_CLK = hcount[0];

endmodule
