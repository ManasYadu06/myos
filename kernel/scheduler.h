#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

/* -----------------------------------------------------------------------
 * Scheduler fires every SCHED_TICKS PIT ticks (at 100 Hz → every 50ms)
 * ----------------------------------------------------------------------- */
#define SCHED_TICKS  5

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

/* call once after process_init() */
void scheduler_init(void);

/* called from pit_handler() every SCHED_TICKS ticks — picks next process */
void scheduler_tick(void);

#endif /* SCHEDULER_H */
