; Multiboot header constants
MBALIGN  equ 1 << 0
MEMINFO  equ 1 << 1
FLAGS    equ MBALIGN | MEMINFO
MAGIC    equ 0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; Reserve 16KB kernel stack
section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top      ; set up stack pointer
    push ebx                ; multiboot info struct (arg2)
    push eax                ; multiboot magic number (arg1)
    call kernel_main
    cli                     ; disable interrupts if kernel returns
.hang:
    hlt
    jmp .hang
