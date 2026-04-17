/*
 * Scanline-based shape renderer
 *
 * For each scanline, iterates through all 48 shape table entries and
 * writes pixels to the linebuffer draw port. Later shapes overwrite
 * earlier ones (painter's algorithm / z-order by index).
 *
 * Supports four shape types:
 *   0 = filled rectangle
 *   1 = filled circle (bounding-box + distance check)
 *   2 = 7-segment digit (digit value in w[3:0])
 *   3 = sprite (32x32 ROM, rendered at 2x scale -> 64x64 on screen;
 *               pixel value 0xFF is transparent)
 *
 * Operation:
 *   1. On render_start pulse, begin background fill from bg_grid
 *   2. After background, iterate shapes 0-47
 *   3. Assert render_done when complete
 *
 * The renderer must complete within ~1600 pixel clocks per scanline.
 */

module shape_renderer(
    input  logic        clk,
    input  logic        reset,

    // Control
    input  logic        render_start,   // pulse: begin rendering this scanline
    input  logic [9:0]  scanline,       // current scanline number (0-479)
    output logic        render_done,    // asserted when rendering is complete

    // Background grid query
    output logic [9:0]  bg_px,
    output logic [9:0]  bg_py,
    input  logic [7:0]  bg_color,

    // Shape table read port
    output logic [5:0]  shape_index,
    input  logic [1:0]  shape_type,
    input  logic        shape_visible,
    input  logic [9:0]  shape_x,
    input  logic [8:0]  shape_y,
    input  logic [8:0]  shape_w,
    input  logic [8:0]  shape_h,
    input  logic [7:0]  shape_color,

    // Sprite ROM read port (combinational addr, 1-cycle registered pixel)
    output logic [9:0]  sprite_rd_addr,
    input  logic [7:0]  sprite_rd_pixel,

    // Linebuffer write port
    output logic        lb_wr_en,
    output logic [9:0]  lb_wr_addr,
    output logic [7:0]  lb_wr_data
);

    // FSM states
    typedef enum logic [2:0] {
        S_IDLE,
        S_BG_FILL,
        S_SHAPE_SETUP,
        S_SHAPE_CHECK,
        S_SHAPE_DRAW,
        S_DONE
    } state_t;

    state_t state;

    // Background fill counter
    logic [9:0] bg_x;

    // Shape iteration
    logic [5:0] cur_shape;
    logic [9:0] draw_x;

    // Shape geometry (registered from shape table for current shape)
    logic [1:0]  s_type;
    logic        s_visible;
    logic [9:0]  s_x;
    logic [8:0]  s_y;
    logic [8:0]  s_w;
    logic [8:0]  s_h;
    logic [7:0]  s_color;

    // Sprite pipeline (1-cycle ROM read latency)
    logic        sprite_pending;
    logic [9:0]  sprite_pending_addr;

    // Combinational sprite ROM address: 2x downscale from screen to ROM (32x32)
    // rom_x = (draw_x - s_x) >> 1, rom_y = (scanline - s_y) >> 1
    /* verilator lint_off UNUSED */
    wire [9:0] sprite_local_x = draw_x - s_x;
    wire [9:0] sprite_local_y = scanline - {1'b0, s_y};
    /* verilator lint_on UNUSED */
    assign sprite_rd_addr = {sprite_local_y[5:1], sprite_local_x[5:1]};

    // (Circle and 7-seg helpers are computed inline in S_SHAPE_DRAW)

    // 7-segment decode table
    function logic [6:0] decode_7seg(input logic [3:0] val);
        case (val)
            4'd0: decode_7seg = 7'b0111111; // a,b,c,d,e,f
            4'd1: decode_7seg = 7'b0000110; // b,c
            4'd2: decode_7seg = 7'b1011011; // a,b,d,e,g
            4'd3: decode_7seg = 7'b1001111; // a,b,c,d,g
            4'd4: decode_7seg = 7'b1100110; // b,c,f,g
            4'd5: decode_7seg = 7'b1101101; // a,c,d,f,g
            4'd6: decode_7seg = 7'b1111101; // a,c,d,e,f,g
            4'd7: decode_7seg = 7'b0000111; // a,b,c
            4'd8: decode_7seg = 7'b1111111; // all
            4'd9: decode_7seg = 7'b1101111; // a,b,c,d,f,g
            default: decode_7seg = 7'b0000000;
        endcase
    endfunction

    // Check if a pixel is in a 7-segment digit
    // Digit bounding box: 20 wide x 30 tall
    // Segment thickness: 3 pixels
    function logic check_7seg(
        input logic [6:0] segs,
        input logic [9:0] lx,  // local x within digit (0-19)
        input logic [9:0] ly   // local y within digit (0-29)
    );
        logic hit;
        hit = 1'b0;
        // Segment a: top horizontal (x:3-16, y:0-2)
        if (segs[0] && lx >= 3 && lx <= 16 && ly <= 2) hit = 1'b1;
        // Segment b: top-right vertical (x:17-19, y:3-13)
        if (segs[1] && lx >= 17 && lx <= 19 && ly >= 3 && ly <= 13) hit = 1'b1;
        // Segment c: bottom-right vertical (x:17-19, y:16-26)
        if (segs[2] && lx >= 17 && lx <= 19 && ly >= 16 && ly <= 26) hit = 1'b1;
        // Segment d: bottom horizontal (x:3-16, y:27-29)
        if (segs[3] && lx >= 3 && lx <= 16 && ly >= 27 && ly <= 29) hit = 1'b1;
        // Segment e: bottom-left vertical (x:0-2, y:16-26)
        if (segs[4] && lx <= 2 && ly >= 16 && ly <= 26) hit = 1'b1;
        // Segment f: top-left vertical (x:0-2, y:3-13)
        if (segs[5] && lx <= 2 && ly >= 3 && ly <= 13) hit = 1'b1;
        // Segment g: middle horizontal (x:3-16, y:13-15)
        if (segs[6] && lx >= 3 && lx <= 16 && ly >= 13 && ly <= 15) hit = 1'b1;
        check_7seg = hit;
    endfunction

    assign bg_py = scanline;
    assign render_done = (state == S_DONE);

    always_ff @(posedge clk or posedge reset) begin
        if (reset) begin
            state      <= S_IDLE;
            lb_wr_en   <= 1'b0;
            lb_wr_addr <= 10'd0;
            lb_wr_data <= 8'd0;
            bg_x       <= 10'd0;
            cur_shape  <= 6'd0;
            draw_x     <= 10'd0;
            shape_index <= 6'd0;
            sprite_pending      <= 1'b0;
            sprite_pending_addr <= 10'd0;
        end else begin
            lb_wr_en <= 1'b0; // default: no write

            case (state)
                S_IDLE: begin
                    if (render_start) begin
                        bg_x  <= 10'd0;
                        state <= S_BG_FILL;
                    end
                end

                // Fill the entire line with background grid colors
                S_BG_FILL: begin
                    bg_px      <= bg_x;
                    lb_wr_en   <= 1'b1;
                    lb_wr_addr <= bg_x;
                    lb_wr_data <= bg_color;
                    if (bg_x == 10'd639) begin
                        cur_shape   <= 6'd0;
                        shape_index <= 6'd0;
                        state       <= S_SHAPE_SETUP;
                    end else begin
                        bg_x <= bg_x + 10'd1;
                    end
                end

                // Register the shape table data for current shape
                S_SHAPE_SETUP: begin
                    // shape_index is already set; data available next cycle
                    s_type    <= shape_type;
                    s_visible <= shape_visible;
                    s_x       <= shape_x;
                    s_y       <= shape_y;
                    s_w       <= shape_w;
                    s_h       <= shape_h;
                    s_color   <= shape_color;
                    state     <= S_SHAPE_CHECK;
                end

                // Check if this shape overlaps the current scanline
                S_SHAPE_CHECK: begin
                    if (!s_visible ||
                        scanline < {1'b0, s_y} ||
                        scanline >= ({1'b0, s_y} + {1'b0, s_h})) begin
                        // Skip this shape
                        if (cur_shape == 6'd47) begin
                            state <= S_DONE;
                        end else begin
                            cur_shape   <= cur_shape + 6'd1;
                            shape_index <= cur_shape + 6'd1;
                            state       <= S_SHAPE_SETUP;
                        end
                    end else begin
                        // Shape overlaps scanline: start drawing
                        draw_x         <= s_x;
                        sprite_pending <= 1'b0;
                        state          <= S_SHAPE_DRAW;
                    end
                end

                // Draw pixels for the current shape on this scanline
                S_SHAPE_DRAW: begin
                    if (s_type == 2'd3) begin
                        // Sprite path: ROM has 1-cycle latency, pipeline writes
                        // Commit pending write from previous cycle
                        if (sprite_pending && sprite_rd_pixel != 8'hFF) begin
                            lb_wr_en   <= 1'b1;
                            lb_wr_addr <= sprite_pending_addr;
                            lb_wr_data <= sprite_rd_pixel;
                        end

                        // Default: clear pending (overridden below if issuing)
                        sprite_pending <= 1'b0;

                        if (draw_x < s_x + {1'b0, s_w} && draw_x < 10'd640) begin
                            // Issue next fetch
                            sprite_pending      <= 1'b1;
                            sprite_pending_addr <= draw_x;
                            draw_x              <= draw_x + 10'd1;
                        end else if (!sprite_pending) begin
                            // All pixels issued AND flushed: move on
                            if (cur_shape == 6'd47) begin
                                state <= S_DONE;
                            end else begin
                                cur_shape   <= cur_shape + 6'd1;
                                shape_index <= cur_shape + 6'd1;
                                state       <= S_SHAPE_SETUP;
                            end
                        end
                        // else: last fetch being flushed this cycle, stay one more
                    end else if (draw_x >= s_x + {1'b0, s_w} || draw_x >= 10'd640) begin
                        // Done with this shape
                        if (cur_shape == 6'd47) begin
                            state <= S_DONE;
                        end else begin
                            cur_shape   <= cur_shape + 6'd1;
                            shape_index <= cur_shape + 6'd1;
                            state       <= S_SHAPE_SETUP;
                        end
                    end else if (draw_x < 10'd640) begin
                        // Determine if pixel should be drawn based on shape type
                        logic pixel_hit;
                        logic [9:0] local_x, local_y_full;
                        logic signed [10:0] ddx, ddy;
                        logic [21:0] dist2;
                        logic [21:0] r2;

                        local_x = draw_x - s_x;
                        local_y_full = scanline - {1'b0, s_y};

                        case (s_type)
                            2'd0: begin // Rectangle: always hit within bounds
                                pixel_hit = 1'b1;
                            end
                            2'd1: begin // Circle: distance check
                                // Center at (s_x + s_w/2, s_y + s_h/2)
                                // Radius = s_w/2
                                ddx = $signed({1'b0, draw_x}) -
                                      $signed({1'b0, s_x} + {2'b0, s_w[8:1]});
                                ddy = $signed({1'b0, scanline}) -
                                      $signed({2'b0, s_y} + {2'b0, s_h[8:1]});
                                dist2 = ddx * ddx + ddy * ddy;
                                r2 = {13'd0, s_w[8:1]} * {13'd0, s_w[8:1]};
                                pixel_hit = (dist2 <= r2);
                            end
                            2'd2: begin // 7-segment digit
                                pixel_hit = check_7seg(
                                    decode_7seg(s_w[3:0]),
                                    local_x,
                                    local_y_full
                                );
                            end
                            default: pixel_hit = 1'b0;
                        endcase

                        if (pixel_hit) begin
                            lb_wr_en   <= 1'b1;
                            lb_wr_addr <= draw_x;
                            lb_wr_data <= s_color;
                        end

                        draw_x <= draw_x + 10'd1;
                    end
                end

                S_DONE: begin
                    // Stay in done until next render_start
                    if (render_start) begin
                        bg_x  <= 10'd0;
                        state <= S_BG_FILL;
                    end
                end

                default: state <= S_IDLE;
            endcase
        end
    end

endmodule
