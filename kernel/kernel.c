#include <stdint.h>
#include <stddef.h>

#include "idt.h"
#include "keyboard.h"
#include "memory.h"
#include "pic.h"
#include "pit.h"
#include "serial.h"
#include "shell.h"
#include "terminal.h"

/* ── VGA 文本模式常量 ── */
#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_ADDR    0xB8000

/* ── 终端状态 ── */
static uint16_t* const vga_buffer = (uint16_t*)VGA_ADDR;
static size_t    terminal_row;
static size_t    terminal_col;
static uint8_t   terminal_color;

/* 构造一个 VGA 条目：低 8 位 = ASCII，高 8 位 = 颜色属性 */
static inline uint16_t vga_entry(unsigned char ch, uint8_t color) {
    return (uint16_t)ch | (uint16_t)color << 8;
}

/* ── 滚动：把第 1~24 行上移一行，清空最后一行 ── */
static void terminal_scroll(void) {
    /* 将第 1 行起的内容复制到第 0 行 */
    for (size_t i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }
    /* 最后一行用空格填充 */
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
            vga_entry(' ', terminal_color);
    }
}

/* ── 初始化终端：清屏 + 光标归零 ── */
void terminal_initialize(void) {
    terminal_row   = 0;
    terminal_col   = 0;
    terminal_color = 0x0A;  /* 黑底绿字 */

    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = vga_entry(' ', terminal_color);
    }
}

void terminal_clear(void) {
    terminal_initialize();
}

void terminal_backspace(void) {
    size_t idx;

    if (terminal_col == 0) {
        if (terminal_row == 0) {
            return;
        }

        terminal_row--;
        terminal_col = VGA_WIDTH - 1;
    } else {
        terminal_col--;
    }

    idx = terminal_row * VGA_WIDTH + terminal_col;
    vga_buffer[idx] = vga_entry(' ', terminal_color);
}

/* ── 写单个字符，支持 '\n' 换行 + 自动滚动 ── */
void terminal_putchar(char ch) {
    if (ch == '\n') {
        terminal_col = 0;
        terminal_row++;
    } else {
        size_t idx = terminal_row * VGA_WIDTH + terminal_col;
        vga_buffer[idx] = vga_entry((unsigned char)ch, terminal_color);
        terminal_col++;

        /* 一行写满自动换行 */
        if (terminal_col >= VGA_WIDTH) {
            terminal_col = 0;
            terminal_row++;
        }
    }

    /* 超过屏幕底部 → 滚动 */
    if (terminal_row >= VGA_HEIGHT) {
        terminal_scroll();
        terminal_row = VGA_HEIGHT - 1;
    }
}

/* ── 写字符串 ── */
void terminal_write(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        terminal_putchar(str[i]);
    }
}

void terminal_write_hex8(uint8_t value) {
    static const char digits[] = "0123456789ABCDEF";
    char buffer[3];

    buffer[0] = digits[(value >> 4) & 0x0F];
    buffer[1] = digits[value & 0x0F];
    buffer[2] = '\0';

    terminal_write(buffer);
}

void terminal_write_hex32(uint32_t value) {
    static const char digits[] = "0123456789ABCDEF";
    int shift = 28;

    while (shift >= 0) {
        terminal_putchar(digits[(value >> (uint32_t)shift) & 0x0F]);
        shift -= 4;
    }
}

void terminal_write_dec32(uint32_t value) {
    char buffer[11];
    size_t i = 0;

    if (value == 0) {
        terminal_putchar('0');
        return;
    }

    while (value > 0 && i < sizeof(buffer)) {
        buffer[i++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (i > 0) {
        i--;
        terminal_putchar(buffer[i]);
    }
}

/* ── 内核入口 ── */
void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_addr) {
    terminal_initialize();
    serial_init();

    terminal_write("Welcome to youOS!\n");
    serial_write("Welcome to youOS!\n");
    terminal_write("[init] IDT setup...\n");
    serial_write("[init] IDT setup...\n");

    idt_install();
    pic_remap(0x20, 0x28);

    terminal_write("[init] PIT setup...\n");
    serial_write("[init] PIT setup...\n");
    pit_init(100);

    terminal_write("[init] Memory info...\n");
    serial_write("[init] Memory info...\n");
    memory_init(multiboot_magic, multiboot_addr);

    terminal_write("[init] Keyboard setup...\n");
    serial_write("[init] Keyboard setup...\n");
    keyboard_init();

    /* 仅开启时钟和键盘中断，避免未实现 IRQ 干扰。 */
    pic_clear_mask(0);
    pic_clear_mask(1);

    __asm__ volatile ("sti");

    terminal_write("[ok] IRQ0/IRQ1 enabled.\n");
    serial_write("[ok] IRQ0/IRQ1 enabled.\n");

    if (memory_is_ready()) {
        terminal_write("[ok] Memory map parsed.\n");
        serial_write("[ok] Memory map parsed.\n");
    } else {
        terminal_write("[warn] Memory map unavailable.\n");
        serial_write("[warn] Memory map unavailable.\n");
    }

    shell_init();

    for (;;) {
        __asm__ volatile ("hlt");
        shell_poll();
    }
}