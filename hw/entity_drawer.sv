/*
 * Entity drawer — "racing the beam" combinational pixel renderer
 *
 * For every VGA pixel (px, py), determines the final color by checking
 * which game entity (if any) covers that pixel.  No frame buffer, no
 * line buffer, no FSM: the rendering happens in lock-step with the VGA
 * scan.
 *
 * Layering (low to high; later overwrites earlier):
 *   1. bg          (lawn checker, from bg_grid)
 *   2. plant       (64x64 sprite ROM, 1:1)
 *   3. pea         (small bright-green square)
 *   4. zombie      (64x64 sprite ROM, 1:1, with transparency)
 *   5. cursor      (yellow border around cursor cell)
 *   6. sun HUD     (yellow blocks at top, one per 100 sun)
 *   7. selector    (two plant icons at top-left; the chosen one
 *                   wears the yellow border, which TAB cycles)
 *
 * Sprite ROMs have 1 clock of read latency.  Stage 1 issues addresses
 * combinationally, stage 2 (one cycle later) merges the ROM outputs with
 * the registered overlay hits.  Final color is registered so the VGA
 * data path stays clean.
 *
 * Layout constants (must match bg_grid.sv and software):
 *   GRID_X = 64, GRID_Y = 112, CELL = 64, GRID_COLS = 8, GRID_ROWS = 4
 *
 * ===================================================================
 * 中文说明 —— 这是整个渲染流水线的"心脏"，新成员请仔细读完
 * ===================================================================
 *
 * 这个模块是 v5 渲染管线最长、最难、也最值得细看的部分。它的工作
 * 就一句话：给一个 VGA 扫描出来的像素坐标 (px, py)，输出最终颜色。
 * 没有帧缓存、没有行缓存、没有状态机 —— 完全是"跟着电子枪一起跑"
 * 的硬件渲染。
 *
 * ----------------------------- 层叠关系 -----------------------------
 * 把屏幕看成一摞透明图层，从下往上依次绘制（后面的会盖住前面的）：
 *   1. bg       —— 草坪棋盘（bg_grid.sv 给的颜色，作为底色）
 *   2. plant    —— 豌豆射手或向日葵的 64x64 sprite，正好一格大
 *   3. pea      —— 8x8 的亮绿色方块（豌豆子弹）
 *   4. zombie   —— 64x64 zombie sprite，带透明色
 *   5. cursor   —— 玩家光标，一个空心黄色方框
 *   6. sun HUD  —— 屏幕顶部的黄色阳光条，每 50 阳光点亮一块
 *   7. selector —— 左上角两个植物图标 + 选中那个的黄色描边
 *
 * 这个顺序很重要：把 HUD 放最上面，玩家就不会被植物/僵尸挡住界面。
 * 注意 selector 也在最顶，因此即使 sun HUD 把它压住，依然能看到。
 *
 * ----------------------------- 两段流水线 ----------------------------
 * Sprite ROM 有 1 cycle 的读延迟，所以我们必须把渲染切成两段：
 *
 *   Stage 1 (本拍组合逻辑):
 *     - 算出当前像素属于哪个 grid cell；
 *     - 查 plant_present / sunflower_present 决定要不要画植物；
 *     - 用 plant_rd_addr 给 plant/sunflower ROM 喂地址；
 *     - 算 zombie 命中（哪个槽位的 zombie 覆盖了这个像素），
 *       同时给 zombie_rd_addr 喂地址；
 *     - 算 pea / cursor / selector / sun HUD 这些不需要 sprite 的 hit。
 *
 *   时钟沿（寄存器级）：
 *     - 把 stage 1 所有 hit 信号和 bg_color 一起锁存进 _d 后缀的寄存器。
 *     - 此时 sprite ROM 也在用 stage 1 给的地址做查表，下一拍输出有效。
 *
 *   Stage 2 (下一拍组合逻辑):
 *     - plant_rd_pixel / sunflower_rd_pixel / zombie_rd_pixel 这一拍才有效。
 *     - 用 _d 的命中信号和 sprite 像素做最终 mux，按层叠顺序覆盖。
 *     - 输出 color_out。注意 color_out 本身没有再寄存一拍 —— 上层
 *       pvz_top.sv 接的是 color_palette（也是组合的），最终 RGB 信号
 *       在送进 VGA 引脚前是经过 VGA_BLANK_n gate 的，所以一拍延迟
 *       和 VGA 的 hcount/vcount 时序对得上。
 *
 * ----------------------------- 几何常量 -----------------------------
 * GRID_X = 64, GRID_Y = 112, CELL = 64, GRID_COLS = 8, GRID_ROWS = 4
 * 这些必须和 hw/bg_grid.sv 以及 sw/pvz.h 里完全一致，不然
 * SW 算的格子坐标会跟 HW 画的位置错位。
 */

