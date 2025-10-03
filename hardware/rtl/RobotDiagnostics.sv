module RobotDiagnostics(
    // The 50 MHZ clock
    input  logic sample_clock, // a 50 MHZ clock
    // input
    input logic [9:0] sw, // switch on the de1 soc
    input logic key,     // the key 0, on the de1 soc

    // UART
    input  logic uart_rx, 
    output logic uart_tx, 
    input  logic uart_cts, 
    output logic uart_rts, 

    /// THE GPIO output 
    output logic gpio_rx, 
    output logic gpio_cts,
    output logic [9:0] led
); 
    logic reset; 
    assign reset = ~key; // active low for de1-soc

    UARTRxModule #(
        .CLOCK_FREQ(50_000_000),
        .BAUD_RATE(115_200)
    ) uart_read_module (
        .uart_rx(uart_rx), 
        .uart_cts(uart_cts),
        .sample_clock(sample_clock), 
        .reset(reset),
        .output_data(led[7:0])
    );

    logic ready;
    UARTTxModule #(
        .CLOCK_FREQ(50_000_000),
        .BAUD_RATE(115_200)
    ) uart_transmission_module (
        .send_input(sw[7:0]),
        .valid(sw[8]),
        .sample_clock(sample_clock), 
        .reset(reset), 

        .uart_tx(uart_tx),
        .uart_rts(uart_rts), 
        .ready(ready)
    );

    // let the high led to be zeor for now
    assign led[9:8] = 2'b00;
    assign uard_tx = 1'b0;
    

    assign gpio_rx = uart_rx; 
    assign gpio_cts = uart_cts;
endmodule
