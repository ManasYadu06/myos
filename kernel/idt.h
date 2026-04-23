#ifndef IDT_H
#define IDT_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_high;
} idt_entry_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint32_t base;
} idt_descriptor_t;

/*
 * Stack layout built by isr.asm (low → high):
 *   gs, fs, es, ds          ← pushed individually
 *   edi,esi,ebp,esp_dummy,
 *   ebx,edx,ecx,eax         ← pusha
 *   int_no, err_code        ← stub pushed
 *   eip, cs, eflags,
 *   esp, ss                 ← CPU pushed
 */
typedef struct __attribute__((packed)) {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp_dummy;
    uint32_t ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, esp, ss;
} registers_t;

void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t sel, uint8_t flags);

#endif
