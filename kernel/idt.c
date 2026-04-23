#include "idt.h"
#include "isr.h"

#define IDT_ENTRIES 256

static idt_entry_t     idt[IDT_ENTRIES];
static idt_descriptor_t idtr;

extern void idt_flush(uint32_t idtr_addr);  /* defined in boot/isr.asm */

/*
 * idt_set_gate — fill one IDT slot
 *
 * num     : interrupt number (0-255)
 * handler : address of the ASM stub (e.g. isr0, irq1)
 * sel     : code segment selector — always 0x08 (kernel code)
 * flags   : type_attr byte — 0x8E for interrupt gate
 *
 * The handler address is split across offset_low and offset_high
 * because Intel's 8-byte gate format was extended from an old
 * 4-byte 286 format. The split is mandatory — hardware reads it.
 */
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t sel, uint8_t flags) {
    idt[num].offset_low  =  handler        & 0xFFFF;
    idt[num].offset_high = (handler >> 16) & 0xFFFF;
    idt[num].selector    = sel;
    idt[num].zero        = 0;
    idt[num].type_attr   = flags;
}

/*
 * idt_init — zero table, install all gates, load IDTR, enable interrupts
 *
 * Order matters:
 *   1. Zero all entries (Present=0 means CPU ignores unused slots)
 *   2. isr_init() remaps PIC then installs all 48 gates
 *   3. lidt via idt_flush — CPU now knows where the table lives
 *   4. sti — safe to enable interrupts only after all three above
 */
void idt_init(void) {
    for (int i = 0; i < IDT_ENTRIES; i++)
        idt_set_gate(i, 0, 0, 0);

    isr_init();

    idtr.limit = (sizeof(idt_entry_t) * IDT_ENTRIES) - 1;
    idtr.base  = (uint32_t)&idt;
    idt_flush((uint32_t)&idtr);

    __asm__ volatile("sti");
}
