#include "gdt.h"

/* We need 3 descriptors: null, kernel code, kernel data */
#define GDT_ENTRIES 3

static gdt_entry_t    gdt[GDT_ENTRIES];
static gdt_descriptor_t gdtr;

/*
 * gdt_set_entry() — fills one descriptor slot.
 *
 * base  : where in memory this segment starts (0 for flat model)
 * limit : how large the segment is (0xFFFFF for full 4GB)
 * access: privilege + type byte
 * flags : granularity + size flags
 *
 * Access byte breakdown (for kernel code = 0x9A):
 *   Bit 7   : Present bit — must be 1 for valid descriptor
 *   Bits 6-5: Privilege level — 00 = Ring 0 (kernel)
 *   Bit 4   : Descriptor type — 1 = code/data segment
 *   Bit 3   : Executable — 1 = code segment, 0 = data segment
 *   Bit 2   : Direction/Conforming — 0 = normal
 *   Bit 1   : Read/Write — 1 = readable (code) or writable (data)
 *   Bit 0   : Accessed — CPU sets this, we initialize to 0
 *
 * So: kernel code  = 1_00_1_1_0_1_0 = 0x9A
 *     kernel data  = 1_00_1_0_0_1_0 = 0x92
 *
 * Granularity byte (0xCF):
 *   Bit 7: Granularity — 1 means limit is in 4KB pages (not bytes)
 *   Bit 6: Size        — 1 means 32-bit protected mode segment
 *   Bit 5: Long mode   — 0 (we are 32-bit)
 *   Bit 4: Reserved    — 0
 *   Bits 3-0: Upper 4 bits of limit
 *   So 0xCF = 1100_1111 → granularity=1, 32-bit, limit bits = 1111
 */
static void gdt_set_entry(int i, uint32_t base, uint32_t limit,
                           uint8_t access, uint8_t flags) {
    gdt[i].base_low    = (base  & 0xFFFF);
    gdt[i].base_mid    = (base  >> 16) & 0xFF;
    gdt[i].base_high   = (base  >> 24) & 0xFF;

    gdt[i].limit_low   = (limit & 0xFFFF);
    /* Upper nibble of granularity byte carries limit bits 16-19 */
    gdt[i].granularity = ((limit >> 16) & 0x0F) | (flags & 0xF0);

    gdt[i].access = access;
}

/*
 * gdt_flush() is defined in gdt_flush.asm.
 * It loads the GDTR register (lgdt instruction) and then
 * performs a far jump to reload the CS (code segment) register,
 * which is the only way to make a new code segment take effect.
 * After the far jump, it reloads all data segment registers
 * (DS, ES, FS, GS, SS) with the new data segment selector.
 */
extern void gdt_flush(uint32_t gdtr_addr);

void gdt_init(void) {
    /* Entry 0: Null descriptor — required by CPU spec, all zeros */
    gdt_set_entry(0, 0, 0, 0, 0);

    /*
     * Entry 1: Kernel Code Segment
     * Base=0, Limit=4GB, Ring 0, executable, readable
     * Selector value = 0x08 (index 1, TI=0, RPL=0)
     */
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0xC0);

    /*
     * Entry 2: Kernel Data Segment
     * Base=0, Limit=4GB, Ring 0, not executable, writable
     * Selector value = 0x10 (index 2, TI=0, RPL=0)
     */
    gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xC0);

    /* Point the GDTR at our table */
    gdtr.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
    gdtr.base  = (uint32_t)&gdt;

    /* Load GDTR and reload segment registers via assembly stub */
    gdt_flush((uint32_t)&gdtr);
}
