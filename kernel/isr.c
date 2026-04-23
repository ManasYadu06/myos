#include "isr.h"
#include "idt.h"
#include "vga.h"
#include "keyboard.h"

/* -----------------------------------------------------------------------
 * PIC I/O ports
 * Master PIC: command=0x20, data=0x21
 * Slave  PIC: command=0xA0, data=0xA1
 * ----------------------------------------------------------------------- */
#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI   0x20   /* End-Of-Interrupt command */

#define ICW1_INIT 0x11   /* start init, expect ICW4 */
#define ICW4_8086 0x01   /* 8086 mode               */

/* port I/O helpers — must be inline assembly, no libc available */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" :: "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}
/* brief delay — writing to unused port 0x80 wastes ~1µs */
static inline void io_wait(void) { outb(0x80, 0); }

/* -----------------------------------------------------------------------
 * pic_remap()
 *
 * Default PIC mapping:  IRQ0-7  → int 8-15  (collides with CPU exceptions)
 * After remapping:      IRQ0-7  → int 32-39
 *                       IRQ8-15 → int 40-47
 *
 * Sequence: ICW1 (init) → ICW2 (vector offset) →
 *           ICW3 (cascade wiring) → ICW4 (mode)
 * ----------------------------------------------------------------------- */
static void pic_remap(void) {
    /* save current masks so we don't accidentally unmask everything */
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    /* ICW1 — start initialisation sequence on both PICs */
    outb(PIC1_CMD, ICW1_INIT); io_wait();
    outb(PIC2_CMD, ICW1_INIT); io_wait();

    /* ICW2 — set new vector offsets */
    outb(PIC1_DATA, 0x20); io_wait();   /* master: IRQ0 → int 32 */
    outb(PIC2_DATA, 0x28); io_wait();   /* slave:  IRQ8 → int 40 */

    /* ICW3 — tell master slave is on IRQ2, tell slave its identity */
    outb(PIC1_DATA, 0x04); io_wait();   /* master: slave on IRQ2 (bit 2) */
    outb(PIC2_DATA, 0x02); io_wait();   /* slave:  cascade identity = 2  */

    /* ICW4 — 8086 mode */
    outb(PIC1_DATA, ICW4_8086); io_wait();
    outb(PIC2_DATA, ICW4_8086); io_wait();

    /* restore masks */
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

/* -----------------------------------------------------------------------
 * isr_handler — called by isr_common in isr.asm for exceptions 0-31
 *
 * regs->int_no  : which exception fired
 * regs->err_code: fault address / flags (0 if exception has none)
 * regs->eip     : instruction that caused the fault
 *
 * Currently halts on any exception. Later you can add per-exception
 * recovery (e.g. handle #PF for demand paging).
 * ----------------------------------------------------------------------- */
static const char *exception_names[] = {
    "Division By Zero",        "Debug",
    "Non-Maskable Interrupt",  "Breakpoint",
    "Overflow",                "Bound Range Exceeded",
    "Invalid Opcode",          "Device Not Available",
    "Double Fault",            "Coprocessor Segment Overrun",
    "Invalid TSS",             "Segment Not Present",
    "Stack-Segment Fault",     "General Protection Fault",
    "Page Fault",              "Reserved",
    "x87 Floating Point",      "Alignment Check",
    "Machine Check",           "SIMD Floating Point",
    "Virtualization",          "Reserved",
    "Reserved",                "Reserved",
    "Reserved",                "Reserved",
    "Reserved",                "Reserved",
    "Reserved",                "Reserved",
    "Security Exception",      "Reserved"
};

void isr_handler(registers_t *regs) {
    vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
    vga_print("\n[EXCEPTION #");

    /* print interrupt number as decimal */
    char buf[4];
    uint32_t n = regs->int_no;
    int i = 0;
    if (n == 0) { buf[i++] = '0'; }
    else { while (n) { buf[i++] = '0' + (n % 10); n /= 10; } }
    /* reverse */
    for (int a = 0, b = i-1; a < b; a++, b--) {
        char tmp = buf[a]; buf[a] = buf[b]; buf[b] = tmp;
    }
    buf[i] = '\0';
    vga_print(buf);

    vga_print("] ");
    if (regs->int_no < 32)
        vga_print(exception_names[regs->int_no]);

    vga_print(" -- system halted.\n");

    /* hang forever — no recovery yet */
    for (;;) __asm__ volatile("cli; hlt");
}

/* -----------------------------------------------------------------------
 * irq_handler — called by irq_common in isr.asm for IRQs 32-47
 *
 * MUST send End-Of-Interrupt (EOI) to PIC after every IRQ.
 * Without EOI the PIC thinks we are still busy and masks that
 * IRQ line permanently — the device goes silent.
 *
 * IRQs from slave PIC (int 40-47) need EOI to both slave and master
 * because the slave is wired through master IRQ2 (cascade).
 * ----------------------------------------------------------------------- */
void irq_handler(registers_t *regs) {
    /* send EOI to slave first if IRQ came from slave */
    if (regs->int_no >= 40)
        outb(PIC2_CMD, PIC_EOI);

    /* always send EOI to master */
    outb(PIC1_CMD, PIC_EOI);

    /* per-IRQ dispatch */
    if (regs->int_no == 33)   /* IRQ1 — PS/2 keyboard */
        keyboard_handler(regs);

}

/* -----------------------------------------------------------------------
 * isr_init — install all 48 IDT gates + remap PIC
 * Called from idt_init() before sti
 * ----------------------------------------------------------------------- */
void isr_init(void) {
    pic_remap();

    /* exceptions 0-31 */
    idt_set_gate(0,  (uint32_t)isr0,  0x08, 0x8E);
    idt_set_gate(1,  (uint32_t)isr1,  0x08, 0x8E);
    idt_set_gate(2,  (uint32_t)isr2,  0x08, 0x8E);
    idt_set_gate(3,  (uint32_t)isr3,  0x08, 0x8E);
    idt_set_gate(4,  (uint32_t)isr4,  0x08, 0x8E);
    idt_set_gate(5,  (uint32_t)isr5,  0x08, 0x8E);
    idt_set_gate(6,  (uint32_t)isr6,  0x08, 0x8E);
    idt_set_gate(7,  (uint32_t)isr7,  0x08, 0x8E);
    idt_set_gate(8,  (uint32_t)isr8,  0x08, 0x8E);
    idt_set_gate(9,  (uint32_t)isr9,  0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);

    /* hardware IRQs 32-47 */
    idt_set_gate(32, (uint32_t)irq0,  0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1,  0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2,  0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3,  0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4,  0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5,  0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6,  0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7,  0x08, 0x8E);
    idt_set_gate(40, (uint32_t)irq8,  0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9,  0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);
}
void test_pic_masks(void) {
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);
    vga_print("PIC1 mask: 0x");
    const char *h = "0123456789ABCDEF";
    char b[3] = {h[mask1>>4], h[mask1&0xF], 0};
    vga_print(b); vga_print("  PIC2 mask: 0x");
    char b2[3] = {h[mask2>>4], h[mask2&0xF], 0};
    vga_print(b2); vga_print("\n");
}
