# Robot Diagnostics

Robot Diagnostics is a system designed to interface with robots using reconfigurable hardware. Its goal is to provide real-time diagnostics and visualization of robot data through a custom FPGA setup.

The system uses a **DE1-SoC FPGA**, which communicates with the robot via a **custom UART module** implemented in Verilog. Robot data is transmitted through this UART interface and displayed on a **VGA monitor** for easy visualization and debugging.

## System Layout

- **FPGA (DE1-SoC):** Handles communication with the robot and processes diagnostic data.  
- **UART Module:** Custom Verilog module for serial communication between the robot and FPGA.  
- **VGA Output:** Displays the robot’s real-time data and diagnostics.  

## Build Instructions

Building and deploying Robot Diagnostics requires compiling the FPGA design and configuring the DE1-SoC. Follow these steps:

1. **Compile FPGA design:** Use Quartus to compile the Verilog design for the DE1-SoC.  
2. **Configure DE1-SoC:** Load the compiled design onto the FPGA.  
3. **Run Linux on DE1-SoC:** Ensure Linux is installed and running on the board to enable UART communication and system control.
