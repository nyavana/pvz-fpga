/*
 * 256-entry color palette LUT: 8-bit index -> 24-bit RGB
 *
 * Hardcoded with 13 MVP colors. Remaining entries default to black.
 *
 * Index  Color         R    G    B    Usage
 * -----  -----------   ---  ---  ---  -----
 *   0    Black         00   00   00   Unused
 *   1    Dark Green    1B   5E   20   Grid cell (dark)
 *   2    Light Green   2D   8B   2D   Grid cell (light)
 *   3    Brown         8B   45   13   Soil / stem
 *   4    Yellow        FF   D7   00   Cursor highlight
 *   5    Red           FF   00   00   Zombie body
 *   6    Dark Red      8B   00   00   Zombie head
 *   7    Green         00   80   00   Peashooter body
 *   8    Dark Green2   00   64   00   Peashooter stem
 *   9    Bright Green  00   FF   00   Pea projectile
 *  10    White         FF   FF   FF   HUD digits
 *  11    Gray          80   80   80   HUD background
 *  12    Orange        FF   A5   00   Sun indicator
 *  13    Sky Blue      87   CE   EB   Background
 *
 * ===================================================================
 * 中文说明
 * ===================================================================
 *
 * 这个模块就是个"颜色查找表"：拿 8-bit 索引进来，吐出 24-bit RGB 出去。
 *
 * 高层：所有上游模块（bg_grid、sprite_rom、entity_drawer）输出的都是
 *      "颜色索引"而不是真的 RGB —— 这种"调色板渲染"是 8/16 位游戏机
 *      时代的经典手法。好处：
 *        1. 每个像素只占 1 字节，sprite ROM 大小直接砍到 1/3。
 *        2. 换风格 / 调色只需要改这一个文件，不用动 sprite 数据。
 *        3. 透明色 (0xFF) 只是约定，不占用 RGB 空间。
 *
 * 低层：综合工具会把这个 case 综合成一个组合 ROM/LUT。没有时钟，
 *      没有寄存器 —— 当前像素的颜色这一拍就能用，时序好排。
 *      256 个索引里只硬编码了 13 个真颜色，剩下 243 个全是黑。
 *      想加新颜色（比如葵花花瓣的金色），直接在 case 里加一行即可，
 *      不会影响其他模块。
 *
 * 关于 0xFF 透明色：entity_drawer.sv 在做层叠时会先比较
 * `plant_rd_pixel != COL_TRANSPARENT` 才决定要不要画 plant 层，
 * 所以 0xFF 实际上永远不会传到这个 LUT 上。即便误传过来，default
 * 分支也会兜底给黑。
 */

module color_palette(
    input  logic [7:0]  index,    // 颜色索引，来自 entity_drawer.color_out
    output logic [7:0]  r,        // 红通道，8 bit
    output logic [7:0]  g,        // 绿通道，8 bit
    output logic [7:0]  b         // 蓝通道，8 bit
);

    // 组合 case：每个索引对应一组 RGB；综合后是一个小 LUT 或 LCELL 阵列。
    // 注意 default 分支：任何没列出来的索引（包括 0xFF 透明色万一漏过来）
    // 都会画黑色，这是一个安全兜底。
    always_comb begin
        case (index)
            8'd0:  {r, g, b} = {8'h00, 8'h00, 8'h00}; // Black
            8'd1:  {r, g, b} = {8'h1B, 8'h5E, 8'h20}; // Dark Green
            8'd2:  {r, g, b} = {8'h2D, 8'h8B, 8'h2D}; // Light Green
            8'd3:  {r, g, b} = {8'h8B, 8'h45, 8'h13}; // Brown
            8'd4:  {r, g, b} = {8'hFF, 8'hD7, 8'h00}; // Yellow
            8'd5:  {r, g, b} = {8'hFF, 8'h00, 8'h00}; // Red
            8'd6:  {r, g, b} = {8'h8B, 8'h00, 8'h00}; // Dark Red
            8'd7:  {r, g, b} = {8'h00, 8'h80, 8'h00}; // Green
            8'd8:  {r, g, b} = {8'h00, 8'h64, 8'h00}; // Dark Green 2
            8'd9:  {r, g, b} = {8'h00, 8'hFF, 8'h00}; // Bright Green
            8'd10: {r, g, b} = {8'hFF, 8'hFF, 8'hFF}; // White
            8'd11: {r, g, b} = {8'h80, 8'h80, 8'h80}; // Gray
            8'd12: {r, g, b} = {8'hFF, 8'hA5, 8'h00}; // Orange
            8'd13: {r, g, b} = {8'h87, 8'hCE, 8'hEB}; // Sky Blue
            default: {r, g, b} = {8'h00, 8'h00, 8'h00}; // Black
        endcase
    end

endmodule
