#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/*
 * A GDT entry (descriptor) is exactly 8 bytes.
 * The format is strange because Intel spread the fields
 * across the structure for historical 286 compatibility reasons.
 *
 * Fields:
 *   limit_low   : lower 16 bits of the segment limit
 *   base_low    : lower 16 bits of the base address
 *   base_mid    : middle 8 bits of the base address
 *   access      : access byte — sets privilege level, type (code/data)
 *   granularity : upper 4 bits = flags, lower 4 bits = limit bits 16-19
 *   base_high   : upper 8 bits of the base address
 */
typedef struct __attribute__((packed)) {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} gdt_entry_t;

/*
 * The GDTR register holds a 6-byte descriptor that tells the CPU
 * where the GDT lives in memory and how large it is.
 *   limit : size of the GDT in bytes minus 1
 *   base  : linear address of the GDT
 */
typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint32_t base;
} gdt_descriptor_t;

/* Initialize and load the GDT */
void gdt_init(void);

#endif
