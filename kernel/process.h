#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

/* -----------------------------------------------------------------------
 * Process states
 * ----------------------------------------------------------------------- */
#define PROC_UNUSED   0
#define PROC_READY    1
#define PROC_RUNNING  2
#define PROC_BLOCKED  3
#define PROC_ZOMBIE   4

#define MAX_PROCS     16

/* -----------------------------------------------------------------------
 * CPU register context — matches push order in context.asm
 * ----------------------------------------------------------------------- */
typedef struct {
    uint32_t edi, esi, ebp, esp;
    uint32_t ebx, edx, ecx, eax;
    uint32_t eip, eflags;
} cpu_context_t;

/* -----------------------------------------------------------------------
 * Process Control Block
 * ----------------------------------------------------------------------- */
typedef struct process {
    uint32_t      pid;
    uint32_t      state;
    cpu_context_t ctx;
    uint32_t      stack[1024];   /* 4KB kernel stack, grows down */
    char          name[32];
    uint32_t      ticks;         /* CPU ticks consumed */
} process_t;

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */
void       process_init(void);
int        process_create(void (*entry)(void), const char *name);
void       process_exit(void);
process_t *process_current(void);

extern process_t proc_table[MAX_PROCS];
extern uint32_t  current_pid;

#endif /* PROCESS_H */
