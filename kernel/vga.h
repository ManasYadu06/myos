#ifndef VGA_H
#define VGA_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    VGA_BLACK = 0, VGA_BLUE, VGA_GREEN, VGA_CYAN,
    VGA_RED, VGA_MAGENTA, VGA_BROWN, VGA_LIGHT_GREY,
    VGA_DARK_GREY, VGA_LIGHT_BLUE, VGA_LIGHT_GREEN,
    VGA_LIGHT_CYAN, VGA_LIGHT_RED, VGA_LIGHT_MAGENTA,
    VGA_LIGHT_BROWN, VGA_WHITE
} vga_color;

void vga_init(void);
void vga_putchar(char c);
void vga_print(const char* str);
void vga_set_color(vga_color fg, vga_color bg);

#endif
