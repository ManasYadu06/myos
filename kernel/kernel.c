#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "keyboard.h"
#include "shell.h"
#include "pmm.h"
#include "vmm.h"


void kernel_main(unsigned int magic, unsigned int mb_info) {
    (void)mb_info;

    vga_init();

    /* Banner */
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print("  __  __       ____  ____  \n");
    vga_print(" |  \\/  |_   _/ __ \\/ ___| \n");
    vga_print(" | |\\/| | | | | |  | |  \n");
    vga_print(" | |  | | |_| | |__| |___ \n");
    vga_print(" |_|  |_|\\__, |\\____/\\____|\n");
    vga_print("          |___/             \n\n");

    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_print("MyOS v0.1 -- 32-bit x86 Hobby Kernel\n\n");

    /* Init sequence — order is mandatory */
    gdt_init();
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print("[OK] GDT loaded\n");

    idt_init();   /* remaps PIC, installs handlers, calls sti */
    vga_print("[OK] PIC remapped\n");
    vga_print("[OK] IDT loaded\n");
    vga_print("[OK] Interrupts enabled\n");

    keyboard_init();
    vga_print("[OK] Keyboard ready\n");

    pmm_init(mb_info);
    vga_print("[OK] Memory manager ready\n");

    vmm_init();
    vga_print("[OK] Paging enabled\n");

    /* Multiboot magic check */
    if (magic == 0x2BADB002)
        vga_print("[OK] Multiboot magic valid\n");
    else {
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_print("[!!] Multiboot magic invalid\n");
    }

    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("\nMyOS> ");

    shell_run();
}
