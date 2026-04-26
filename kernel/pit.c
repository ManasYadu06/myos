#include "pit.h"
#include "scheduler.h"

/* -----------------------------------------------------------------------
 * port I/O helpers — mirrors isr.c, no libc
 * ----------------------------------------------------------------------- */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" :: "a"(val), "Nd"(port));
}

/* -----------------------------------------------------------------------
 * Internal tick counter
 * volatile: read from both irq context and kernel/shell context
 * ----------------------------------------------------------------------- */
static volatile uint32_t tick_count = 0;

/* -----------------------------------------------------------------------
 * pit_init()
 *
 * Programs PIT channel 0:
 *   1. Send command byte  → channel 0, lo/hi access, mode 3, binary
 *   2. Send divisor low   → PIT_DIVISOR & 0xFF
 *   3. Send divisor high  → PIT_DIVISOR >> 8
 *
 * Divisor = 1193182 / 100 = 11931  (actual freq ≈ 99.998 Hz, <0.01% error)
 * IRQ0 fires every ~10 ms.
 * ----------------------------------------------------------------------- */
void pit_init(void) {
    tick_count = 0;

    /* send mode command */
    outb(PIT_CMD, PIT_CMD_BINARY_MODE3);

    /* send 16-bit divisor, low byte first then high byte */
    uint16_t divisor = (uint16_t)PIT_DIVISOR;
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)(divisor >> 8));
}

/* -----------------------------------------------------------------------
 * pit_handler()
 *
 * Called by irq_handler() in isr.c on every IRQ0 (int 32).
 * EOI is already sent by irq_handler before dispatch — do NOT send here.
 * Just increment the counter.
 * ----------------------------------------------------------------------- */
void pit_handler(void) {
    tick_count++;
    if (tick_count % SCHED_TICKS == 0)
        scheduler_tick();
}

/* -----------------------------------------------------------------------
 * pit_get_ticks()
 * Returns raw tick count since pit_init().
 * At 100 Hz: overflows uint32_t after ~497 days — fine for a hobby OS.
 * ----------------------------------------------------------------------- */
uint32_t pit_get_ticks(void) {
    return tick_count;
}

/* -----------------------------------------------------------------------
 * pit_get_uptime()
 * Returns whole seconds elapsed since pit_init().
 * ----------------------------------------------------------------------- */
uint32_t pit_get_uptime(void) {
    return tick_count / PIT_TICK_FREQ;
}
