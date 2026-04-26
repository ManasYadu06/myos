; -----------------------------------------------------------------------
; context.asm — cooperative/preemptive context switch for x86 32-bit
;
; void context_switch(cpu_context_t *old_ctx, cpu_context_t *new_ctx);
;
; Calling convention (cdecl):
;   [esp+4] = old_ctx  pointer to cpu_context_t to save INTO
;   [esp+8] = new_ctx  pointer to cpu_context_t to restore FROM
;
; cpu_context_t layout (must match process.h):
;   offset  0 : edi
;   offset  4 : esi
;   offset  8 : ebp
;   offset 12 : esp
;   offset 16 : ebx
;   offset 20 : edx
;   offset 24 : ecx
;   offset 28 : eax
;   offset 32 : eip
;   offset 36 : eflags
; -----------------------------------------------------------------------

[bits 32]
[global context_switch]

context_switch:
    ; --- save old context ---
    mov eax, [esp+4]        ; eax = old_ctx pointer

    ; save general-purpose registers into old_ctx
    mov [eax+0],  edi
    mov [eax+4],  esi
    mov [eax+8],  ebp

    ; save esp BEFORE we touch the stack further
    ; +4 because caller's ret addr is at [esp], we want caller's esp
    lea ecx, [esp+4]
    mov [eax+12], ecx       ; old_ctx->esp = caller's esp

    mov [eax+16], ebx
    mov [eax+20], edx
    ; ecx is scratch above, save the real caller ecx from stack
    ; (cdecl: ecx is caller-saved so we don't need to preserve it,
    ;  but we zero-fill for determinism)
    mov dword [eax+24], 0   ; ecx — caller-saved, skip
    ; eax is clobbered — save 0 (caller-saved anyway)
    mov dword [eax+28], 0   ; eax — caller-saved, skip

    ; save eip = return address (where context_switch was called from)
    mov ecx, [esp]          ; return address on stack
    mov [eax+32], ecx       ; old_ctx->eip

    ; save eflags
    pushfd
    pop ecx
    mov [eax+36], ecx       ; old_ctx->eflags

    ; --- restore new context ---
    mov eax, [esp+8]        ; eax = new_ctx pointer

    ; restore eflags
    mov ecx, [eax+36]
    push ecx
    popfd

    ; restore general-purpose registers
    mov edi, [eax+0]
    mov esi, [eax+4]
    mov ebp, [eax+8]
    mov ebx, [eax+16]
    mov edx, [eax+20]
    mov ecx, [eax+24]
    ; restore eip into a scratch location — jump to it last
    mov esp, [eax+12]       ; switch stack BEFORE any further pushes

    ; jump to new process eip
    ; eax still points to new_ctx — grab eip then overwrite eax
    push dword [eax+32]     ; push new eip onto new stack
    mov eax,  [eax+28]      ; restore eax last

    ret                     ; pops eip → jumps to new process
