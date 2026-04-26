#include "scheduler.h"
#include "process.h"

/* -----------------------------------------------------------------------
 * context_switch — defined in context.asm
 * Saves current context into *old, restores from *next, jumps to next->eip
 * ----------------------------------------------------------------------- */
extern void context_switch(cpu_context_t *old_ctx, cpu_context_t *new_ctx);

static uint32_t sched_enabled = 0;

/* -----------------------------------------------------------------------
 * scheduler_init — enable scheduling (call after process_init)
 * ----------------------------------------------------------------------- */
void scheduler_init(void) {
    sched_enabled = 1;
}

/* -----------------------------------------------------------------------
 * scheduler_tick — round-robin, called from pit_handler every SCHED_TICKS
 *
 * 1. Increment ticks for current process
 * 2. Walk table from current+1, find next PROC_READY process
 * 3. If none found (only idle), stay on current
 * 4. context_switch old → new
 * ----------------------------------------------------------------------- */
void scheduler_tick(void) {
    if (!sched_enabled) return;

    process_t *cur = &proc_table[current_pid];
    cur->ticks++;

    /* find next READY process (round-robin, skip zombies/unused) */
    uint32_t next_pid = current_pid;
    for (int i = 1; i <= MAX_PROCS; i++) {
        uint32_t candidate = (current_pid + i) % MAX_PROCS;
        if (proc_table[candidate].state == PROC_READY) {
            next_pid = candidate;
            break;
        }
    }

    if (next_pid == current_pid) {
        /* no other ready process — if current is zombie, run idle */
        if (cur->state == PROC_ZOMBIE) next_pid = 0;
        else return;   /* keep running current */
    }

    /* transition states */
    if (cur->state == PROC_RUNNING)
        cur->state = PROC_READY;

    proc_table[next_pid].state = PROC_RUNNING;

    cpu_context_t *old_ctx = &cur->ctx;
    current_pid = next_pid;
    cpu_context_t *new_ctx = &proc_table[next_pid].ctx;

    context_switch(old_ctx, new_ctx);
}
