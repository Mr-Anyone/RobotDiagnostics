module UARTTxModule #(
    parameter CLOCK_FREQ = 50_000_000,
    parameter BAUD_RATE = 115_200
)(
    input logic [7:0] send_input,
    input logic valid,
    input logic sample_clock,
    input logic reset,

    output logic uart_tx, 
    output logic uart_rts,
    output logic ready 
);
    localparam DIVISOR = CLOCK_FREQ / BAUD_RATE;

    enum {
        WAITING, 
        SENDING 
    } state;

    logic [8:0] counter; // for clock cycle division passed!
    logic [3:0] bit_counter;
    logic [7:0] send_data;  // the actual valid data that we are SENDING 
                            // Note that send_input is invalid when valid 
                            // is pulled down, so we have this instead
    
    // setting state information 
    always_ff @(posedge sample_clock) begin 
        if(reset) begin 
            counter <= 9'b0; 
            bit_counter <= 4'b0;
            state <= WAITING;
        end else begin 
            case(state)
                WAITING: begin 
                    state <= (valid == 1'b1) ?  SENDING : WAITING; 
                    send_data <= (valid == 1'b1) ? send_input : send_data;
                end
                SENDING:begin 
                    // format: 
                    if(counter == DIVISOR && bit_counter == 4'd9 ) begin 
                        state <= WAITING;
                        counter <= 9'b0; 
                        bit_counter <= 4'b0;
                    end else begin 
                        counter <= (counter == DIVISOR) ? 0 : counter + 1;
                        bit_counter <= (counter == DIVISOR) ? bit_counter + 1 : bit_counter;
                    end
                end
                default: state <= WAITING;
            endcase
        end
    end

    // Setting that data that we are going to send
    always_comb begin 
        case(state)
        SENDING:begin 
            uart_rts = 1'b0; // active low
            
            // | start bit (1 bit) | data (8 bit) | end (1 bit)
            if(bit_counter == 1'b0) begin
                uart_tx = 1'b0;  // the first bit uart is always low
            end else if(bit_counter >= 4'd1 && bit_counter <= 4'd8) begin 
                uart_tx = send_data[bit_counter - 1];
            end else begin 
                uart_tx = 1'b1; // we are sending the final bit, the stop bit
            end
        end
        default: begin 
            uart_tx = 1'b1; 
            uart_rts = 1'b1; // active low
        end
        endcase
    end

    assign ready = (state == WAITING);

endmodule
