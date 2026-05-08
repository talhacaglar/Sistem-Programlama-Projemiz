# Create project and set device
set_device -name GW1NR-9 GW1NR-LV9QN88PC6/I5

# Add source files
add_file fpga/src/top.v
add_file fpga/src/picorv32.v
add_file fpga/src/memory.v
add_file fpga/src/gpio_led.v

# Add constraint file
add_file fpga/constraints/tangnano9k.cst

# Set options
set_option -top_module top

# Run all (Synthesis, Placement, Routing, Bitstream generation)
run all
