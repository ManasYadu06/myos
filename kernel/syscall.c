#include "syscall.h"
#include "idt.h"
#include "vga.h"
#include "process.h"

/* -----------------------------------------------------------------------
 * syscall_stub — defined in syscall.asm, registered as int 0x80 handler
 * ----------------------------------------------------------------------- */
extern void syscall_stub(void);

/* -----------------------------------------------------------------------
 * syscall_init — register int 0x80 as trap gate (DPL=3 so user can call)
 *
 * Gate type 0x8F:
 *   1000 = present, ring 0 descriptor
 *   1111 = 32-bit trap gate (IF not cleared on entry, unlike int gate 0x8E)
 * DPL bits [14:13] = 11 → ring 3 can issue int 0x80
 * Full flags byte: 1_11_01111 = 0xEF
 * ----------------------------------------------------------------------- */
void syscall_init(void) {
    idt_set_gate(0x80, (uint32_t)syscall_stub, 0x08, 0xEF);
}

/* -----------------------------------------------------------------------
 * sys_write — print null-terminated string to VGA
 * arg1 = pointer to string
 * returns: number of characters written
 * ----------------------------------------------------------------------- */
uint32_t sys_write(const char *msg) {
    if (!msg) return 0;
    uint32_t count = 0;
    while (msg[count]) count++;
    vga_print(msg);
    return count;
}

/* -----------------------------------------------------------------------
 * sys_getpid — return PID of current process
 * ----------------------------------------------------------------------- */
uint32_t sys_getpid(void) {
    return process_current()->pid;
}

/* -----------------------------------------------------------------------
 * sys_exit — terminate current process
 * arg1 = exit code (stored in ticks field for now, no wait() yet)
 * ----------------------------------------------------------------------- */
void sys_exit(uint32_t code) {
    process_t *p = process_current();
    p->ticks = code;   /* stash exit code */
    process_exit();    /* marks zombie, halts */
}

/* -----------------------------------------------------------------------
 * syscall_handler — dispatches by syscall number
 * Called from syscall_stub with args already on stack (cdecl)
 * Return value goes into EAX via normal C calling convention
 * ----------------------------------------------------------------------- */
void syscall_handler(uint32_t syscall_no,
                     uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    (void)arg2; (void)arg3;   /* unused by current syscalls */

    switch (syscall_no) {
        case SYS_WRITE:
            sys_write((const char *)arg1);
            break;

        case SYS_GETPID:
            sys_getpid();
            break;

        case SYS_EXIT:
            sys_exit(arg1);
            break;

        default:
            vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
            vga_print("[SYSCALL] unknown number\n");
            vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
            break;
    }
}
