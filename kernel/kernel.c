#include <stdint.h>

void kernel_main(void) {
    // 0xB8000 是 VGA 文本显存地址
    volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;

    // 先清屏：80列 x 25行 = 2000 个字符位，全部填空格（黑底黑字）
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (uint16_t)' ' | (uint16_t)0x0A << 8;
    }

    const char* str = "Welcome to youOS!";
    for (int i = 0; str[i] != '\0'; i++) {
        // 0x0A 表示黑底绿字
        vga_buffer[i] = (uint16_t)str[i] | (uint16_t)0x0A << 8;
    }
}