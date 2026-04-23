; gdt_flush.asm
; Called from C as: gdt_flush(uint32_t gdtr_addr)
;
; The C compiler passes the argument on the stack.
; We load it into EAX, call lgdt, then do a far jump to reload CS.
;
; Why a far jump? The lgdt instruction loads the GDTR register,
; but the CPU caches the old code segment descriptor internally.
; The only way to force it to re-read the descriptor from the new
; GDT is to perform a far jump (ljmp) that explicitly specifies
; the new segment selector. Format: jmp selector:offset
; 0x08 is our kernel code segment (GDT entry 1 × 8 bytes = 0x08)

global gdt_flush

gdt_flush:
    mov  eax, [esp+4]   ; grab the gdtr pointer argument
    lgdt [eax]          ; load the GDT register

    ; Far jump to reload CS with our new code segment selector (0x08)
    jmp 0x08:.flush

.flush:
    ; Reload all data segment registers with data segment selector
    ; 0x10 = GDT entry 2 (index 2 × 8 = 16 = 0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret
