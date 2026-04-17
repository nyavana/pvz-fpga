/*
 * Sprite ROM — 32x32 palette-indexed sprite
 *
 * Stores 1024 bytes loaded from peas_idx.mem at synthesis / simulation.
 * Each byte is:
 *   0x00-0x0C : palette index (matches color_palette.sv)
 *   0xFF      : transparent (renderer must skip write)
 *
 * Read latency: 1 clock (inferred M10K block RAM).
 */
module sprite_rom(
    input  logic        clk,
    input  logic [9:0]  addr,   // 0..1023 = y*32 + x
    output logic [7:0]  pixel
);

    logic [7:0] rom [0:1023];

    initial begin
        $readmemh("peas_idx.mem", rom);
    end

    always_ff @(posedge clk)
        pixel <= rom[addr];

endmodule
