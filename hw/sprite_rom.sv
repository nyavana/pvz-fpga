/*
 * Sprite ROM — 64x64 palette-indexed sprite
 *
 * Stores 4096 bytes loaded from MEM_FILE at synthesis / simulation.
 * Each byte is:
 *   0x00-0x0C : palette index (matches color_palette.sv)
 *   0xFF      : transparent (renderer must skip write)
 *
 * Read latency: 1 clock (inferred M10K block RAM).
 *
 * ===================================================================
 * 中文说明
 * ===================================================================
 *
 * 这个模块就是 sprite 的存储 + 读出。一份模块代码、不同 MEM_FILE 参数
 * 可以例化出多份独立的 ROM（pvz_top.sv 里实际例化了 3 份：peashooter、
 * sunflower、zombie）。每份对应一块 64x64 = 4096 字节的 sprite 图。
 *
 * 高层：
 *   - 由 entity_drawer.sv 喂地址（addr = y*64 + x，sprite 内坐标），
 *     这个模块下一个时钟把对应像素的"调色板索引"输出。
 *   - 字节值 0x00..0x0C 是真颜色，会被 color_palette.sv 翻译成 RGB；
 *     字节值 0xFF 是"透明"，entity_drawer 看到 0xFF 就直接跳过这层，
 *     露出下面的背景或其他 sprite。
 *
 * 低层：
 *   - $readmemh 是 SystemVerilog 标准系统任务，Quartus 综合时会把
 *     MEM_FILE（如 peashooter_idx.mem）的内容初始化进 ROM 数组，
 *     ModelSim 仿真时也会重新读一遍。三个 .mem 文件需要放在 Quartus
 *     的工程目录或者 search path 里。
 *   - 4096 个 8-bit 单元，Quartus 会自动推断成一块 M10K 嵌入式 BRAM。
 *     M10K 必须有寄存的输出 -> 这就是为什么有 always_ff 那一拍。
 *
 * 1-cycle read latency 是整个流水线设计的关键约束：
 *   - 在 cycle T 写 addr，cycle T+1 才能拿到 pixel。
 *   - entity_drawer.sv 因此被切成两段：stage 1 算 addr，
 *     时钟沿之后 stage 2 用上一拍 addr 对应的 pixel 做最终的颜色 mux。
 */
module sprite_rom #(
    // 例化时改这个参数就能切换 sprite。pvz_top.sv 例化时分别传
    // "peashooter_idx.mem" / "sunflower_idx.mem" / "zombie_idx.mem"。
    parameter MEM_FILE = "peashooter_idx.mem"
) (
    input  logic        clk,        // 时钟，和顶层共用 50 MHz
    input  logic [11:0] addr,   // 0..4095 = y*64 + x
                                // 12 位地址 = 高 6 位行号 [11:6] + 低 6 位列号 [5:0]，
                                // 因为 sprite 是 64x64，所以两边都用 6 位刚好。
                                // entity_drawer.sv 算 addr 时直接 {in_cell_y, in_cell_x}。
    output logic [7:0]  pixel       // 调色板索引，1 cycle 后有效
);

    // 4096 字节的 ROM 数组。Quartus 会把它推断成一块 M10K
    // 嵌入式 BRAM（一个 M10K 块 10 Kb，能装下这 4 KB 数据）。
    logic [7:0] rom [0:4095];

    // 初始化：综合时和仿真时都会执行。MEM_FILE 是十六进制文本，
    // 一行一个字节。开发流程是先把 PNG 转成调色板索引图，再用
    // 脚本生成 .mem 文件（具体在 sw/或工具目录里）。
    initial begin
        $readmemh(MEM_FILE, rom);
    end

    // 同步读：addr 在本拍准备好，下拍才有 pixel。
    // M10K 块必须用寄存的输出端口，所以这里只能这么写 —— 改成
    // assign pixel = rom[addr] 反而会让综合器放弃用 BRAM、
    // 占大量 LE。
    always_ff @(posedge clk)
        pixel <= rom[addr];

endmodule