module entity_drawer(
    input  logic        clk,        // 系统时钟，50 MHz，和 vga_counters 共用
    input  logic        reset,      // 同步/异步复位，高有效

    // Pixel coordinates from VGA scan
    // 当前正在画的像素坐标，由 pvz_top 从 vga_counters.hcount[10:1]/vcount 取来
    input  logic [9:0]  px,
    input  logic [9:0]  py,

    // Background color for this pixel (combinational from bg_grid)
    // 来自 bg_grid 的草坪/天蓝底色索引，也是组合的，跟 (px, py) 同拍
    input  logic [7:0]  bg_color,

    // ---------- Entity registers (driven by pvz_top register file) ----------
    // 下面所有的 input 都是 pvz_top.sv 里的寄存器输出，对应 Avalon-MM 那 51 个字。
    // 详细字段定义请对照 sw/pvz.h 的"中文翻译"部分。

    // Plants: one bit per grid cell, bit (row*8+col).
    // 植物位图：32 个格子各占一个 bit。bit (row*8+col)=1 表示那格种了植物。
    // 两个位图独立（peashooter / sunflower），同一格上软件保证只点亮一个。
    input  logic [31:0] plant_present,
    input  logic [31:0] sunflower_present,

    // Currently selected plant for the top HUD box (0=pea, 1=sunflower)
    // TAB 切换选中植物：0 = peashooter，1 = sunflower。
    // 用于决定 selector HUD 里哪个图标戴上黄色描边。
    input  logic [1:0]  selected_plant,

    // Up to 8 zombies / 8 peas. Packed so we don't need unpacked-array
    // ports (more portable across synthesis tools).
    //   zombie_x[i]   = pixel x position (0..639)   width 10
    //   zombie_row[i] = grid row (0..3)             width 2
    //   zombie_alive  = bit i high if zombie i is on screen
    //
    // 为什么打包成 packed bus？因为 SystemVerilog 的 unpacked array port
    // 在 Quartus / Verilator / ModelSim 等不同工具上行为不一致，会出兼容性
    // 问题。pvz_top.sv 那边先把数组打包成一根宽 bus，本模块入口再用 generate
    // 拆回数组，最稳妥。
    input  logic [7:0]  zombie_alive,
    input  logic [79:0] zombie_x_packed,    // {zombie_x[7], ..., zombie_x[0]}
    input  logic [15:0] zombie_row_packed,  // {zombie_row[7], ..., zombie_row[0]}

    // 豌豆子弹：和 zombie 结构完全相同的 8 槽位寄存器。
    input  logic [7:0]  pea_alive,
    input  logic [79:0] pea_x_packed,
    input  logic [15:0] pea_row_packed,

    // Cursor: a hollow yellow border around one cell
    // 玩家光标：一个空心黄色方框，套在 (cursor_row, cursor_col) 那一格上。
    // 注意 cursor 用的是格子坐标，不是像素坐标 —— 跟 zombie/pea 不同。
    input  logic        cursor_visible,
    input  logic [2:0]  cursor_col,
    input  logic [1:0]  cursor_row,

    // Sun count for HUD (each block = 100 sun, up to 10 blocks)
    // 阳光数值。下面的 sun_hit_comb 块里实际用的是"每 50 点 1 块"
    // （SUN_PER_BLOCK = 50），注释里的 100 是历史值。
    input  logic [13:0] sun_value,

    // Plant sprite ROM read interface (1-cycle read latency).
    // Sunflower ROM shares the same address (issued via plant_rd_addr).
    // 植物 sprite ROM 接口：我们这一拍输出 plant_rd_addr，
    // 下一拍 plant_rd_pixel / sunflower_rd_pixel 才有效。
    // 共用同一个 addr 是因为 plant 和 sunflower 在同一个 cell 上互斥，
    // 而且它们 sprite 都是 64x64，cell 内坐标算法完全一样。
    output logic [11:0] plant_rd_addr,
    input  logic [7:0]  plant_rd_pixel,
    input  logic [7:0]  sunflower_rd_pixel,

    // Zombie sprite ROM read interface (1-cycle read latency)
    // 僵尸 sprite ROM 接口，同样 1 cycle 读延迟。
    output logic [11:0] zombie_rd_addr,
    input  logic [7:0]  zombie_rd_pixel,

    // Final pixel color (registered, 1 cycle of latency vs px/py)
    // 最终像素颜色索引。相对入口 (px, py) 有 1 拍延迟（因为内部 stage 1 -> 寄存
    // 器 -> stage 2 的流水线）。pvz_top 用它喂 color_palette 查 RGB。
    output logic [7:0]  color_out
);

    // ---------------------------------------------------------------
    // Color indices (must match color_palette.sv)
    // 颜色索引常量。这里只列出 entity_drawer 直接用到的几个：
    //   COL_YELLOW       —— 光标、HUD 边框、阳光块
    //   COL_GREEN        —— peashooter 选择框填充
    //   COL_BRIGHT_GREEN —— 豌豆子弹
    //   COL_ORANGE       —— sunflower 选择框填充
    //   COL_TRANSPARENT  —— sprite ROM 里的 0xFF 透明标记，永远不传到 palette
    // 加新颜色记得同步改 hw/color_palette.sv。
    // ---------------------------------------------------------------
    localparam logic [7:0] COL_YELLOW       = 8'd4;
    localparam logic [7:0] COL_GREEN        = 8'd7;
    localparam logic [7:0] COL_BRIGHT_GREEN = 8'd9;
    localparam logic [7:0] COL_ORANGE       = 8'd12;
    localparam logic [7:0] COL_TRANSPARENT  = 8'hFF;

    // Layout constants (mirror bg_grid.sv)
    // 网格几何，必须和 hw/bg_grid.sv 以及 sw/pvz.h 完全一致。
    localparam logic [9:0] GRID_X     = 10'd64;    // 网格左上角屏幕 x
    localparam logic [9:0] GRID_Y     = 10'd112;   // 网格左上角屏幕 y
    localparam logic [9:0] CELL       = 10'd64;    // 单格边长（也是 sprite 尺寸）

    // Entity sprite sizes
    // 实体尺寸常量：
    //   ZOMBIE_W/H：僵尸 sprite 64x64，正好和一个 cell 一样大
    //   PEA_SIZE：  豌豆子弹是 8x8 的纯色方块（不用 sprite ROM）
    //   CURSOR_BORDER：光标边框宽 4 px（外围 4 像素环带是黄色，中间空）
    localparam logic [9:0] ZOMBIE_W = 10'd64;
    localparam logic [9:0] ZOMBIE_H = 10'd64;
    localparam logic [9:0] PEA_SIZE = 10'd8;
    localparam logic [9:0] CURSOR_BORDER = 10'd4;

    // Sun HUD layout: 10 blocks across the top of the screen
    // 阳光 HUD 布局：屏幕顶部右侧一排黄色小方块，每个代表 50 阳光。
    //   SUN_X     = 起点屏幕 x 坐标
    //   SUN_Y     = 起点屏幕 y 坐标
    //   SUN_BW/H  = 单块宽 / 高
    //   SUN_PITCH = 块到块的水平间距（含 2 px 间隙）
    //   SUN_PER_BLOCK = 每点亮一块对应的阳光数
    // 注意 sun_value 14 位，最大 16383，理论上可以多于 10 * 50 = 500，
    // 那时屏幕上仍然只显示 10 块满（hit 循环只跑 i = 0..9）。
    localparam logic [9:0]  SUN_X     = 10'd440;
    localparam logic [9:0]  SUN_Y     = 10'd24;
    localparam logic [9:0]  SUN_BW    = 10'd16;  // block width
    localparam logic [9:0]  SUN_BH    = 10'd24;  // block height
    localparam logic [9:0]  SUN_PITCH = 10'd18;  // block + 2 px gap
    localparam logic [13:0] SUN_PER_BLOCK = 14'd50;

    // Plant-selector HUD: two always-visible icon boxes at top-left,
    // one per plant type.  Box 0 (peashooter) is green; box 1
    // (sunflower) is orange.  Only the currently selected box wears
    // the yellow border, so TAB visibly cycles the "cursor" between
    // the two icons.  Both boxes sit above the grid (GRID_Y=112) and
    // well clear of the sun HUD on the right side of the screen.
    //
    // 植物选择器 HUD 布局：屏幕左上角两个 48x48 的方块图标，
    // 一个绿色代表 peashooter，一个橙色代表 sunflower。两个都一直
    // 显示，按 TAB 时只是"切换黄色描边戴在哪个上"，玩家因此能直观
    // 看到当前选中哪一个、还有哪些植物可选。
    //   SEL_X0/X1 = 两个图标的左上角 x（8 / 64，中间有 8 px 间隙）
    //   SEL_Y     = 两个图标共用的左上角 y
    //   SEL_SZ    = 单个图标边长
    //   SEL_BORDER= 黄色描边宽度（和 CURSOR_BORDER 取一致，视觉协调）
    localparam logic [9:0] SEL_X0     = 10'd8;     // peashooter icon
    localparam logic [9:0] SEL_X1     = 10'd64;    // sunflower icon (8 + 48 + 8 px gap)
    localparam logic [9:0] SEL_Y      = 10'd8;
    localparam logic [9:0] SEL_SZ     = 10'd48;
    localparam logic [9:0] SEL_BORDER = 10'd4;

    // ---------------------------------------------------------------
    // Unpack the zombie/pea arrays into indexable arrays
    // 把 packed bus 拆回 8 个独立的小数组，方便后面的 for 循环
    // 用 zombie_x[i] 这种自然写法。综合后 generate 展开成 8 路并行
    // 的连线，每路都是组合直通，零延迟。
    //   zombie_x_packed[10i +: 10] 是 SystemVerilog 的"位选片段"语法，
    //   等价于 zombie_x_packed[10*i + 9 : 10*i]，但用 +: 写法和参数化
    //   循环更搭。
    // ---------------------------------------------------------------
    logic [9:0] zombie_x   [0:7];
    logic [1:0] zombie_row [0:7];
    logic [9:0] pea_x      [0:7];
    logic [1:0] pea_row    [0:7];

    genvar gi;
    generate
        for (gi = 0; gi < 8; gi++) begin : unpack_entities
            assign zombie_x[gi]   = zombie_x_packed[gi*10 +: 10];
            assign zombie_row[gi] = zombie_row_packed[gi*2 +: 2];
            assign pea_x[gi]      = pea_x_packed[gi*10 +: 10];
            assign pea_row[gi]    = pea_row_packed[gi*2 +: 2];
        end
    endgenerate

    // ---------------------------------------------------------------
    // Stage 1: figure out which grid cell the current pixel is in,
    // and issue the plant sprite ROM read for that cell.
    //
    // 这一段属于 stage 1：
    //   1) 判断当前 (px, py) 是否在 4x8 网格里；
    //   2) 算出格内坐标 (in_cell_x, in_cell_y)；
    //   3) 查植物/向日葵位图，确定要不要画 plant；
    //   4) 立即给 plant_rd_addr / sunflower_rd_addr 喂地址，让
    //      sprite ROM 在下一拍把对应像素吐出来。
    // 这一拍算 plant_here，下一拍配合 plant_rd_pixel 就能画 plant。
    // ---------------------------------------------------------------
    wire in_grid_x = (px >= GRID_X) && (px < GRID_X + 10'd512);  // [64, 576)
    wire in_grid_y = (py >= GRID_Y) && (py < GRID_Y + 10'd256);  // [112, 368)
    wire in_grid   = in_grid_x && in_grid_y;

    /* verilator lint_off UNUSED */
    // 网格内相对坐标。和 bg_grid.sv 里的 gx/gy 用法相同：
    //   高位 = 行号/列号，低 6 位 = 格内像素偏移。
    wire [9:0] gx = px - GRID_X;
    wire [9:0] gy = py - GRID_Y;
    /* verilator lint_on UNUSED */

    // Cell index and within-cell pixel (cells are 64 px = 2^6).  Sprite
    // is 64x64 so we use the full 6 bits of in_cell_{x,y} as the address.
    //   cell_col  = gx[8:6]  -> 列号 0..7（3 bit）
    //   cell_row  = gy[7:6]  -> 行号 0..3（2 bit）
    //   in_cell_x = gx[5:0]  -> 格内 x，刚好作 sprite 地址低 6 位
    //   in_cell_y = gy[5:0]  -> 格内 y，作 sprite 地址高 6 位
    // 用位切片代替乘除是这套渲染管线随处可见的招数。
    wire [2:0] cell_col  = gx[8:6];
    wire [1:0] cell_row  = gy[7:6];
    wire [5:0] in_cell_x = gx[5:0];
    wire [5:0] in_cell_y = gy[5:0];

    // Plant / sunflower bits for this cell (false if outside grid)
    // plant_idx = row*8 + col，正好等于 {cell_row, cell_col}（位拼接），
    // 因为 cell_col 占低 3 位 cell_row 占高 2 位。位拼接比乘加更省 LUT。
    // 然后从 32 位位图里直接取出对应 bit，配合 in_grid 排除超出网格的查询。
    wire [4:0] plant_idx = {cell_row, cell_col};
    wire plant_here     = in_grid && plant_present[plant_idx];
    wire sunflower_here = in_grid && sunflower_present[plant_idx];

    // Plant sprite ROM address: 1:1 mapping (64x64 ROM into 64x64 cell).
    // sprite 大小 = cell 大小，所以 addr = y*64 + x = {in_cell_y, in_cell_x}。
    // 注意我们这里没有判 plant_here 才给 addr —— 直接每个像素都喂地址，
    // ROM 会一直读数据，但 plant_here 在 stage 2 才决定到底用不用这个像素。
    // 这样写更省时序：sprite ROM 的地址路径是关键路径之一，不要再加门。
    // 注意 plant 和 sunflower 共用同一个 addr，是 pvz_top.sv 里把这个 addr
    // 同时接到 plant_rom_inst 和 sunflower_rom_inst 实现的。
    assign plant_rd_addr = {in_cell_y, in_cell_x};

    // ---------------------------------------------------------------
    // Stage 1: zombie hit detection AND zombie sprite ROM address.
    // Priority encoder: first alive zombie covering this pixel wins
    // (zombies don't normally overlap on screen).
    //
    // 这一段：遍历 8 个 zombie 槽位，看当前像素是不是落在某个 alive
    // 的 zombie sprite 矩形 [zombie_x, zombie_x+64) x [zy_top, zy_top+64)
    // 里。一旦找到第一个匹配，就锁定 zombie_in_x/y 作为 sprite ROM
    // 的格内坐标。`!zombie_hit_comb` 这个守卫让循环具有"优先编码器"
    // 的语义 —— 找到第一个就不再覆盖，索引小的 zombie 优先。
    //
    // 高层：游戏逻辑通常不会让两个 zombie 重叠在同一像素，所以"谁先
    //      谁赢"看起来无所谓；但万一吃植物时两只僵尸卡在一起，这条
    //      规则保证画面确定性，不会出现 z-fighting 闪烁。
    //
    // 低层：循环每轮都做一次 px/py 范围比较，综合后是 8 路并行比较器 +
    //      一棵 OR 树。dx/dy 是 sprite 内的偏移，因为只有 6 位有意义
    //      （64 = 2^6），直接截 dx[5:0]/dy[5:0] 即可。
    //
    // zy_top 算法：GRID_Y + row * 64。这里用 `({8'd0, zombie_row[i]} << 6)`
    // 而不是写 `zombie_row[i] * 10'd64`，是因为左移 6 == 乘 64，
    // 综合后是连线（零硬件），乘法器反而会被推断出来浪费 DSP。
    // ---------------------------------------------------------------
    logic        zombie_hit_comb;
    logic [5:0]  zombie_in_x, zombie_in_y;
    always_comb begin
        zombie_hit_comb = 1'b0;
        zombie_in_x     = 6'd0;
        zombie_in_y     = 6'd0;
        for (int i = 0; i < 8; i++) begin
            logic [9:0] zy_top, dx, dy;
            zy_top = GRID_Y + ({8'd0, zombie_row[i]} << 6);  // 该 zombie 所在行的屏幕 y
            dx     = px - zombie_x[i];                       // 当前像素在 zombie sprite 内的 x
            dy     = py - zy_top;                            // 当前像素在 zombie sprite 内的 y
            // alive 且本拍还没命中 && 像素落在 sprite 矩形内 -> 锁定
            if (zombie_alive[i] && !zombie_hit_comb &&
                px >= zombie_x[i] && px < zombie_x[i] + ZOMBIE_W &&
                py >= zy_top      && py < zy_top      + ZOMBIE_H)
            begin
                zombie_hit_comb = 1'b1;
                zombie_in_x     = dx[5:0];
                zombie_in_y     = dy[5:0];
            end
        end
    end
    // 同 plant 一样直接拼地址。即使 zombie_hit_comb=0 也照常送地址，
    // stage 2 再用 zombie_hit_d 决定要不要用这个像素。
    assign zombie_rd_addr = {zombie_in_y, zombie_in_x};

    // ---------------------------------------------------------------
    // Stage 1: combinational hit detection for non-sprite entities
    // 下面几个 hit 检测都不需要 sprite ROM，直接组合算出来即可。
    // 因为它们没有 sprite 内坐标，也就不存在 1 cycle 读延迟问题，
    // 但仍然要寄存一拍才能和 stage 2 的 sprite pixel 对齐。
    // ---------------------------------------------------------------

    // 豌豆子弹命中：在 pea 那一行垂直居中 8x8 像素方块。
    //   py_top = 该行屏幕 y + 28，所以 pea 占 py ∈ [row*64+112+28, +36)，
    //   也就是 cell 中线附近 8 px 的范围，看上去像"豌豆飞行轨迹"。
    // 注意 8 个 pea 不做优先编码：只要任一个 alive 且覆盖该像素就命中，
    // 反正最终颜色都是 COL_BRIGHT_GREEN，不需要区分是哪颗豌豆。
    logic pea_hit_comb;
    always_comb begin
        pea_hit_comb = 1'b0;
        for (int i = 0; i < 8; i++) begin
            // Center the pea vertically in its row (row*64 + 28..36)
            logic [9:0] py_top;
            py_top = GRID_Y + ({8'd0, pea_row[i]} << 6) + 10'd28;
            if (pea_alive[i] &&
                px >= pea_x[i] && px < pea_x[i] + PEA_SIZE &&
                py >= py_top && py < py_top + PEA_SIZE)
                pea_hit_comb = 1'b1;
        end
    end

    // Cursor: hollow border around the cursor cell
    // 玩家光标命中：在 cursor 那一格里，且距离任一边不超过 CURSOR_BORDER (4) 像素。
    // 这样画出来是个 64x64 的空心方框，中间留空让玩家能看到底下的草坪/植物。
    //
    // 算法：
    //   - 先用 cur_left/cur_top 算出 cursor 那一格的屏幕左上角；
    //   - 再判断 (px, py) 是否在格内（外层 if）；
    //   - 在格内的话，看是不是落在边框带上：左边距 (px - cur_left)、
    //     右边距 (cur_left + CELL - px)、上下同理 —— 任一个 < BORDER 就是边框。
    //   - 注意右/下用 <= BORDER 而左/上用 < BORDER，这是因为边距是
    //     "未来还有几像素到边"，右边那条线本身要算成边框（off-by-one 微调）。
    //
    // cursor_visible 是软件控制的：游戏失败或暂停画面时藏起光标。
    logic cursor_hit_comb;
    always_comb begin
        logic [9:0] cur_left, cur_top;
        cur_left = GRID_X + ({7'd0, cursor_col} << 6);  // col*64+64
        cur_top  = GRID_Y + ({8'd0, cursor_row} << 6);  // row*64+112
        cursor_hit_comb = 1'b0;
        if (cursor_visible &&
            px >= cur_left && px < cur_left + CELL &&
            py >= cur_top  && py < cur_top  + CELL) begin
            // On border? (within CURSOR_BORDER pixels of any edge)
            if ( (px - cur_left)        < CURSOR_BORDER ||
                 (cur_left + CELL - px) <= CURSOR_BORDER ||
                 (py - cur_top)         < CURSOR_BORDER ||
                 (cur_top  + CELL - py) <= CURSOR_BORDER )
                cursor_hit_comb = 1'b1;
        end
    end

    // Plant selector HUD: two icon boxes, one per plant type.  Both
    // fills are always drawn so the player can see the available
    // plants at a glance; the border is gated by selected_plant
    // later in the mux so only the chosen box looks like a cursor.
    //   selN_hit_comb    = anywhere inside box N (fill region)
    //   selN_border_comb = on box N's border (yellow cursor look)
    //
    // 这一块和 cursor 的算法基本一样，但有两个独立的盒子：
    //   - sel0 = peashooter（绿色填充）
    //   - sel1 = sunflower（橙色填充）
    // selN_hit_comb 标记"像素在这个盒子内部（包括边框区）"，
    // selN_border_comb 标记"像素恰好在 SEL_BORDER 那条黄色边带上"。
    // 这两个信号会一起进 stage 2 寄存器，最终 mux 才决定：
    //   1) 填充色总是画 -> 两个盒子永远可见；
    //   2) 黄色描边只在 selected_plant 对应的那盒上画 -> TAB 一按
    //      就让黄边在两盒之间跳。
    // 把"画填充"和"画描边"在 stage 1 全算好，能让 stage 2 的最终 mux
    // 维持简单的优先级层叠形式。
    logic sel0_hit_comb, sel0_border_comb;
    logic sel1_hit_comb, sel1_border_comb;
    always_comb begin
        // Box 0 — peashooter
        sel0_hit_comb = (px >= SEL_X0 && px < SEL_X0 + SEL_SZ &&
                         py >= SEL_Y  && py < SEL_Y  + SEL_SZ);
        sel0_border_comb = 1'b0;
        if (sel0_hit_comb) begin
            // 同 cursor 边框检测：四个边距任一个落在 BORDER 内就算边框。
            if ((px - SEL_X0)           < SEL_BORDER ||
                (SEL_X0 + SEL_SZ - px) <= SEL_BORDER ||
                (py - SEL_Y)            < SEL_BORDER ||
                (SEL_Y + SEL_SZ - py)  <= SEL_BORDER)
                sel0_border_comb = 1'b1;
        end

        // Box 1 — sunflower
        sel1_hit_comb = (px >= SEL_X1 && px < SEL_X1 + SEL_SZ &&
                         py >= SEL_Y  && py < SEL_Y  + SEL_SZ);
        sel1_border_comb = 1'b0;
        if (sel1_hit_comb) begin
            if ((px - SEL_X1)           < SEL_BORDER ||
                (SEL_X1 + SEL_SZ - px) <= SEL_BORDER ||
                (py - SEL_Y)            < SEL_BORDER ||
                (SEL_Y + SEL_SZ - py)  <= SEL_BORDER)
                sel1_border_comb = 1'b1;
        end
    end

    // Sun HUD: 10 yellow blocks across the top.  Block i is lit when
    // sun_value >= (i+1)*100.  Loop is unrolled at synthesis.
    //
    // 阳光 HUD 命中检测：
    //   - 先用 py 的范围过滤，HUD 只在屏幕顶部那一条 SUN_BH 像素高的区域；
    //   - 然后展开 10 个块，块 i 的左上角 bx = SUN_X + i * SUN_PITCH；
    //   - 块 i 点亮的条件 = sun_value >= (i+1)*50（即至少 (i+1)*50 阳光）；
    //   - 像素若同时落在 bx + [0, SUN_BW) 内且该块点亮，sun_hit_comb=1。
    //
    // 注释里说"(i+1)*100"是历史值，代码现在实际用 50（SUN_PER_BLOCK），
    // 表示每多 50 阳光就点亮一块，便于在屏幕上看见进度。
    //
    // 综合时 SystemVerilog 的 for 循环会被完全展开成 10 路并行电路，
    // 没有 runtime overhead，所以这里可以放心写循环。
    //
    // 14'((i+1)*50) 是显式宽度类型转换，避免 32-bit 常量和 14-bit
    // sun_value 比较时产生 Quartus 警告。
    logic sun_hit_comb;
    always_comb begin
        sun_hit_comb = 1'b0;
        if (py >= SUN_Y && py < SUN_Y + SUN_BH) begin
            for (int i = 0; i < 10; i++) begin
                logic [9:0] bx;
                bx = SUN_X + 10'(i) * SUN_PITCH;
                if (sun_value >= 14'((i+1) * 50) &&
                    px >= bx && px < bx + SUN_BW)
                    sun_hit_comb = 1'b1;
            end
        end
    end

    // ---------------------------------------------------------------
    // Stage 2: register everything to align with the 1-cycle sprite
    // ROM read latency.  Then mux to produce final color.
    //
    // 流水线寄存器：把 stage 1 算出来的所有 hit/背景色锁存一拍。
    // 这一拍过去之后，sprite ROM 的 plant_rd_pixel / sunflower_rd_pixel /
    // zombie_rd_pixel 才有效，可以和这里 _d 后缀的寄存器对齐做最终 mux。
    //
    // 高层：这就是经典的"两段组合 + 中间一级寄存器"的硬件渲染流水线。
    //      因为 sprite ROM 是同步 BRAM（M10K），它的输出本身就比 addr
    //      晚一拍，所以我们只需要把"其它"信号也延一拍，时序就对得上了。
    //
    // 低层：所有寄存器都用同步 reset 清零 —— 复位释放后保证第一帧是
    //      干净背景，不会闪一帧"上一帧残留"。selected_plant 也要延一拍
    //      （-> selected_plant_d），否则会比 sel*_border_d 早一拍判断，
    //      画出来 TAB 按下时会有一帧"两个盒子同时戴边"的鬼影。
    // ---------------------------------------------------------------
    logic [7:0] bg_color_d;
    logic       plant_here_d;
    logic       sunflower_here_d;
    logic       zombie_hit_d;
    logic       pea_hit_d;
    logic       cursor_hit_d;
    logic       sun_hit_d;
    logic       sel0_hit_d, sel0_border_d;
    logic       sel1_hit_d, sel1_border_d;
    logic [1:0] selected_plant_d;

    always_ff @(posedge clk or posedge reset) begin
        if (reset) begin
            // 复位：所有 hit 清零，背景色置 0（黑），避免复位释放瞬间
            // 输出未定义值导致显示器抖动。
            bg_color_d       <= 8'd0;
            plant_here_d     <= 1'b0;
            sunflower_here_d <= 1'b0;
            zombie_hit_d     <= 1'b0;
            pea_hit_d        <= 1'b0;
            cursor_hit_d     <= 1'b0;
            sun_hit_d        <= 1'b0;
            sel0_hit_d       <= 1'b0;
            sel0_border_d    <= 1'b0;
            sel1_hit_d       <= 1'b0;
            sel1_border_d    <= 1'b0;
            selected_plant_d <= 2'd0;
        end else begin
            // 正常工作：每个时钟把组合输出搬到 _d 寄存器，下一拍 stage 2 用。
            bg_color_d       <= bg_color;
            plant_here_d     <= plant_here;
            sunflower_here_d <= sunflower_here;
            zombie_hit_d     <= zombie_hit_comb;
            pea_hit_d        <= pea_hit_comb;
            cursor_hit_d     <= cursor_hit_comb;
            sun_hit_d        <= sun_hit_comb;
            sel0_hit_d       <= sel0_hit_comb;
            sel0_border_d    <= sel0_border_comb;
            sel1_hit_d       <= sel1_hit_comb;
            sel1_border_d    <= sel1_border_comb;
            selected_plant_d <= selected_plant;
        end
    end

    // Final mux: paint layers from bottom to top.  Sprite pixels are
    // valid this cycle (issued from address registered last cycle by
    // the sprite ROM, which has 1-cycle latency).
    //
    // ============================== STAGE 2 ==============================
    // 这是整个文件的最终输出点：按"从下往上"顺序覆盖每一层。
    // 注意每条 if 都是独立的 —— 后面的赋值会自然覆盖前面的，这就是
    // SystemVerilog 组合 always 里"最后赋值胜出"的特性，刚好对应层叠顺序。
    //
    // 这里 plant_rd_pixel / sunflower_rd_pixel / zombie_rd_pixel 都是
    // 来自 hw/sprite_rom.sv 的同步 BRAM 输出 —— 上一拍我们设了
    // {plant,zombie}_rd_addr，这一拍 BRAM 就把对应字节吐出来。所以
    // 它们和 stage 2 自然对齐 1 拍读延迟，正好和 _d 寄存器拍号相同。
    //
    // 关键约定 COL_TRANSPARENT (0xFF)：sprite 数据里背景透明像素都用 0xFF，
    // 这里显式过滤，0xFF 永远不会走到 color_palette，所以 palette LUT
    // 不用考虑这个值（其 default 是黑色保底也行）。
    always_comb begin
        // 层 1：底色（草坪 / 天蓝）
        color_out = bg_color_d;

        // 层 2：植物 sprite（peashooter / sunflower）
        // plant_here_d / sunflower_here_d 是上一拍 plant_present 位图查询的结果，
        // 用来开关植物层；像素里的 0xFF 透明再额外过一层守卫。
        if (plant_here_d && plant_rd_pixel != COL_TRANSPARENT)
            color_out = plant_rd_pixel;
        if (sunflower_here_d && sunflower_rd_pixel != COL_TRANSPARENT)
            color_out = sunflower_rd_pixel;

        // 层 3：豌豆子弹 —— 纯亮绿色 8x8 方块，没有 sprite。
        if (pea_hit_d)
            color_out = COL_BRIGHT_GREEN;

        // 层 4：僵尸 sprite —— 同样过透明色，画在豌豆之上。
        // 注意僵尸比豌豆"更上层"是为了视觉上让豌豆显得是"穿进僵尸身体"，
        // 而不是停在身体前面。
        if (zombie_hit_d && zombie_rd_pixel != COL_TRANSPARENT)
            color_out = zombie_rd_pixel;

        // 层 5：玩家光标 —— 黄色边框，画在所有游戏对象之上。
        if (cursor_hit_d)
            color_out = COL_YELLOW;

        // 层 6：阳光 HUD —— 黄色块，在 cursor 之上（因为 cursor 在格子里，
        // sun HUD 在顶部，正常情况不会重叠，但顺序仍然定义清楚）。
        if (sun_hit_d)
            color_out = COL_YELLOW;

        // Selector fills: always show both plants so the player can
        // see what's on offer.  Box 0 = peashooter (green), box 1 =
        // sunflower (orange).
        // 层 7-a：选择器填充。两个图标都画填充色，让玩家随时看到"我有这些可选"。
        if (sel0_hit_d)
            color_out = COL_GREEN;
        if (sel1_hit_d)
            color_out = COL_ORANGE;

        // Selector border: only the chosen box wears the yellow
        // outline.  TAB flips selected_plant in software, which
        // moves the border from one box to the other.
        // 层 7-b：选中那盒的黄色描边。TAB 一按 -> selected_plant 翻转
        // -> 这里的 selected_plant_d 跟着翻 -> 黄边瞬间从一盒跳到另一盒。
        // 描边永远画在填充之上，所以即使 SEL_BORDER 把填充压住一圈也没关系。
        if (sel0_border_d && selected_plant_d == 2'd0)
            color_out = COL_YELLOW;
        if (sel1_border_d && selected_plant_d == 2'd1)
            color_out = COL_YELLOW;
    end

endmodule
