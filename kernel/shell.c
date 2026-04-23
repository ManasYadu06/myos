#include "shell.h"
#include "vga.h"
#include "keyboard.h"

#define BUF_SIZE 256

/* -----------------------------------------------------------------------
 * Internal state
 * buf     : current line being typed
 * buf_pos : how many characters are in buf
 * ----------------------------------------------------------------------- */
static char     buf[BUF_SIZE];
static uint32_t buf_pos;

/* -----------------------------------------------------------------------
 * String helpers — no libc available
 * ----------------------------------------------------------------------- */
static int sh_strcmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

static int sh_strncmp(const char *a, const char *b, uint32_t n) {
    while (n && *a && *b && *a == *b) { a++; b++; n--; }
    if (!n) return 0;
    return *a - *b;
}

static uint32_t sh_strlen(const char *s) {
    uint32_t i = 0;
    while (s[i]) i++;
    return i;
}

/* -----------------------------------------------------------------------
 * Commands
 * ----------------------------------------------------------------------- */
static void cmd_help(void) {
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_print("Available commands:\n");
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_print("  help    ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("-- show this message\n");
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_print("  clear   ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("-- clear the screen\n");
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_print("  echo    ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("-- echo text  e.g. echo hello\n");
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_print("  meminfo ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("-- show memory info\n");
}

static void cmd_clear(void) {
    vga_init();   /* re-init clears and resets cursor to 0,0 */
}

static void cmd_echo(const char *args) {
    if (args && *args) {
        vga_print(args);
        vga_putchar('\n');
    }
}

static void cmd_meminfo(void) {
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("Memory manager not yet initialized.\n");
    vga_print("Phase 5 will add physical memory tracking.\n");
}

/* -----------------------------------------------------------------------
 * parse_and_exec — split command from args, dispatch
 * ----------------------------------------------------------------------- */
static void parse_and_exec(void) {
    /* skip leading spaces */
    uint32_t i = 0;
    while (buf[i] == ' ') i++;

    if (buf[i] == '\0') return;   /* empty line */

    const char *cmd  = buf + i;

    /* find end of command word */
    uint32_t cmd_len = 0;
    while (buf[i + cmd_len] && buf[i + cmd_len] != ' ') cmd_len++;

    /* find start of arguments (skip spaces after command) */
    uint32_t arg_start = i + cmd_len;
    while (buf[arg_start] == ' ') arg_start++;
    const char *args = buf + arg_start;

    /* dispatch */
    if (sh_strncmp(cmd, "help",    cmd_len) == 0 && cmd_len == 4)
        cmd_help();
    else if (sh_strncmp(cmd, "clear",   cmd_len) == 0 && cmd_len == 5)
        cmd_clear();
    else if (sh_strncmp(cmd, "echo",    cmd_len) == 0 && cmd_len == 4)
        cmd_echo(args);
    else if (sh_strncmp(cmd, "meminfo", cmd_len) == 0 && cmd_len == 7)
        cmd_meminfo();
    else {
        /* unknown command */
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_print("Unknown command: ");
        /* print just the command word */
        for (uint32_t j = 0; j < cmd_len; j++)
            vga_putchar(cmd[j]);
        vga_putchar('\n');
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        vga_print("Type 'help' for available commands.\n");
    }
}

/* -----------------------------------------------------------------------
 * shell_input — called by keyboard layer when a character is ready
 *
 * Instead of keyboard printing directly to VGA, the shell takes
 * ownership: it echoes to screen AND manages the input buffer.
 * ----------------------------------------------------------------------- */
void shell_input(char c) {
    if (c == '\n') {
        vga_putchar('\n');
        buf[buf_pos] = '\0';
        parse_and_exec();
        /* reset buffer */
        buf_pos = 0;
        buf[0]  = '\0';
        /* reprint prompt */
        vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        vga_print("MyOS> ");
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    } else if (c == '\b') {
        if (buf_pos > 0) {
            buf_pos--;
            buf[buf_pos] = '\0';
            vga_putchar('\b');
        }

    } else {
        if (buf_pos < BUF_SIZE - 1) {
            buf[buf_pos++] = c;
            buf[buf_pos]   = '\0';
            vga_putchar(c);   /* echo to screen */
        }
    }
}

/* -----------------------------------------------------------------------
 * shell_run — the read-eval-print loop
 * Prints the prompt then halts waiting for IRQ1 to deliver characters
 * via shell_input(). Never returns.
 * ----------------------------------------------------------------------- */
void shell_run(void) {
    buf_pos = 0;
    buf[0]  = '\0';

    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print("MyOS> ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    for (;;) __asm__ volatile("hlt");
}
