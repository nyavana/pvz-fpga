/*
 * Sprite ROM — 64x64 palette-indexed sprite
 *
 * Stores 4096 bytes loaded from MEM_FILE at synthesis / simulation.
 * Each byte is:
 *   0x00-0x0C : palette index (matches color_palette.sv)
 *   0xFF      : transparent (renderer must skip write)
 *
 * Read latency: 1 clock (inferred M10K block RAM).
 */
module sprite_rom #(
    parameter MEM_FILE = "peashooter_idx.mem"
) (
    input  logic        clk,
    input  logic [11:0] addr,   // 0..4095 = y*64 + x
    output logic [7:0]  pixel
);

    logic [7:0] rom [0:4095];

    initial begin
        $readmemh(MEM_FILE, rom);
    end

    always_ff @(posedge clk)
        pixel <= rom[addr];

endmodule
