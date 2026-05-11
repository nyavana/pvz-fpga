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
 */

module entity_drawer(
    input  logic        clk,
    input  logic        reset,

    // Pixel coordinates from VGA scan
    input  logic [9:0]  px,
    input  logic [9:0]  py,

    // Background color for this pixel (combinational from bg_grid)
    input  logic [7:0]  bg_color,

    // ---------- Entity registers (driven by pvz_top register file) ----------
    // Plants: one bit per grid cell, bit (row*8+col).
    input  logic [31:0] plant_present,
    input  logic [31:0] sunflower_present,

    // Currently selected plant for the top HUD box (0=pea, 1=sunflower)
    input  logic [1:0]  selected_plant,

    // Up to 8 zombies / 8 peas. Packed so we don't need unpacked-array
    // ports (more portable across synthesis tools).
    //   zombie_x[i]   = pixel x position (0..639)   width 10
    //   zombie_row[i] = grid row (0..3)             width 2
    //   zombie_alive  = bit i high if zombie i is on screen
    input  logic [7:0]  zombie_alive,
    input  logic [79:0] zombie_x_packed,    // {zombie_x[7], ..., zombie_x[0]}
    input  logic [15:0] zombie_row_packed,  // {zombie_row[7], ..., zombie_row[0]}

    input  logic [7:0]  pea_alive,
    input  logic [79:0] pea_x_packed,
    input  logic [15:0] pea_row_packed,

    // Cursor: a hollow yellow border around one cell
    input  logic        cursor_visible,
    input  logic [2:0]  cursor_col,
    input  logic [1:0]  cursor_row,

    // Sun count for HUD (each block = 100 sun, up to 10 blocks)
    input  logic [13:0] sun_value,

    // Plant sprite ROM read interface (1-cycle read latency).
    // Sunflower ROM shares the same address (issued via plant_rd_addr).
    output logic [11:0] plant_rd_addr,
    input  logic [7:0]  plant_rd_pixel,
    input  logic [7:0]  sunflower_rd_pixel,

    // Zombie sprite ROM read interface (1-cycle read latency)
    output logic [11:0] zombie_rd_addr,
    input  logic [7:0]  zombie_rd_pixel,

    // Final pixel color (registered, 1 cycle of latency vs px/py)
    output logic [7:0]  color_out
);

    // ---------------------------------------------------------------
    // Color indices (must match color_palette.sv)
    // ---------------------------------------------------------------
    localparam logic [7:0] COL_YELLOW       = 8'd4;
    localparam logic [7:0] COL_GREEN        = 8'd7;
    localparam logic [7:0] COL_BRIGHT_GREEN = 8'd9;
    localparam logic [7:0] COL_ORANGE       = 8'd12;
    localparam logic [7:0] COL_TRANSPARENT  = 8'hFF;

    // Layout constants (mirror bg_grid.sv)
    localparam logic [9:0] GRID_X     = 10'd64;
    localparam logic [9:0] GRID_Y     = 10'd112;
    localparam logic [9:0] CELL       = 10'd64;

    // Entity sprite sizes
    localparam logic [9:0] ZOMBIE_W = 10'd64;
    localparam logic [9:0] ZOMBIE_H = 10'd64;
    localparam logic [9:0] PEA_SIZE = 10'd8;
    localparam logic [9:0] CURSOR_BORDER = 10'd4;

    // Sun HUD layout: 10 blocks across the top of the screen
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
    localparam logic [9:0] SEL_X0     = 10'd8;     // peashooter icon
    localparam logic [9:0] SEL_X1     = 10'd64;    // sunflower icon (8 + 48 + 8 px gap)
    localparam logic [9:0] SEL_Y      = 10'd8;
    localparam logic [9:0] SEL_SZ     = 10'd48;
    localparam logic [9:0] SEL_BORDER = 10'd4;

    // ---------------------------------------------------------------
    // Unpack the zombie/pea arrays into indexable arrays
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
    // ---------------------------------------------------------------
    wire in_grid_x = (px >= GRID_X) && (px < GRID_X + 10'd512);
    wire in_grid_y = (py >= GRID_Y) && (py < GRID_Y + 10'd256);
    wire in_grid   = in_grid_x && in_grid_y;

    /* verilator lint_off UNUSED */
    wire [9:0] gx = px - GRID_X;
    wire [9:0] gy = py - GRID_Y;
    /* verilator lint_on UNUSED */

    // Cell index and within-cell pixel (cells are 64 px = 2^6).  Sprite
    // is 64x64 so we use the full 6 bits of in_cell_{x,y} as the address.
    wire [2:0] cell_col  = gx[8:6];
    wire [1:0] cell_row  = gy[7:6];
    wire [5:0] in_cell_x = gx[5:0];
    wire [5:0] in_cell_y = gy[5:0];

    // Plant / sunflower bits for this cell (false if outside grid)
    wire [4:0] plant_idx = {cell_row, cell_col};
    wire plant_here     = in_grid && plant_present[plant_idx];
    wire sunflower_here = in_grid && sunflower_present[plant_idx];

    // Plant sprite ROM address: 1:1 mapping (64x64 ROM into 64x64 cell).
    assign plant_rd_addr = {in_cell_y, in_cell_x};

    // ---------------------------------------------------------------
    // Stage 1: zombie hit detection AND zombie sprite ROM address.
    // Priority encoder: first alive zombie covering this pixel wins
    // (zombies don't normally overlap on screen).
    // ---------------------------------------------------------------
    logic        zombie_hit_comb;
    logic [5:0]  zombie_in_x, zombie_in_y;
    always_comb begin
        zombie_hit_comb = 1'b0;
        zombie_in_x     = 6'd0;
        zombie_in_y     = 6'd0;
        for (int i = 0; i < 8; i++) begin
            logic [9:0] zy_top, dx, dy;
            zy_top = GRID_Y + ({8'd0, zombie_row[i]} << 6);
            dx     = px - zombie_x[i];
            dy     = py - zy_top;
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
    assign zombie_rd_addr = {zombie_in_y, zombie_in_x};

    // ---------------------------------------------------------------
    // Stage 1: combinational hit detection for non-sprite entities
    // ---------------------------------------------------------------
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
    logic cursor_hit_comb;
    always_comb begin
        logic [9:0] cur_left, cur_top;
        cur_left = GRID_X + ({7'd0, cursor_col} << 6);
        cur_top  = GRID_Y + ({8'd0, cursor_row} << 6);
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
    logic sel0_hit_comb, sel0_border_comb;
    logic sel1_hit_comb, sel1_border_comb;
    always_comb begin
        // Box 0 — peashooter
        sel0_hit_comb = (px >= SEL_X0 && px < SEL_X0 + SEL_SZ &&
                         py >= SEL_Y  && py < SEL_Y  + SEL_SZ);
        sel0_border_comb = 1'b0;
        if (sel0_hit_comb) begin
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
    always_comb begin
        color_out = bg_color_d;
        if (plant_here_d && plant_rd_pixel != COL_TRANSPARENT)
            color_out = plant_rd_pixel;
        if (sunflower_here_d && sunflower_rd_pixel != COL_TRANSPARENT)
            color_out = sunflower_rd_pixel;
        if (pea_hit_d)
            color_out = COL_BRIGHT_GREEN;
        if (zombie_hit_d && zombie_rd_pixel != COL_TRANSPARENT)
            color_out = zombie_rd_pixel;
        if (cursor_hit_d)
            color_out = COL_YELLOW;
        if (sun_hit_d)
            color_out = COL_YELLOW;
        // Selector fills: always show both plants so the player can
        // see what's on offer.  Box 0 = peashooter (green), box 1 =
        // sunflower (orange).
        if (sel0_hit_d)
            color_out = COL_GREEN;
        if (sel1_hit_d)
            color_out = COL_ORANGE;
        // Selector border: only the chosen box wears the yellow
        // outline.  TAB flips selected_plant in software, which
        // moves the border from one box to the other.
        if (sel0_border_d && selected_plant_d == 2'd0)
            color_out = COL_YELLOW;
        if (sel1_border_d && selected_plant_d == 2'd1)
            color_out = COL_YELLOW;
    end

endmodule
