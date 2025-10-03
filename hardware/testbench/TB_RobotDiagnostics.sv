`timescale 1ns/1ps 

module TB_RobotDiagnostics();
    // inputs
    logic sample_clock; // a 50 MHZ clock
    logic uart_rx;
    logic uart_cts;
    logic uart_tx;
    logic uart_rts;
    logic gpio_rx;
    logic gpio_cts;
    logic [9:0] led;
    logic [7:0] write_data;
    logic [9:0] sw; 
    logic key;

    RobotDiagnostics dut(
        .sample_clock(sample_clock), // a 50 MHZ clock
        .key(key), 
        .sw(sw),

        /// uart stuff
        .uart_rx(uart_rx), 
        .uart_tx(uart_tx), 
        .uart_cts(uart_cts), 
        .uart_rts(uart_rts), 

        /// THE GPIO output 
        .gpio_rx(gpio_rx), 
        .gpio_cts(gpio_cts),
        .led(led)
    );

    initial begin 
        forever begin 
            sample_clock = 1'b1; 
            #10
            sample_clock = 1'b0; 
            #10;
        end
    end

    task write_to_interface(); begin 
        uart_rx = 1'b0;
        #8680;
        for(int i = 0;i<8; ++i) begin 
            uart_rx = write_data[i];
            #8680;
        end
        // we have no parity bit, pull with with a one, that is the stop bit
        uart_rx = 1'b1;
        #8680;
    end  endtask 

    task emit_uart(); begin 
        sw[8] = 1'b1;
        #100
        sw[8] = 1'b0;
    end endtask

    initial begin
        sw = 10'b0;
        key = 1'b0; 
        #10;
        key = 1'b1; 
        #10 
        
        uart_rx = 1'b1;
        uart_cts = 1'b0;
        #8680;
        write_data = 8'b10101010;
        write_to_interface();
        
        write_data = 8'b01010101; 
        write_to_interface();

        // tries to emit uart now
        sw[7:0] = 8'b11111111;
        emit_uart();

        write_to_interface();
        write_to_interface();

        sw[7:0] = 8'b01010101;
        emit_uart();

        write_to_interface();
        $stop; 
    end
    
endmodule
