# pvz_top_hw.tcl — Platform Designer component descriptor for pvz_top
#
# Defines the Avalon-MM slave interface so Platform Designer can
# generate the bus fabric and device tree entries automatically.

package require -exact qsys 16.0

# Module properties
set_module_property DESCRIPTION "PvZ GPU — VGA shape-based display engine"
set_module_property NAME pvz_top
set_module_property VERSION 1.0
set_module_property INTERNAL false
set_module_property OPAQUE_ADDRESS_MAP true
set_module_property AUTHOR "CSEE4840 Team"
set_module_property DISPLAY_NAME "PvZ GPU"
set_module_property INSTANTIATE_IN_SYSTEM_MODULE true
set_module_property EDITABLE true
set_module_property REPORT_TO_TALKBACK false
set_module_property ALLOW_GREYBOX_GENERATION false
set_module_property REPORT_HIERARCHY false

# Device tree compatible string — must match kernel driver's of_device_id
set_module_assignment embeddedsw.dts.compatible "csee4840,pvz_gpu-1.0"
set_module_assignment embeddedsw.dts.group "pvz_gpu"
set_module_assignment embeddedsw.dts.vendor "csee4840"

# Source files
add_fileset QUARTUS_SYNTH QUARTUS_SYNTH "" ""
set_fileset_property QUARTUS_SYNTH TOP_LEVEL pvz_top
add_fileset_file pvz_top.sv SYSTEM_VERILOG PATH pvz_top.sv TOP_LEVEL_FILE
add_fileset_file vga_counters.sv SYSTEM_VERILOG PATH vga_counters.sv
add_fileset_file linebuffer.sv SYSTEM_VERILOG PATH linebuffer.sv
add_fileset_file bg_grid.sv SYSTEM_VERILOG PATH bg_grid.sv
add_fileset_file shape_table.sv SYSTEM_VERILOG PATH shape_table.sv
add_fileset_file shape_renderer.sv SYSTEM_VERILOG PATH shape_renderer.sv
add_fileset_file color_palette.sv SYSTEM_VERILOG PATH color_palette.sv
add_fileset_file sprite_rom.sv SYSTEM_VERILOG PATH sprite_rom.sv
add_fileset_file peas_idx.mem OTHER PATH peas_idx.mem

# Clock interface
add_interface clock clock end
set_interface_property clock clockRate 0
add_interface_port clock clk clk Input 1

# Reset interface
add_interface reset reset end
set_interface_property reset associatedClock clock
set_interface_property reset synchronousEdges DEASSERT
add_interface_port reset reset reset Input 1

# Avalon-MM slave interface
add_interface s1 avalon end
set_interface_property s1 addressUnits WORDS
set_interface_property s1 associatedClock clock
set_interface_property s1 associatedReset reset
set_interface_property s1 bitsPerSymbol 8
set_interface_property s1 burstOnBurstBoundariesOnly false
set_interface_property s1 burstcountUnits WORDS
set_interface_property s1 explicitAddressSpan 0
set_interface_property s1 holdTime 0
set_interface_property s1 linewrapBursts false
set_interface_property s1 maximumPendingReadTransactions 0
set_interface_property s1 maximumPendingWriteTransactions 0
set_interface_property s1 readLatency 0
set_interface_property s1 readWaitTime 1
set_interface_property s1 setupTime 0
set_interface_property s1 timingUnits Cycles
set_interface_property s1 writeWaitTime 0

add_interface_port s1 address address Input 3
add_interface_port s1 writedata writedata Input 32
add_interface_port s1 write write Input 1
add_interface_port s1 chipselect chipselect Input 1

# VGA conduit interface (directly exported to top level)
add_interface vga conduit end
set_interface_property vga associatedClock clock
set_interface_property vga associatedReset reset

add_interface_port vga VGA_R r Output 8
add_interface_port vga VGA_G g Output 8
add_interface_port vga VGA_B b Output 8
add_interface_port vga VGA_CLK clk Output 1
add_interface_port vga VGA_HS hs Output 1
add_interface_port vga VGA_VS vs Output 1
add_interface_port vga VGA_BLANK_n blank_n Output 1
add_interface_port vga VGA_SYNC_n sync_n Output 1
