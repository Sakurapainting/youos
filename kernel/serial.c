#include <stdint.h>

#include "io.h"
#include "serial.h"

#define COM1_PORT 0x3F8

static int serial_is_transmit_empty(void) {
    return (inb(COM1_PORT + 5) & 0x20) != 0;
}

void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x80);
    outb(COM1_PORT + 0, 0x03);
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03);
    outb(COM1_PORT + 2, 0xC7);
    outb(COM1_PORT + 4, 0x0B);
}

void serial_write_char(char ch) {
    while (!serial_is_transmit_empty()) {
    }

    outb(COM1_PORT, (uint8_t)ch);
}

void serial_write(const char* str) {
    while (*str != '\0') {
        if (*str == '\n') {
            serial_write_char('\r');
        }

        serial_write_char(*str);
        str++;
    }
}

void serial_write_hex8(uint8_t value) {
    static const char digits[] = "0123456789ABCDEF";
    char buffer[3];

    buffer[0] = digits[(value >> 4) & 0x0F];
    buffer[1] = digits[value & 0x0F];
    buffer[2] = '\0';

    serial_write(buffer);
}

void serial_write_hex32(uint32_t value) {
    static const char digits[] = "0123456789ABCDEF";
    int shift = 28;

    while (shift >= 0) {
        serial_write_char(digits[(value >> (uint32_t)shift) & 0x0F]);
        shift -= 4;
    }
}