#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

/* -----------------------------------------------------------------------
 * Syscall numbers — passed in EAX before `int 0x80`
 * ----------------------------------------------------------------------- */
#define SYS_EXIT     0
#define SYS_WRITE    1
#define SYS_GETPID   2

/* -----------------------------------------------------------------------
 * Calling convention (Linux-style, ring 0 only for now):
 *   EAX = syscall number
 *   EBX = arg1
 *   ECX = arg2
 *   EDX = arg3
 *   return value in EAX
 * ----------------------------------------------------------------------- */

/* install int 0x80 trap gate — call from isr_init() */
void syscall_init(void);

/* C handler — called from syscall_stub in isr.asm */
void syscall_handler(uint32_t syscall_no,
                     uint32_t arg1, uint32_t arg2, uint32_t arg3);

/* -----------------------------------------------------------------------
 * Kernel-side syscall implementations
 * ----------------------------------------------------------------------- */
void     sys_exit(uint32_t code);
uint32_t sys_write(const char *msg);
uint32_t sys_getpid(void);

#endif /* SYSCALL_H */
