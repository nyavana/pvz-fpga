/*
 * PvZ GPU — top-level Avalon-MM peripheral
 *
 * Wires together VGA timing, background grid, sprite ROM, and the
 * entity drawer.  Holds the entity register file that software writes
 * via Avalon.
 *
 * Register map (32-bit words, byte offset = 4 * word index):
 *   word  0       PVZ_PLANTS    32 bits, bit i = peashooter at cell i
 *                                (i = row*8 + col, row in 0..3, col in 0..7)
 *   word  1       PVZ_SUNFLOWER 32 bits, bit i = sunflower at cell i
 *   word 32..39  PVZ_ZOMBIE[i]  bit 31 = alive
 *                                bits [9:0]   = x_pixel (0..639)
 *                                bits [11:10] = row (0..3)
 *   word 40..47  PVZ_PEA[i]     same encoding as zombie
 *   word 48      PVZ_CURSOR     bit 31 = visible
 *                                bits [4:2] = col (0..7)
 *                                bits [1:0] = row (0..3)
 *   word 49      PVZ_SUN        bits [13:0] = sun value
 *   word 50      PVZ_SELECTED   bits [1:0]  = selected plant (0=pea, 1=sunflower)
 *
 * Avalon notes:
 *   - The address port is in WORDS (qsys addressUnits = WORDS) so a
 *     CPU byte offset N maps to address = N >> 2.
 *   - Writes take effect on the next clock; no commit handshake.
 *   - There is no vsync latching, so a write that races the scan can
 *     produce one frame of tearing.  Acceptable for ~60 Hz game state.
 *
 * ===================================================================
 * 中文总览 —— 这是 FPGA 这边整个 PvZ 渲染管线的"顶层"。
 * ===================================================================
 *
 * 这个模块在 Platform Designer (Qsys) 里被实例化为一个 Avalon-MM 从设备
 * (slave/agent)，挂在 Lightweight HPS-to-FPGA bridge 上。ARM 这边的
 * sw/pvz_driver.c 用 iowrite32 把寄存器写下来 —— 走的路就是：
 *   sw/render.c
 *      -> ioctl(/dev/pvz, PVZ_WRITE_REG, {word, value})
 *      -> sw/pvz_driver.c 的 pvz_ioctl
 *      -> iowrite32(value, virtbase + word*4)
 *      -> Avalon-MM bridge
 *      -> 本文件 (pvz_top.sv) 下面那个 always_ff 解码
 *      -> hw/entity_drawer.sv 每个像素读寄存器决定颜色
 *      -> color_palette -> VGA_R/G/B 引脚
 *
 * 这个模块自己做两件事：
 *   1) 维护一份 51 个字的"实体寄存器文件" —— 见上面 Register map。
 *   2) 把渲染流水线的各个零件 (vga_counters / bg_grid / sprite_rom×3
 *      / entity_drawer / color_palette) 接起来，最终输出 VGA 信号。
 *
 * 寄存器表换成"中文人话"版本：
 *   word 0  豌豆射手 (peashooter) 的位图：32 bit 对应 4 行 x 8 列 = 32 格，
 *           bit i = 1 表示位置 (row = i/8, col = i%8) 上有一棵豌豆。
 *           例如 (row=1, col=3) 种豌豆 -> bit 11 置 1。
 *   word 1  向日葵 (sunflower) 位图，编码同上但独立存。
 *   word 32..39  8 个僵尸槽位，每个 32-bit 字 = {alive[31], _, row[11:10], x[9:0]}。
 *                注意 y 坐标不存，硬件按 row*64+112 自己算。
 *   word 40..47  8 个豌豆子弹 (pea) 槽位，编码和僵尸完全一样。
 *   word 48  cursor (玩家黄色光标)：{visible[31], _, col[4:2], row[1:0]}。
 *            注意 cursor 用格子坐标 (col,row)，不是像素 x。
 *   word 49  sun_value：当前阳光值，14 位。
 *   word 50  selected_plant：当前选中的植物类型 (0=pea, 1=sunflower)，
 *            决定顶上 HUD 那个黄框落在哪个植物图标上。
 *
 * Avalon 几个要点（容易踩坑）：
 *   - address 是 WORDS 不是 bytes！这是在 hw/pvz_top_hw.tcl 里
 *     配置 addressUnits = WORDS 决定的。CPU 那边写字节偏移 N
 *     (= word*4)，bridge 自动右移两位变成 address = word，
 *     所以这里直接 (address == 6'd0..50) 就能匹配。
 *   - 没有 readdata：本外设是只写的，HPS 不读寄存器。
 *     真要读，得改 hw/pvz_top_hw.tcl 把 read 加上。
 *   - 没有 vsync 同步：写下去那一拍就生效，可能在扫描中途换状态
 *     导致一帧 tearing。60 Hz 游戏这种延迟看不出来，所以接受。
 *
 * 跨文件引用速查：
 *   - 寄存器表也写在 sw/pvz.h（PVZ_REG_* 常量），两边必须同步。
 *   - 寄存器布局的"消费者"是 hw/entity_drawer.sv。
 *   - sub-module 实例化的来源：
 *       hw/vga_counters.sv  -> 产生 hcount/vcount + VGA_HS/VS/CLK
 *       hw/bg_grid.sv       -> 算背景草地颜色
 *       hw/sprite_rom.sv    -> 64x64 调色板贴图 ROM（3 份：豌豆/向日葵/僵尸）
 *       hw/entity_drawer.sv -> 每个像素决定最终 8-bit color index
 *       hw/color_palette.sv -> color index -> 24-bit RGB
 */

