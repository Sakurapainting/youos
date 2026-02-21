CC = i686-elf-gcc
AS = nasm
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
LDFLAGS = -ffreestanding -O2 -nostdlib -lgcc

all: youos.bin

youos.bin: build/boot.o build/kernel.o
	$(CC) -T scripts/linker.ld -o build/youos.bin $(LDFLAGS) build/boot.o build/kernel.o

build/boot.o: arch/i386/boot.s
	$(AS) -felf32 arch/i386/boot.s -o build/boot.o

build/kernel.o: kernel/kernel.c
	$(CC) -c kernel/kernel.c -o build/kernel.o $(CFLAGS)

run: youos.bin
	qemu-system-i386 -kernel build/youos.bin

clean:
	rm -rf build/*.o build/*.bin