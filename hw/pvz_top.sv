/*
 * PvZ GPU — Avalon-MM peripheral for Plants vs Zombies VGA display
 *
 * Wires together: vga_counters, linebuffer, bg_grid, shape_table,
 * shape_renderer, and color_palette.
 *
 * Register map (32-bit, word-aligned, byte offsets from CPU):
 *   0x00  BG_CELL      - [12:8]=color, [4:3]=row, [2:0]=col
 *   0x04  SHAPE_ADDR   - [5:0]=shape index
 *   0x08  SHAPE_DATA0  - [1:0]=type, [2]=visible, [12:3]=x, [21:13]=y
 *   0x0C  SHAPE_DATA1  - [8:0]=w, [17:9]=h, [25:18]=color
 *   0x10  SHAPE_COMMIT - write any nonzero to commit
 *
 * Avalon uses word addresses internally (32-bit writedata -> address is
 * word offset 0-4). CPU uses byte offsets (base + 4*N).
 */

module pvz_top(
    input  logic        clk,
    input  logic        reset,

    // Avalon-MM slave interface
    input  logic [2:0]  address,      // word offset (0-4)
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
        .clk50(clk),
        .reset(reset),
        .hcount(hcount),
        .vcount(vcount),
        .VGA_CLK(VGA_CLK),
        .VGA_HS(VGA_HS),
        .VGA_VS(VGA_VS),
        .VGA_BLANK_n(VGA_BLANK_n),
        .VGA_SYNC_n(VGA_SYNC_n)
    );

    // Pixel coordinates from hcount
    wire [9:0] pixel_x = hcount[10:1];
    wire [9:0] pixel_y = vcount[9:0];

    // ---------------------------------------------------------------
    // Vsync latch signal: pulse at start of vertical blanking
    // ---------------------------------------------------------------
    logic vsync_latch;
    assign vsync_latch = (vcount == 10'd480) && (hcount == 11'd0);

    // ---------------------------------------------------------------
    // Linebuffer swap: pulse at hsync (end of each active line)
    // Swap when we transition from active to blanking on each line
    // ---------------------------------------------------------------
    logic lb_swap;
    assign lb_swap = (hcount == 11'd0) && (vcount < 10'd480);

    // ---------------------------------------------------------------
    // Render start: begin rendering the NEXT scanline
    // Start right after swap, during the new line's display
    // ---------------------------------------------------------------
    logic render_start;
    assign render_start = (hcount == 11'd2) && (vcount < 10'd480);

    // ---------------------------------------------------------------
    // Linebuffer
    // ---------------------------------------------------------------
    logic        lb_wr_en;
    logic [9:0]  lb_wr_addr;
    logic [7:0]  lb_wr_data;
    logic [7:0]  lb_rd_data;

    linebuffer lb_inst(
        .clk(clk),
        .reset(reset),
        .wr_en(lb_wr_en),
        .wr_addr(lb_wr_addr),
        .wr_data(lb_wr_data),
        .rd_addr(pixel_x),
        .rd_data(lb_rd_data),
        .swap(lb_swap)
    );

    // ---------------------------------------------------------------
    // Background grid
    // ---------------------------------------------------------------
    logic        bg_wr_en;
    logic [2:0]  bg_wr_col;
    logic [1:0]  bg_wr_row;
    logic [7:0]  bg_wr_color;

    logic [9:0]  bg_query_px, bg_query_py;
    logic [7:0]  bg_query_color;

    bg_grid bg_inst(
        .clk(clk),
        .reset(reset),
        .wr_en(bg_wr_en),
        .wr_col(bg_wr_col),
        .wr_row(bg_wr_row),
        .wr_color(bg_wr_color),
        .vsync_latch(vsync_latch),
        .px(bg_query_px),
        .py(bg_query_py),
        .color_out(bg_query_color)
    );

    // ---------------------------------------------------------------
    // Shape table
    // ---------------------------------------------------------------
    logic        st_addr_wr, st_data0_wr, st_data1_wr, st_commit_wr;
    logic [5:0]  st_addr_data;
    logic [31:0] st_data0, st_data1;

    logic [5:0]  st_rd_index;
    logic [1:0]  st_rd_type;
    logic        st_rd_visible;
    logic [9:0]  st_rd_x;
    logic [8:0]  st_rd_y, st_rd_w, st_rd_h;
    logic [7:0]  st_rd_color;

    shape_table st_inst(
        .clk(clk),
        .reset(reset),
        .addr_wr(st_addr_wr),
        .addr_data(st_addr_data),
        .data0_wr(st_data0_wr),
        .data0(st_data0),
        .data1_wr(st_data1_wr),
        .data1(st_data1),
        .commit_wr(st_commit_wr),
        .vsync_latch(vsync_latch),
        .rd_index(st_rd_index),
        .rd_type(st_rd_type),
        .rd_visible(st_rd_visible),
        .rd_x(st_rd_x),
        .rd_y(st_rd_y),
        .rd_w(st_rd_w),
        .rd_h(st_rd_h),
        .rd_color(st_rd_color)
    );

    // ---------------------------------------------------------------
    // Sprite ROM (32x32 peashooter, 1024x8-bit)
    // ---------------------------------------------------------------
    logic [9:0] sprite_rd_addr;
    logic [7:0] sprite_rd_pixel;

    sprite_rom sprite_inst(
        .clk(clk),
        .addr(sprite_rd_addr),
        .pixel(sprite_rd_pixel)
    );

    // ---------------------------------------------------------------
    // Shape renderer
    // ---------------------------------------------------------------
    /* verilator lint_off UNUSED */
    logic render_done;
    /* verilator lint_on UNUSED */

    shape_renderer renderer(
        .clk(clk),
        .reset(reset),
        .render_start(render_start),
        .scanline(pixel_y),
        .render_done(render_done),
        .bg_px(bg_query_px),
        .bg_py(bg_query_py),
        .bg_color(bg_query_color),
        .shape_index(st_rd_index),
        .shape_type(st_rd_type),
        .shape_visible(st_rd_visible),
        .shape_x(st_rd_x),
        .shape_y(st_rd_y),
        .shape_w(st_rd_w),
        .shape_h(st_rd_h),
        .shape_color(st_rd_color),
        .sprite_rd_addr(sprite_rd_addr),
        .sprite_rd_pixel(sprite_rd_pixel),
        .lb_wr_en(lb_wr_en),
        .lb_wr_addr(lb_wr_addr),
        .lb_wr_data(lb_wr_data)
    );

    // ---------------------------------------------------------------
    // Color palette: convert linebuffer 8-bit index to 24-bit RGB
    // ---------------------------------------------------------------
    logic [7:0] pal_r, pal_g, pal_b;

    color_palette pal_inst(
        .index(lb_rd_data),
        .r(pal_r),
        .g(pal_g),
        .b(pal_b)
    );

    // VGA output: palette color during active, black during blanking
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

    // ---------------------------------------------------------------
    // Avalon-MM register decode
    // ---------------------------------------------------------------
    always_comb begin
        bg_wr_en    = 1'b0;
        bg_wr_col   = 3'd0;
        bg_wr_row   = 2'd0;
        bg_wr_color = 8'd0;
        st_addr_wr  = 1'b0;
        st_addr_data = 6'd0;
        st_data0_wr = 1'b0;
        st_data0    = 32'd0;
        st_data1_wr = 1'b0;
        st_data1    = 32'd0;
        st_commit_wr = 1'b0;

        if (chipselect && write) begin
            case (address)
                3'd0: begin // BG_CELL (byte offset 0x00)
                    bg_wr_en    = 1'b1;
                    bg_wr_col   = writedata[2:0];
                    bg_wr_row   = writedata[4:3];
                    bg_wr_color = writedata[15:8];
                end
                3'd1: begin // SHAPE_ADDR (byte offset 0x04)
                    st_addr_wr   = 1'b1;
                    st_addr_data = writedata[5:0];
                end
                3'd2: begin // SHAPE_DATA0 (byte offset 0x08)
                    st_data0_wr = 1'b1;
                    st_data0    = writedata;
                end
                3'd3: begin // SHAPE_DATA1 (byte offset 0x0C)
                    st_data1_wr = 1'b1;
                    st_data1    = writedata;
                end
                3'd4: begin // SHAPE_COMMIT (byte offset 0x10)
                    st_commit_wr = (writedata != 32'd0);
                end
                default: ;
            endcase
        end
    end

endmodule
