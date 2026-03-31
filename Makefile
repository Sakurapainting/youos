CC = i686-elf-gcc
AS = nasm
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Ikernel
LDFLAGS = -ffreestanding -O2 -nostdlib -lgcc
OBJS = build/boot.o build/isr.o build/kernel.o build/idt.o build/pic.o build/pit.o build/keyboard.o build/shell.o build/serial.o

all: youos.bin

youos.bin: $(OBJS)
	$(CC) -T scripts/linker.ld -o build/youos.bin $(LDFLAGS) $(OBJS)

build/boot.o: arch/i386/boot.s | build-dir
	$(AS) -felf32 arch/i386/boot.s -o build/boot.o

build/isr.o: arch/i386/isr.s | build-dir
	$(AS) -felf32 arch/i386/isr.s -o build/isr.o

build/kernel.o: kernel/kernel.c kernel/terminal.h kernel/idt.h kernel/pic.h kernel/pit.h kernel/keyboard.h kernel/shell.h kernel/serial.h | build-dir
	$(CC) -c kernel/kernel.c -o build/kernel.o $(CFLAGS)

build/idt.o: kernel/idt.c kernel/idt.h kernel/terminal.h kernel/pic.h kernel/serial.h | build-dir
	$(CC) -c kernel/idt.c -o build/idt.o $(CFLAGS)

build/pic.o: kernel/pic.c kernel/pic.h kernel/io.h | build-dir
	$(CC) -c kernel/pic.c -o build/pic.o $(CFLAGS)

build/pit.o: kernel/pit.c kernel/pit.h kernel/idt.h kernel/io.h | build-dir
	$(CC) -c kernel/pit.c -o build/pit.o $(CFLAGS)

build/keyboard.o: kernel/keyboard.c kernel/keyboard.h kernel/idt.h kernel/io.h | build-dir
	$(CC) -c kernel/keyboard.c -o build/keyboard.o $(CFLAGS)

build/shell.o: kernel/shell.c kernel/shell.h kernel/keyboard.h kernel/pit.h kernel/terminal.h | build-dir
	$(CC) -c kernel/shell.c -o build/shell.o $(CFLAGS)

build/serial.o: kernel/serial.c kernel/serial.h kernel/io.h | build-dir
	$(CC) -c kernel/serial.c -o build/serial.o $(CFLAGS)

build-dir:
	mkdir -p build

run:
	qemu-system-i386 -kernel build/youos.bin

clean:
	rm -rf build/*.o build/*.bin

.PHONY: all run clean build-dir