#include "vga.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY (uint16_t*)0xB8000

static uint16_t* vga_buf;
static size_t    vga_row;
static size_t    vga_col;
static uint8_t   vga_clr;

static uint8_t make_color(vga_color fg, vga_color bg) {
    return fg | (bg << 4);
}

static uint16_t make_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

void vga_init(void) {
    vga_buf = VGA_MEMORY;
    vga_row = 0;
    vga_col = 0;
    vga_clr = make_color(VGA_LIGHT_GREY, VGA_BLACK);

    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga_buf[i] = make_entry(' ', vga_clr);
}

void vga_set_color(vga_color fg, vga_color bg) {
    vga_clr = make_color(fg, bg);
}

static void vga_scroll(void) {
    for (size_t row = 1; row < VGA_HEIGHT; row++)
        for (size_t col = 0; col < VGA_WIDTH; col++)
            vga_buf[(row-1)*VGA_WIDTH+col] = vga_buf[row*VGA_WIDTH+col];

    for (size_t col = 0; col < VGA_WIDTH; col++)
        vga_buf[(VGA_HEIGHT-1)*VGA_WIDTH+col] = make_entry(' ', vga_clr);

    vga_row = VGA_HEIGHT - 1;
}

void vga_putchar(char c) {
    if (c == '\n') {
        vga_col = 0;
        if (++vga_row == VGA_HEIGHT) vga_scroll();
        return;
    }
    if (c == '\b') {
        if (vga_col > 0) {
            vga_col--;
            vga_buf[vga_row*VGA_WIDTH+vga_col] = make_entry(' ', vga_clr);
        }
        return;
    }
    vga_buf[vga_row*VGA_WIDTH+vga_col] = make_entry(c, vga_clr);
    if (++vga_col == VGA_WIDTH) {
        vga_col = 0;
        if (++vga_row == VGA_HEIGHT) vga_scroll();
    }
}

void vga_print(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++)
        vga_putchar(str[i]);
}
