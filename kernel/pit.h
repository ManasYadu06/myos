#ifndef PIT_H
#define PIT_H

#include <stdint.h>

/* -----------------------------------------------------------------------
 * 8253/8254 PIT I/O ports
 * ----------------------------------------------------------------------- */
#define PIT_CHANNEL0  0x40   /* IRQ0 counter — system timer              */
#define PIT_CHANNEL2  0x42   /* PC speaker (not used here)               */
#define PIT_CMD       0x43   /* mode/command register (write only)       */

/* -----------------------------------------------------------------------
 * PIT command byte
 *
 * Bits [7:6] — channel select : 00 = channel 0
 * Bits [5:4] — access mode   : 11 = lo/hi byte
 * Bits [3:1] — mode          : 011 = mode 3 (square wave)
 * Bit  [0]   — BCD/binary    : 0   = 16-bit binary
 *
 * Result: 0x36 = 0b00_11_011_0
 * ----------------------------------------------------------------------- */
#define PIT_CMD_BINARY_MODE3  0x36

/* -----------------------------------------------------------------------
 * Base oscillator frequency of the 8254 PIT (Hz)
 * ----------------------------------------------------------------------- */
#define PIT_BASE_FREQ   1193182UL

/* -----------------------------------------------------------------------
 * Desired tick frequency (Hz) and derived divisor
 * ----------------------------------------------------------------------- */
#define PIT_TICK_FREQ   100UL
#define PIT_DIVISOR     (PIT_BASE_FREQ / PIT_TICK_FREQ)

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

/* init PIT: programs channel 0 at PIT_TICK_FREQ Hz, resets tick counter */
void     pit_init(void);

/* called from irq_handler() when int_no == 32 (IRQ0) */
void     pit_handler(void);

/* returns total ticks since pit_init() */
uint32_t pit_get_ticks(void);

/* returns seconds elapsed since pit_init() */
uint32_t pit_get_uptime(void);

#endif /* PIT_H */
