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
 */

module bg_grid(
    input  logic [9:0] px,
    input  logic [9:0] py,
    output logic [7:0] color_out
);

    // Color indices (must match color_palette.sv)
    localparam logic [7:0] COL_BLACK       = 8'd0;
    localparam logic [7:0] COL_DARK_GREEN  = 8'd1;
    localparam logic [7:0] COL_LIGHT_GREEN = 8'd2;
    localparam logic [7:0] COL_BLUE = 8'd13;

    // Game area bounds
    localparam logic [9:0] GRID_X = 10'd64;
    localparam logic [9:0] GRID_Y = 10'd112;
    localparam logic [9:0] GRID_W = 10'd512; // 8 cols x 64
    localparam logic [9:0] GRID_H = 10'd256; // 4 rows x 64

    wire in_grid = (px >= GRID_X) && (px < GRID_X + GRID_W) &&
                   (py >= GRID_Y) && (py < GRID_Y + GRID_H);

    // Within-grid offset (full 10 bits; high bit unused inside grid).
    // Cell index = offset >> 6 because cell size is 64.
    /* verilator lint_off UNUSED */
    wire [9:0] gx = px - GRID_X;
    wire [9:0] gy = py - GRID_Y;
    /* verilator lint_on UNUSED */

    // Checker pattern: alternate by (col + row) parity.  col is gx[8:6],
    // row is gy[7:6] — only the LSB of each matters for parity.
    wire light_cell = gx[6] ^ gy[6];

    always_comb begin
        if (!in_grid)
            color_out = COL_BLUE;
        else if (light_cell)
            color_out = COL_LIGHT_GREEN;
        else
            color_out = COL_DARK_GREEN;
    end

endmodule
