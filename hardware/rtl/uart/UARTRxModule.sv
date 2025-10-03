
module UARTRxModule #(
    parameter CLOCK_FREQ = 50_000_000,
    parameter BAUD_RATE = 115_200
) (
    input logic uart_rx, 
    input logic uart_cts,
    input logic sample_clock,
    input logic reset,

    output logic [7:0] output_data
);
    enum{
        WAIT_STATE, // waiting for a first active low edge when reading 
        FIND_MIDDLE, // we are finding the middle
        READ_MIDDLE, // getting until we are reading the middle
        READ_CLK, // now we are reading every clock cycle
        READ_END // we are at the end, so we read the fina clock cycle
    } state;

    localparam DIVISOR      = CLOCK_FREQ / BAUD_RATE;
    localparam HALF_DIVISOR =  DIVISOR / 2;
    // ========================================
    // Counter related variables
    logic [8:0] counter; // for how many clock cycle are pass in each stage
    logic [3:0] read_count; // how many data bit have the module read 
    
    // ========================================
    // Reading and writing into buffers
    logic [7:0] current_reading; // the output of the uart interface
    logic [7:0] valid_data; // the current valid buffer
    logic should_flush; // if this is one, we should write to buffer on the next clock cycle
    always_ff @(posedge sample_clock) begin 
        if(reset) begin
            counter <= 8'b0;
            read_count <= 9'b0; 
            current_reading <= 8'b0;
            should_flush <= 1'b0;

            state <= WAIT_STATE; 
        end else begin 
            case(state)
                WAIT_STATE: begin 
                    // we read a zero, it means that we are reading something
                    if(uart_rx == 1'b0) begin 
                        state <= FIND_MIDDLE;
                        should_flush <= 1'b0; 
                    end 
                end
                FIND_MIDDLE: begin
                    if (counter == HALF_DIVISOR) begin 
                        state <= READ_CLK;
                        counter <= 9'b0;
                    end else begin 
                        counter <= counter + 1;
                    end 
                end
                READ_CLK: begin 
                    if(counter == DIVISOR) begin 
                        current_reading[read_count] <= uart_rx;
                        state <= (read_count == 4'd7) ? READ_END : READ_CLK;
                        read_count <= (read_count == 4'd7) ? 4'd0 : read_count + 1;
                        counter <= 9'b0;
                    end else begin
                        counter <= counter + 1;
                    end
                end
                READ_END: begin
                    if(counter == DIVISOR) begin 
                        counter <= 9'b0; 
                        state <= WAIT_STATE;
                        should_flush <= 1'b1;
                    end else begin 
                        counter <= counter + 1;
                    end
                end
                default: state <= WAIT_STATE;            
            endcase
        end 
    end 

    // flusing the data into the valid_data buffer
    always_ff @(posedge sample_clock) begin 
        if(reset)
            valid_data <= 8'b0;
        else if(should_flush) begin 
            valid_data <=  current_reading;
        end
    end

    assign uart_rts = 1'b1; // not ready to send
    assign uart_tx = 1'b1;
    assign output_data = valid_data;
endmodule