module pvz_top(
    // clk / reset 来自 Platform Designer 内部 clock/reset 网络，
    // 实际上就是板子上的 50 MHz CLOCK_50 引脚 + 系统 reset。
    // 所有寄存器和子模块都用这同一个域，单时钟设计，省事。
    input  logic        clk,
    input  logic        reset,

    // Avalon-MM slave
    // ---- Avalon-MM 从设备（slave/agent）接口 ----
    // 高层：这是 HPS（ARM）写寄存器进来的入口。Platform Designer
    //      在 Qsys 里把这几个口自动接到 Lightweight HPS-to-FPGA bridge。
    // 低层：
    //   address    寄存器索引（WORDS！见 hw/pvz_top_hw.tcl
    //              的 addressUnits=WORDS），6 位够覆盖 word 0..50。
    //   writedata  CPU 这一拍要写下来的 32 位数据。
    //   write      写使能，高有效。
    //   chipselect Avalon 标准信号，bridge 认为本外设被选中时拉高。
    //              我们的写解码用 (chipselect && write) 才生效。
    // 注意没有 readdata / read：本外设只写不读。
    input  logic [5:0]  address,    // word index, 0..49 used, 64 max
    input  logic [31:0] writedata,
    input  logic        write,
    input  logic        chipselect,

    // VGA output
    // ---- VGA 输出（全部走 conduit 接到板子 ADV7123 DAC 引脚）----
    // 这一组信号在 hw/pvz_top_hw.tcl 里被声明为 conduit 端口，
    // 然后在 hw/soc_system_top.sv 里被直接接到 VGA_R/G/B/HS/VS/CLK/BLANK_N/SYNC_N
    // 板上引脚 -> 跑去 DE1-SoC 板载 DAC -> 显示器。
    //   VGA_R/G/B    每通道 8 位 (24-bit RGB，但 DAC 只有 8-bit/通道)
    //   VGA_CLK      25 MHz 像素时钟，喂给 DAC 的 PCLK 输入
    //   VGA_HS/VS    行/场同步脉冲（active-low）
    //   VGA_BLANK_n  消隐期为 0：必须在 blanking 时输出黑色，否则 DAC
    //                会把同步打到亮度上，画面会变形。
    //   VGA_SYNC_n   复合同步，未用（恒 0）。
    output logic [7:0]  VGA_R, VGA_G, VGA_B,
    output logic        VGA_CLK, VGA_HS, VGA_VS,
    output logic        VGA_BLANK_n,
    output logic        VGA_SYNC_n
);

    // ---------------------------------------------------------------
    // VGA timing
    // ---------------------------------------------------------------
    // 高层：实例化 hw/vga_counters.sv，吃 50 MHz 时钟，吐出标准
    //      640x480@60Hz 的 hcount/vcount 以及 VGA 的同步/消隐/像素时钟。
    //      这些信号是整个 racing-the-beam 渲染管线的"时间基准"。
    // 低层：hcount 是 11 位是因为它按 50 MHz cycle 计数 (0..1599 一行)，
    //      最低位 hcount[0] 就是 25 MHz 的像素时钟。所以 px = hcount[10:1]
    //      (砍掉最低位) 把 50 MHz 计数缩成 25 MHz 像素索引，得到 0..639 像素列。
    //      vcount 已经是按行计数 (0..524)，直接当 py 用。
    //      VGA_HS / VGA_VS / VGA_BLANK_n / VGA_SYNC_n / VGA_CLK 都从这个
    //      子模块直出，本文件不动它们，直接绑到顶层的 VGA 输出口。
    logic [10:0] hcount;
    logic [9:0]  vcount;

    vga_counters counters(
        .clk50      (clk),
        .reset      (reset),
        .hcount     (hcount),
        .vcount     (vcount),
        .VGA_CLK    (VGA_CLK),
        .VGA_HS     (VGA_HS),
        .VGA_VS     (VGA_VS),
        .VGA_BLANK_n(VGA_BLANK_n),
        .VGA_SYNC_n (VGA_SYNC_n)
    );

    // px, py = 当前正在扫描的像素坐标，作为整个下游 (bg_grid /
    // entity_drawer) 的共享地址。所有像素级判断都是 "if (px,py) 落在
    // 某个矩形内 -> 显示某颜色" 这种组合逻辑形式。
    wire [9:0] px = hcount[10:1];
    wire [9:0] py = vcount;

    // ---------------------------------------------------------------
    // Entity register file
    // ---------------------------------------------------------------
    // 这一段就是"被 HPS 写、被 entity_drawer 读"的那个寄存器组的存储声明。
    // 每个变量对应 sw/pvz.h 里的某一个 PVZ_REG_* 索引。下面 always_ff 是写
    // 解码，把 writedata 的位段切到这里来。
    // Plants: one bit per grid cell (32 cells)
    // 4 行 x 8 列 = 32 格，所以一整张地图正好塞进一个 32-bit 字。
    // bit i 对应格子 (row = i/8, col = i%8)。两种植物各一张图，互相独立。
    logic [31:0] plant_present;
    logic [31:0] sunflower_present;

    // Currently selected plant type for the top HUD box (0=pea, 1=sunflower)
    // 玩家按 TAB 切换选中的植物类型，软件把这个值写下来，entity_drawer
    // 根据它决定把"黄色光标边框"画在左上角的哪个植物图标上。
    logic [1:0]  selected_plant;

    // Zombies and peas: 8 each.  Alive bits packed; x and row also
    // packed into wide buses for handing to entity_drawer.
    // 8 个僵尸 + 8 个豌豆。alive 位被打成 8-bit 向量方便整体复位 /
    // 切片，x 和 row 用 SystemVerilog 的"unpacked array"声明，
    // 后面再用 generate 块打成连续的位串传给 entity_drawer（因为
    // 不是所有综合工具都支持 unpacked array 端口）。
    logic [7:0]  zombie_alive, pea_alive;
    logic [9:0]  zombie_x   [0:7];
    logic [1:0]  zombie_row [0:7];
    logic [9:0]  pea_x      [0:7];
    logic [1:0]  pea_row    [0:7];

    // Cursor + sun
    // cursor_visible：游戏失败/胜利画面时设 0 把光标藏起来。
    // cursor_col/row：3 位+2 位，正好 8 列 / 4 行。
    // sun_value：14 位最多 16383。当前 v5 还没有数字 HUD，
    //            但寄存器先留好，将来加 7-seg 显示直接读这位。
    logic        cursor_visible;
    logic [2:0]  cursor_col;
    logic [1:0]  cursor_row;
    logic [13:0] sun_value;

    // ---------------------------------------------------------------
    // Avalon-MM write decode
    // ---------------------------------------------------------------
    // 高层：这是整个文件最关键的一段 —— "HPS 写下来一个 32 位字，
    //      根据 address 把它的位段塞到对应寄存器里"。
    //      触发条件 = posedge clk && chipselect && write，每拍最多写一个寄存器。
    //
    // 注意三件事：
    //   1) address 单位是 WORDS (hw/pvz_top_hw.tcl 里 addressUnits=WORDS)，
    //      所以这里直接和常量 6'd0..6'd50 比较即可，不用乘 4。
    //   2) 异步高有效复位：reset 一来，所有寄存器全部清零。
    //      上电 / kernel module 刚加载时整个画面干净，没有残留状态。
    //   3) zombie/pea 用 address[2:0] 选 8 个槽位里的哪一个 —— 因为
    //      32..39 这 8 个地址的低 3 位正好覆盖 000..111。位切片省了一个 case 表。
    always_ff @(posedge clk or posedge reset) begin
        if (reset) begin
            // ---- 同步复位：把所有寄存器拉回安全的"空"状态 ----
            // 这样上电后即便 HPS 还没写任何寄存器，画面也是干净的空网格。
            plant_present     <= 32'd0;
            sunflower_present <= 32'd0;
            selected_plant    <= 2'd0;
            zombie_alive   <= 8'd0;
            pea_alive      <= 8'd0;
            cursor_visible <= 1'b0;
            cursor_col     <= 3'd0;
            cursor_row     <= 2'd0;
            sun_value      <= 14'd0;
            for (int i = 0; i < 8; i++) begin
                zombie_x[i]   <= 10'd0;
                zombie_row[i] <= 2'd0;
                pea_x[i]      <= 10'd0;
                pea_row[i]    <= 2'd0;
            end
        end else if (chipselect && write) begin
            // chipselect && write 同时高才算一次有效写。Avalon-MM 标准要求。
            // Peashooter bitmap: word 0
            // word 0 —— 把整个 32-bit writedata 直接当成"豌豆射手位图"存。
            // 软件那边 sw/render.c 每帧重新组装这张图发下来。
            if (address == 6'd0) begin
                plant_present <= writedata;
            end
            // Sunflower bitmap: word 1
            // word 1 —— 向日葵位图，同上。
            else if (address == 6'd1) begin
                sunflower_present <= writedata;
            end
            // Zombies: word 32..39
            // word 32..39 —— 8 个僵尸槽位。address[2:0] 取低 3 位选槽位 0..7。
            // writedata 位布局 = {alive[31], _, _, _, _, _, _, _, _, _,
            //                      _, _, _, _, _, _, _, _, _, _,
            //                      row[11:10], x_pixel[9:0]} —— 和
            // sw/pvz.h 里的 pvz_pack_entity() 函数严格对应。
            else if (address < 6'd40) begin
                zombie_alive[address[2:0]] <= writedata[31];
                zombie_x[address[2:0]]     <= writedata[9:0];
                zombie_row[address[2:0]]   <= writedata[11:10];
            end
            // Peas: word 40..47
            // word 40..47 —— 8 个豌豆子弹槽位，编码格式和僵尸完全一致。
            // 重复套用同一套切片逻辑，简化解码。
            else if (address < 6'd48) begin
                pea_alive[address[2:0]] <= writedata[31];
                pea_x[address[2:0]]     <= writedata[9:0];
                pea_row[address[2:0]]   <= writedata[11:10];
            end
            // Cursor: word 48
            // word 48 —— 光标。注意 col/row 用的是格子坐标不是像素，
            // 位段位置和僵尸不一样（col 在 [4:2]，row 在 [1:0]）。
            // 这一段必须和 sw/pvz.h 的 pvz_pack_cursor() 对得上。
            else if (address == 6'd48) begin
                cursor_visible <= writedata[31];
                cursor_col     <= writedata[4:2];
                cursor_row     <= writedata[1:0];
            end
            // Sun: word 49
            // word 49 —— 阳光值，软件只用低 14 位。HW 现在没画数字 HUD，
            // 但 entity_drawer 用它点亮顶上的黄色块 (每块代表 50 sun)。
            else if (address == 6'd49) begin
                sun_value <= writedata[13:0];
            end
            // Selected plant: word 50
            // word 50 —— 当前选中的植物类型，低 2 位即可。entity_drawer
            // 在最终 mux 时用它决定把黄色光标边框画在哪个植物图标上。
            else if (address == 6'd50) begin
                selected_plant <= writedata[1:0];
            end
        end
    end

    // Pack arrays into wide buses for the drawer module
    // 把上面 unpacked array (例如 zombie_x[0:7]) 拉平成连续位串
    // (zombie_x_packed[79:0])，方便作为 module 端口传给 entity_drawer。
    // 为什么这么干：SystemVerilog 标准允许 unpacked array 当端口，
    // 但部分综合工具（特别是 Quartus 老版本）对它支持不好，会报错
    // 或者综出来很丑。打成 packed bus 是最保险的写法。
    //
    // 位段约定 (8 个槽位)：
    //   zombie_x_packed   = { zombie_x[7], zombie_x[6], ..., zombie_x[0] }
    //                       每个 10 位 -> 总宽 80
    //   zombie_row_packed = { zombie_row[7], ..., zombie_row[0] }
    //                       每个 2 位  -> 总宽 16
    // entity_drawer.sv 里有镜像的 unpack 代码 (unpack_entities generate)，
    // 把 packed bus 还原回 unpacked array 使用。
    logic [79:0] zombie_x_packed, pea_x_packed;
    logic [15:0] zombie_row_packed, pea_row_packed;
    genvar gi;
    generate
        for (gi = 0; gi < 8; gi++) begin : pack_entities
            assign zombie_x_packed[gi*10 +: 10]    = zombie_x[gi];
            assign zombie_row_packed[gi*2 +: 2]    = zombie_row[gi];
            assign pea_x_packed[gi*10 +: 10]       = pea_x[gi];
            assign pea_row_packed[gi*2 +: 2]       = pea_row[gi];
        end
    endgenerate

    // ---------------------------------------------------------------
    // Background grid (combinational, no state)
    // ---------------------------------------------------------------
    // 高层：实例化 hw/bg_grid.sv，纯组合逻辑的小模块，输入当前像素 (px, py)，
    //      输出 8-bit 背景颜色 index。
    // 低层：bg_grid 内部根据 (px, py) 是否落在游戏网格区域里返回：
    //         - 在区域外：天蓝色 (index 13)
    //         - 在区域内：明暗交替的两种绿色（草地 checker，用
    //           (col+row) 奇偶性切换）
    //      它就是个 LUT，没有寄存器，跟着 (px, py) 一拍一拍变。
    //      下游 entity_drawer 把这个 bg_color 当成"最底层 (z-order 1)"，
    //      上面再叠植物 / 豌豆 / 僵尸 / 光标 / HUD。
    logic [7:0] bg_color;
    bg_grid bg_inst(
        .px       (px),
        .py       (py),
        .color_out(bg_color)
    );

    // ---------------------------------------------------------------
    // Sprite ROMs (64x64 each, 4096 bytes, 1-cycle read latency).
    // One ROM per sprite type; both share the same module.
    // ---------------------------------------------------------------
    // 三个 sprite_rom 实例，全是同一个 hw/sprite_rom.sv 模块，
    // 通过 parameter MEM_FILE 加载不同 .mem 文件（在 Quartus 综合时
    // $readmemh 读进 M10K 块 RAM）。每个 ROM 4096 字节 = 64x64 像素 x 1 字节
    // (调色板索引)，读延迟固定 1 拍。
    //
    // 地址 = {in_cell_y[5:0], in_cell_x[5:0]} = y*64+x，从 entity_drawer
    // 那边算出来送过来。pixel 是 8-bit 调色板索引，0xFF 表示透明
    // (entity_drawer 会跳过透明像素以保留下面一层)。
    logic [11:0] plant_addr;
    logic [7:0]  plant_pixel;
    sprite_rom #(.MEM_FILE("peashooter_idx.mem")) plant_rom_inst(
        .clk  (clk),
        .addr (plant_addr),
        .pixel(plant_pixel)
    );

    // Sunflower ROM shares the cell-local address with the peashooter ROM
    // 关键技巧：向日葵 ROM 直接挂在 plant_addr 上 —— 因为两种植物
    // 在同一格里位置是完全一样的 (64x64 居中)，所以共用 cell-local 地址。
    // 当 entity_drawer 决定这个像素属于哪种植物时，再在 mux 阶段选择
    // 是用 plant_pixel 还是 sunflower_pixel。比给两个 ROM 各算一份
    // 地址省了一组加法器和位运算。
    logic [7:0]  sunflower_pixel;
    sprite_rom #(.MEM_FILE("sunflower_idx.mem")) sunflower_rom_inst(
        .clk  (clk),
        .addr (plant_addr),
        .pixel(sunflower_pixel)
    );

    // 僵尸 ROM 独立编址 (zombie_addr)，因为僵尸可以出现在任何 x 坐标，
    // entity_drawer 要根据 (px - zombie_x[i]) 算偏移找出当前像素是僵尸
    // 贴图里的第几个像素 —— 跟植物的 cell-local 地址完全是两套算法。
    logic [11:0] zombie_addr;
    logic [7:0]  zombie_pixel;
    sprite_rom #(.MEM_FILE("zombie_idx.mem")) zombie_rom_inst(
        .clk  (clk),
        .addr (zombie_addr),
        .pixel(zombie_pixel)
    );

    // ---------------------------------------------------------------
    // Entity drawer: produces the final pixel color
    // ---------------------------------------------------------------
    // 高层：这是整个渲染管线的"大脑" —— hw/entity_drawer.sv。
    //      它收下当前像素坐标 (px, py)、bg_grid 给的背景色、所有
    //      实体寄存器状态、还有三个 sprite ROM 的读接口；
    //      做一堆"这个像素是否落在某个实体的矩形/贴图里"的命中检测，
    //      按 z-order 从下到上叠色，最终输出 8-bit 调色板 index。
    //
    // 关键点：sprite ROM 有 1 拍读延迟，所以 entity_drawer 内部把所有
    //      hit 标志先寄存一拍 (always_ff)，再和这一拍才到的 ROM pixel
    //      一起做最终 mux。因此 color_out 相对 (px, py) 有 1 拍延迟，
    //      但下游 color_palette 是组合的，最终 VGA 输出也跟着延 1 拍。
    //      这点延迟在 VGA 整体的水平 800 cycle 周期里完全可以忽略。
    //
    // 端口对应：plant_present / sunflower_present / selected_plant 等
    //      就是上面 always_ff 里我们维护的寄存器；packed bus 是 generate
    //      块拉平出来的；plant_addr / zombie_addr 由 drawer 内部算出来后
    //      送回给上面那几个 sprite_rom。
    logic [7:0] pixel_color;
    entity_drawer drawer_inst(
        .clk             (clk),
        .reset           (reset),
        .px              (px),
        .py              (py),
        .bg_color        (bg_color),
        .plant_present   (plant_present),
        .sunflower_present(sunflower_present),
        .selected_plant  (selected_plant),
        .zombie_alive    (zombie_alive),
        .zombie_x_packed (zombie_x_packed),
        .zombie_row_packed(zombie_row_packed),
        .pea_alive       (pea_alive),
        .pea_x_packed    (pea_x_packed),
        .pea_row_packed  (pea_row_packed),
        .cursor_visible  (cursor_visible),
        .cursor_col      (cursor_col),
        .cursor_row      (cursor_row),
        .sun_value       (sun_value),
        .plant_rd_addr   (plant_addr),
        .plant_rd_pixel  (plant_pixel),
        .sunflower_rd_pixel(sunflower_pixel),
        .zombie_rd_addr  (zombie_addr),
        .zombie_rd_pixel (zombie_pixel),
        .color_out       (pixel_color)
    );

    // ---------------------------------------------------------------
    // Palette: 8-bit color index -> 24-bit RGB.  Black during blanking.
    // ---------------------------------------------------------------
    // 高层：hw/color_palette.sv 是 256 项 LUT，把 entity_drawer 给的
    //      8-bit 调色板 index 翻译成 24-bit RGB。当前只用了 0..13 共
    //      14 个颜色（草地两种绿、僵尸红、豌豆亮绿、HUD 黄等），
    //      其余 index 默认黑色。
    // 低层：纯组合（case 表），改颜色直接改 color_palette.sv 那张表，
    //      不用动渲染逻辑。
    logic [7:0] pal_r, pal_g, pal_b;
    color_palette pal_inst(
        .index(pixel_color),
        .r    (pal_r),
        .g    (pal_g),
        .b    (pal_b)
    );

    // 最终 VGA 输出门控：消隐期 (VGA_BLANK_n=0) 必须强制输出黑色，
    // 否则板载 ADV7123 DAC 会把这一段亮度叠到行/场同步上，
    // 显示器要么花屏要么直接 no signal。这是 VGA 时序的硬性要求。
    // 可见区直接把 palette 的 RGB 送出去。
    always_comb begin
        if (VGA_BLANK_n) begin
            VGA_R = pal_r;
            VGA_G = pal_g;
            VGA_B = pal_b;
        end else begin
            VGA_R = 8'h00;
            VGA_G = 8'h00;
            VGA_B = 8'h00;
        end
    end

endmodule
