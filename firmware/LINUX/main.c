#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <assert.h>

#include "font.h"

#define AXI_BUS_BASE 0xC0000000 
#define LW_BASE      0xff200000

#define VGA_CONTROL_SLAVE_BASE 0x50000
#define WIDTH   640 
#define HEIGHT  480
#define BUFFER_ONE_OFFSET 0x17D000
#define BUFFER_TWO_OFFSET 0x2A9000

#define UART0_BASE   0x1000  
#define UART_RDATA   0x00
#define UART_WDATA   0x04
#define UART_STATUS  0x08
#define UART_CONTROL 0x0C
#define UART_RX_VALID (1 << 0)

void flush_buffer(void* write_to, int* buffer_one, int* buffer_two){
    assert(buffer_one != buffer_two);
    volatile uint32_t* write_mem = (volatile uint32_t*) write_to;
    write_mem[0] = (uint32_t) buffer_two; 
    write_mem[1] = (uint32_t) buffer_one;
}

static inline void put_pixel(int* buffer, int x, int y, uint32_t color){
    if(x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    buffer[y * WIDTH + x] = color;
}


void draw_char(int* buffer, int x, int y, char c, uint32_t color) {
    if (c < 32 || c > 126) return;  // skip non-printable chars
    int index = c - 32;

    for (int row = 0; row < 13; row++) {
        unsigned char bits = font[index][row];
        for (int col = 0; col < 8; col++) {
            if (bits & (1 << (7 - col))) { // MSB is leftmost pixel
                put_pixel(buffer, x + col, y + row, color);
            }
        }
    }
}

void draw_string(int* buffer, int x, int y, const char* str, uint32_t color) {
    while (*str) {
        draw_char(buffer, x, y, *str, color);
        x += 8;  // move 8 pixels right
        str++;
    }
}

char uart_read(volatile uint32_t* uart_base){
    while(!(uart_base[UART_STATUS/4] & UART_RX_VALID)); // wait for data
    return (char)(uart_base[UART_RDATA/4] & 0xFF);
}

void clear_region(int* buffer, int x, int y, int w, int h, uint32_t color){
    for(int j = 0; j < h; j++){
        for(int i = 0; i < w; i++){
            put_pixel(buffer, x+i, y+j, color);
        }
    }
}

int main(){
    int fd;
    if((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1){
        printf("cannot open /dev/mem\n");
        return 0;
    }

    void* axi_base
        = mmap(NULL, 0x500000, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, AXI_BUS_BASE);
    void* axi_slave_address = axi_base + VGA_CONTROL_SLAVE_BASE;
    assert(BUFFER_TWO_OFFSET + (WIDTH*HEIGHT*4) <= (0x500000) && "this must be true");
    
    int* buffer_one_address = (int*)(axi_base + BUFFER_ONE_OFFSET);
    int* buffer_two_address = (int*)(axi_base + BUFFER_TWO_OFFSET);

    // Map UART
    void* lw_base = mmap(NULL, 0x20000, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, LW_BASE);
    volatile uint32_t* uart_base = (volatile uint32_t*)(lw_base + UART0_BASE);

    // clear screen
    for(int i = 0;i<WIDTH*HEIGHT; ++i){
        buffer_one_address[i] = 0x00000000; // black
        buffer_two_address[i] = 0x00000000;
    }
    flush_buffer(axi_slave_address, buffer_one_address, buffer_two_address);

    char uart_buf[64];
    int idx = 0;

    while(1){
        char c = uart_read(uart_base);
        if(c == '\n'){  
            uart_buf[idx] = '\0';
            idx = 0;

            // Convert to float
            float voltage = atof(uart_buf);

            // Clear old value region (top left corner)
            clear_region(buffer_one_address, 0, 0, 200, 16, 0x00000000);

            // Build string
            char display[64];
            snprintf(display, sizeof(display), "Voltage: %.2f V", voltage);

            // Draw text
            draw_string(buffer_one_address, 0, 0, display, 0xFFFFFFFF);

            flush_buffer(axi_slave_address, buffer_one_address, buffer_two_address);
        }
        else if(idx < sizeof(uart_buf)-1){
            uart_buf[idx++] = c;
        }
    }

    return 0;
}