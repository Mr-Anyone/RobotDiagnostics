#define PIXEL_BUF_BASE   0xC8000000UL   /* default — change to your mapping if needed */

#define PIXEL_WIDTH      320U
#define PIXEL_HEIGHT     240U
#define BYTES_PER_PIXEL  2U   
#define PIXEL_BUF_SIZE   (PIXEL_WIDTH * PIXEL_HEIGHT * BYTES_PER_PIXEL)

#define VGA_BUF_REG_ADDR        0xFF203020UL  /* Buffer (read) / swap trigger (write 1) */
#define VGA_BACKBUF_REG_ADDR    0xFF203024UL  /* Backbuffer (read/write) */
#define VGA_RESOLUTION_ADDR     0xFF203028UL  /* resolution read (Y:X) */
#define VGA_STATUS_ADDR         0xFF20302CUL  /* Status register (S bit indicates swap) */

#define VGA_STATUS_S_BIT        (1U << 0)  /* S is LSB in many docs; using mask for clarity */

#define VGA_TASK_STACK_SIZE   (configMINIMAL_STACK_SIZE * 4)
#define VGA_TASK_PRIORITY     (tskIDLE_PRIORITY + 2U)

static inline void mmio_write32(uint32_t addr, uint32_t value) {
    volatile uint32_t *p = (volatile uint32_t *)addr;
    *p = value;
}
static inline uint32_t mmio_read32(uint32_t addr) {
    volatile uint32_t *p = (volatile uint32_t *)addr;
    return *p;
}

static uint32_t front_buffer_phys = PIXEL_BUF_BASE;                /* default */
static uint32_t back_buffer_phys  = PIXEL_BUF_BASE + PIXEL_BUF_SIZE; /* second buffer */

static inline uint16_t rgb_to_565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)((((uint16_t)r & 0xF8) << 8) | (((uint16_t)g & 0xFC) << 3) | (((uint16_t)b) >> 3));
}

static inline void vga_put_pixel_phys(uint32_t buffer_phys, uint32_t x, uint32_t y, uint16_t color) {
    if (x >= PIXEL_WIDTH || y >= PIXEL_HEIGHT) return;
    uint32_t offset_bytes = (y * PIXEL_WIDTH + x) * BYTES_PER_PIXEL;
    volatile uint16_t *pix = (volatile uint16_t *)(buffer_phys + offset_bytes);
    *pix = color;
}

static void vga_clear_buffer_phys(uint32_t buffer_phys, uint16_t color) {
    uint32_t total = PIXEL_WIDTH * PIXEL_HEIGHT;
    volatile uint16_t *p = (volatile uint16_t *)buffer_phys;
    for (uint32_t i = 0; i < total; ++i) {
        p[i] = color;
    }
}

static void vga_fill_rect_phys(uint32_t buffer_phys,
                               uint32_t x0, uint32_t y0,
                               uint32_t w, uint32_t h,
                               uint16_t color)
{
    uint32_t x1 = (x0 + w > PIXEL_WIDTH) ? PIXEL_WIDTH : (x0 + w);
    uint32_t y1 = (y0 + h > PIXEL_HEIGHT) ? PIXEL_HEIGHT : (y0 + h);
    for (uint32_t y = y0; y < y1; ++y) {
        for (uint32_t x = x0; x < x1; ++x) {
            vga_put_pixel_phys(buffer_phys, x, y, color);
        }
    }
}

static int vga_request_swap_and_wait(uint32_t backbuf_phys, TickType_t timeout_ticks) {
    mmio_write32(VGA_BACKBUF_REG_ADDR, backbuf_phys);
    mmio_write32(VGA_BUF_REG_ADDR, 1U);

    TickType_t start = xTaskGetTickCount();
    for (;;) {
        uint32_t status = mmio_read32(VGA_STATUS_ADDR);
        if ((status & VGA_STATUS_S_BIT) == 0) {
            return 0; /* swap complete */
        }
        if (timeout_ticks != portMAX_DELAY) {
            TickType_t now = xTaskGetTickCount();
            if ((now - start) > timeout_ticks) return -1;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void vga_demo_task(void *pvParameters) {
    const TickType_t swap_timeout = pdMS_TO_TICKS(200); /* 200 ms */
    uint16_t bg_color = rgb_to_565(7, 14, 25); /* dark bluish */
    uint16_t rect_color = rgb_to_565(255, 120, 30);

    uint32_t rect_x = 0, rect_y = 0;
    int32_t vx = 2, vy = 1;
    const uint32_t rect_w = 40, rect_h = 24;

    for (;;) {
        vga_clear_buffer_phys(back_buffer_phys, bg_color);

        vga_fill_rect_phys(back_buffer_phys, rect_x, rect_y, rect_w, rect_h, rect_color);

        if (vga_request_swap_and_wait(back_buffer_phys, swap_timeout) == 0) {
            uint32_t old_front = front_buffer_phys;
            front_buffer_phys = back_buffer_phys;
            back_buffer_phys = old_front;

        } else {
        }

        if ((int32_t)rect_x + (int32_t)rect_w >= (int32_t)PIXEL_WIDTH) vx = -abs(vx);
        if ((int32_t)rect_x <= 0) vx = abs(vx);
        if ((int32_t)rect_y + (int32_t)rect_h >= (int32_t)PIXEL_HEIGHT) vy = -abs(vy);
        if ((int32_t)rect_y <= 0) vy = abs(vy);
        rect_x += vx;
        rect_y += vy;

        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

static void vga_init(void) {
    mmio_write32(VGA_BACKBUF_REG_ADDR, back_buffer_phys);

    vga_clear_buffer_phys(front_buffer_phys, rgb_to_565(0,0,0));
    vga_clear_buffer_phys(back_buffer_phys, rgb_to_565(0,0,0));
}