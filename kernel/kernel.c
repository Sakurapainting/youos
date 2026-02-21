#include <stdint.h>

void kernel_main(void) {
    // 0xB8000 是 VGA 文本显存地址
    volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;

    const char* str = "Welcome to youOS!";
    for (int i = 0; str[i] != '\0'; i++) {
        // 0x0A 表示黑底绿字
        vga_buffer[i] = (uint16_t)str[i] | (uint16_t)0x0A << 8;
    }
}