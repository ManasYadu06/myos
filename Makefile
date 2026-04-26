CC      = i686-elf-gcc
AS      = nasm
LD      = i686-elf-gcc

CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
LDFLAGS = -ffreestanding -O2 -nostdlib -T linker.ld

OBJS    = boot/boot.o       \
          boot/gdt_flush.o  \
          boot/isr.o        \
          kernel/kernel.o   \
          kernel/vga.o      \
          kernel/gdt.o      \
          kernel/idt.o      \
          kernel/isr.o      \
          kernel/keyboard.o \
          kernel/shell.o    \
          kernel/pmm.o      \
          kernel/vmm.o      \
          kernel/pit.o      \
          kernel/heap.o


.PHONY: all clean run iso

all: myos.bin

boot/%.o: boot/%.asm
	$(AS) -f elf32 $< -o $@

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

myos.bin: $(OBJS)
	$(LD) $(LDFLAGS) -lgcc $(OBJS) -o $@
	@grub-file --is-x86-multiboot $@ && echo "[OK] Valid Multiboot binary"

iso: myos.bin
	cp myos.bin iso/boot/myos.bin
	grub-mkrescue -o myos.iso iso
	@echo "[OK] myos.iso ready"

run: iso
	qemu-system-i386 -cdrom myos.iso -monitor stdio

clean:
	rm -f $(OBJS) myos.bin myos.iso iso/boot/myos.bin
