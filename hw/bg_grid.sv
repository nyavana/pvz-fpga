/*
 * Background grid — 8 cols x 4 rows, 64x64 px each
 *
 * Game area: x in [64, 576), y in [112, 368).  The lawn light_cell pattern
 * is hardcoded (alternating dark/light green by (row+col) parity), so
 * no CPU writes or double buffering are needed.
 *
 * Output: given pixel (px, py), returns the background color index.
 *   - in game area: dark green or light green light_cell
 *   - outside    : blue
 *
 * Cell math uses bit slices because 64 = 2^6.
 *
 * ===================================================================
 * 中文说明
 * ===================================================================
 *
 * 这个模块就是用来画"草坪格子背景"的：4 行 x 8 列，每格 64x64 像素，
 * 整个网格锚在屏幕 (64, 112)。深绿浅绿交替排成棋盘，让玩家一眼能
 * 数出格子；网格外的地方画天蓝色当背景。
 *
 * 高层：纯组合逻辑，没有寄存器。给个 (px, py) 立刻得到颜色索引，
 *      entity_drawer.sv 把这个索引作为最底层来叠加植物/僵尸/HUD。
 *      因为图案是硬编码的，软件完全不用管背景，省下宝贵的 Avalon
 *      带宽和帧间同步麻烦。
 *
 * 低层：CELL=64=2^6，所以 (px-64)/64 等价于取高位、(px-64)%64
 *      等价于取低 6 位 —— 完全不需要除法器。棋盘奇偶用一个异或就搞定。
 *
 * 颜色索引要和 hw/color_palette.sv 严格对齐，否则查 LUT 时颜色就错乱。
 */

module bg_grid(
    input  logic [9:0] px,            // 当前像素 x，来自 vga_counters 的 hcount[10:1]
    input  logic [9:0] py,            // 当前像素 y，来自 vga_counters 的 vcount
    output logic [7:0] color_out      // 8-bit 颜色索引，交给 color_palette 查 RGB
);

    // Color indices (must match color_palette.sv)
    // 颜色索引常量，必须和 hw/color_palette.sv 的 case 完全对得上。
    // 这里只用到了棋盘的深绿/浅绿和外围的天蓝；BLACK 留作占位（暂未使用）。
    localparam logic [7:0] COL_BLACK       = 8'd0;
    localparam logic [7:0] COL_DARK_GREEN  = 8'd1;   // 棋盘暗格
    localparam logic [7:0] COL_LIGHT_GREEN = 8'd2;   // 棋盘亮格
    localparam logic [7:0] COL_BLUE = 8'd13;          // 网格外的天蓝背景

    // Game area bounds
    // 网格几何，必须和 sw/pvz.h 里 PVZ_GRID_X/Y/CELL_SIZE 一致，
    // 也必须和 hw/entity_drawer.sv 里的 GRID_X/GRID_Y/CELL 一致。
    localparam logic [9:0] GRID_X = 10'd64;    // 网格左上角屏幕 x
    localparam logic [9:0] GRID_Y = 10'd112;   // 网格左上角屏幕 y（顶上留 112 px 放 HUD）
    localparam logic [9:0] GRID_W = 10'd512; // 8 cols x 64
    localparam logic [9:0] GRID_H = 10'd256; // 4 rows x 64

    // 判断当前像素是否落在游戏区里。两个范围都满足才算 in_grid。
    wire in_grid = (px >= GRID_X) && (px < GRID_X + GRID_W) &&
                   (py >= GRID_Y) && (py < GRID_Y + GRID_H);

    // Within-grid offset (full 10 bits; high bit unused inside grid).
    // Cell index = offset >> 6 because cell size is 64.
    // 网格内的相对坐标 (gx, gy)：从网格左上角算起的像素偏移。
    // 因为格子 64x64 = 2^6，所以：
    //   gx[8:6] = 列号 (0..7)，gy[7:6] = 行号 (0..3)
    //   gx[5:0] = 格子内的 x (0..63)，gy[5:0] = 格子内的 y
    // 这种"位切片"代替乘除法是 FPGA 上经典的小聪明。
    /* verilator lint_off UNUSED */
    wire [9:0] gx = px - GRID_X;
    wire [9:0] gy = py - GRID_Y;
    /* verilator lint_on UNUSED */

    // Checker pattern: alternate by (col + row) parity.  col is gx[8:6],
    // row is gy[7:6] — only the LSB of each matters for parity.
    // 棋盘奇偶 = (列号 + 行号) 的奇偶 = (列号最低位) XOR (行号最低位)。
    // 而列号最低位就是 gx[6]，行号最低位就是 gy[6]（因为 >>6 之后
    // 第 0 位就是 [6]）。所以一个异或就出来了，完全不需要加法器。
    wire light_cell = gx[6] ^ gy[6];

    // 三选一的组合逻辑：在网格外 -> 天蓝；棋盘亮格 -> 浅绿；否则深绿。
    always_comb begin
        if (!in_grid)
            color_out = COL_BLUE;
        else if (light_cell)
            color_out = COL_LIGHT_GREEN;
        else
            color_out = COL_DARK_GREEN;
    end

endmodule
