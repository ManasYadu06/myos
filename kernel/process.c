#include "process.h"
#include "heap.h"

/* -----------------------------------------------------------------------
 * Global process table and current PID
 * ----------------------------------------------------------------------- */
process_t proc_table[MAX_PROCS];
uint32_t  current_pid = 0;

/* -----------------------------------------------------------------------
 * idle_task — runs when no other process is READY
 * Burns CPU doing nothing; HLT would stop PIT IRQ0 from firing
 * ----------------------------------------------------------------------- */
static void idle_task(void) {
    for (;;) __asm__ volatile("nop");
}

/* -----------------------------------------------------------------------
 * process_init — zero table, create idle process (pid 0)
 * ----------------------------------------------------------------------- */
void process_init(void) {
    /* zero entire table */
    for (int i = 0; i < MAX_PROCS; i++) {
        proc_table[i].pid   = 0;
        proc_table[i].state = PROC_UNUSED;
        proc_table[i].ticks = 0;
    }

    /* create idle process in slot 0 */
    process_t *idle    = &proc_table[0];
    idle->pid          = 0;
    idle->state        = PROC_RUNNING;
    idle->ticks        = 0;
    idle->name[0]      = 'i'; idle->name[1] = 'd';
    idle->name[2]      = 'l'; idle->name[3] = 'e';
    idle->name[4]      = '\0';

    /* stack grows down — set esp to top of stack array */
    idle->ctx.esp    = (uint32_t)(&idle->stack[1024]);
    idle->ctx.eip    = (uint32_t)idle_task;
    idle->ctx.eflags = 0x202;   /* IF=1, reserved bit 1 set */

    current_pid = 0;
}

/* -----------------------------------------------------------------------
 * process_create — find free slot, set up stack frame for context_switch
 *
 * Stack primed so context_restore in context.asm can "return" into entry:
 *   [top]  entry          ← popped as eip by ret in context_restore
 *          0              ← fake return address (process must not return)
 *          eflags
 *          eax..edi
 * ----------------------------------------------------------------------- */
int process_create(void (*entry)(void), const char *name) {
    /* find free slot (skip 0 — idle) */
    int slot = -1;
    for (int i = 1; i < MAX_PROCS; i++) {
        if (proc_table[i].state == PROC_UNUSED) { slot = i; break; }
    }
    if (slot < 0) return -1;   /* table full */

    process_t *p = &proc_table[slot];

    /* zero the PCB */
    for (uint32_t i = 0; i < sizeof(process_t) / 4; i++)
        ((uint32_t *)p)[i] = 0;

    p->pid   = (uint32_t)slot;
    p->state = PROC_READY;
    p->ticks = 0;

    /* copy name */
    int i = 0;
    while (name[i] && i < 31) { p->name[i] = name[i]; i++; }
    p->name[i] = '\0';

    /* prime the kernel stack — context_restore pops these */
    uint32_t *sp = &p->stack[1024];   /* top of stack */

    *(--sp) = (uint32_t)entry;   /* eip — where to jump           */
    *(--sp) = 0;                 /* fake return addr (never used)  */
    *(--sp) = 0x202;             /* eflags: IF=1                   */
    *(--sp) = 0;                 /* eax */
    *(--sp) = 0;                 /* ecx */
    *(--sp) = 0;                 /* edx */
    *(--sp) = 0;                 /* ebx */
    *(--sp) = 0;                 /* ebp */
    *(--sp) = 0;                 /* esi */
    *(--sp) = 0;                 /* edi */

    p->ctx.esp = (uint32_t)sp;

    return slot;
}

/* -----------------------------------------------------------------------
 * process_exit — mark current process zombie; scheduler will skip it
 * ----------------------------------------------------------------------- */
void process_exit(void) {
    proc_table[current_pid].state = PROC_ZOMBIE;
    for (;;) __asm__ volatile("hlt");
}

/* -----------------------------------------------------------------------
 * process_current — return pointer to running PCB
 * ----------------------------------------------------------------------- */
process_t *process_current(void) {
    return &proc_table[current_pid];
}
