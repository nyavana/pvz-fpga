/*
 * PvZ GPU — top-level Avalon-MM peripheral
 *
 * Wires together VGA timing, background grid, sprite ROM, and the
 * entity drawer.  Holds the entity register file that software writes
 * via Avalon.
 *
 * Register map (32-bit words, byte offset = 4 * word index):
 *   word  0..31  PVZ_PLANT[i]   bit 0 = peashooter present at cell i
 *                                (i = row*8 + col, row in 0..3, col in 0..7)
 *   word 32..39  PVZ_ZOMBIE[i]  bit 31 = alive
 *                                bits [9:0]   = x_pixel (0..639)
 *                                bits [11:10] = row (0..3)
 *   word 40..47  PVZ_PEA[i]     same encoding as zombie
 *   word 48      PVZ_CURSOR     bit 31 = visible
 *                                bits [4:2] = col (0..7)
 *                                bits [1:0] = row (0..3)
 *   word 49      PVZ_SUN        bits [13:0] = sun value (reserved)
 *
 * Avalon notes:
 *   - The address port is in WORDS (qsys addressUnits = WORDS) so a
 *     CPU byte offset N maps to address = N >> 2.
 *   - Writes take effect on the next clock; no commit handshake.
 *   - There is no vsync latching, so a write that races the scan can
 *     produce one frame of tearing.  Acceptable for ~60 Hz game state.
 */

module pvz_top(
    input  logic        clk,
    input  logic        reset,

    // Avalon-MM slave
    input  logic [5:0]  address,    // word index, 0..49 used, 64 max
    input  logic [31:0] writedata,
    input  logic        write,
    input  logic        chipselect,

    // VGA output
    output logic [7:0]  VGA_R, VGA_G, VGA_B,
    output logic        VGA_CLK, VGA_HS, VGA_VS,
    output logic        VGA_BLANK_n,
    output logic        VGA_SYNC_n
);

    // ---------------------------------------------------------------
    // VGA timing
    // ---------------------------------------------------------------
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

    wire [9:0] px = hcount[10:1];
    wire [9:0] py = vcount;

    // ---------------------------------------------------------------
    // Entity register file
    // ---------------------------------------------------------------
    // Plants: one bit per grid cell (32 cells)
    logic [31:0] plant_present;

    // Zombies and peas: 8 each.  Alive bits packed; x and row also
    // packed into wide buses for handing to entity_drawer.
    logic [7:0]  zombie_alive, pea_alive;
    logic [9:0]  zombie_x   [0:7];
    logic [1:0]  zombie_row [0:7];
    logic [9:0]  pea_x      [0:7];
    logic [1:0]  pea_row    [0:7];

    // Cursor + sun
    logic        cursor_visible;
    logic [2:0]  cursor_col;
    logic [1:0]  cursor_row;
    logic [13:0] sun_value;

    // ---------------------------------------------------------------
    // Avalon-MM write decode
    // ---------------------------------------------------------------
    always_ff @(posedge clk or posedge reset) begin
        if (reset) begin
            plant_present  <= 32'd0;
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
            // Plant cells: word 0..31  -> plant_present[address[4:0]]
            if (address < 6'd32) begin
                plant_present[address[4:0]] <= writedata[0];
            end
            // Zombies: word 32..39
            else if (address < 6'd40) begin
                zombie_alive[address[2:0]] <= writedata[31];
                zombie_x[address[2:0]]     <= writedata[9:0];
                zombie_row[address[2:0]]   <= writedata[11:10];
            end
            // Peas: word 40..47
            else if (address < 6'd48) begin
                pea_alive[address[2:0]] <= writedata[31];
                pea_x[address[2:0]]     <= writedata[9:0];
                pea_row[address[2:0]]   <= writedata[11:10];
            end
            // Cursor: word 48
            else if (address == 6'd48) begin
                cursor_visible <= writedata[31];
                cursor_col     <= writedata[4:2];
                cursor_row     <= writedata[1:0];
            end
            // Sun: word 49
            else if (address == 6'd49) begin
                sun_value <= writedata[13:0];
            end
        end
    end

    // Pack arrays into wide buses for the drawer module
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
    logic [11:0] plant_addr;
    logic [7:0]  plant_pixel;
    sprite_rom #(.MEM_FILE("peashooter_idx.mem")) plant_rom_inst(
        .clk  (clk),
        .addr (plant_addr),
        .pixel(plant_pixel)
    );

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
    logic [7:0] pixel_color;
    entity_drawer drawer_inst(
        .clk             (clk),
        .reset           (reset),
        .px              (px),
        .py              (py),
        .bg_color        (bg_color),
        .plant_present   (plant_present),
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
        .zombie_rd_addr  (zombie_addr),
        .zombie_rd_pixel (zombie_pixel),
        .color_out       (pixel_color)
    );

    // ---------------------------------------------------------------
    // Palette: 8-bit color index -> 24-bit RGB.  Black during blanking.
    // ---------------------------------------------------------------
    logic [7:0] pal_r, pal_g, pal_b;
    color_palette pal_inst(
        .index(pixel_color),
        .r    (pal_r),
        .g    (pal_g),
        .b    (pal_b)
    );

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
