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
 */

module color_palette(
    input  logic [7:0]  index,
    output logic [7:0]  r,
    output logic [7:0]  g,
    output logic [7:0]  b
);

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
