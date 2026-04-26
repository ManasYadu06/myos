; -----------------------------------------------------------------------
; syscall.asm — int 0x80 entry stub
;
; When `int 0x80` fires the CPU pushes (from bottom of stack):
;   ss, esp (only if privilege change), eflags, cs, eip
; Then we push our own registers and call the C handler.
;
; C signature:
;   void syscall_handler(uint32_t syscall_no,
;                        uint32_t arg1, uint32_t arg2, uint32_t arg3);
; Register convention:
;   EAX = syscall number
;   EBX = arg1
;   ECX = arg2
;   EDX = arg3
; -----------------------------------------------------------------------

[bits 32]
[global syscall_stub]
[extern syscall_handler]

syscall_stub:
    ; save scratch registers we'll clobber
    push edx        ; arg3
    push ecx        ; arg2
    push ebx        ; arg1
    push eax        ; syscall number

    call syscall_handler   ; cdecl — args already on stack in order

    ; clean up args (4 x 4 bytes)
    add esp, 16

    iret            ; return to caller (restores eip, cs, eflags)
