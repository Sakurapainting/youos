#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>

void terminal_initialize(void);
void terminal_putchar(char ch);
void terminal_write(const char* str);
void terminal_write_hex8(uint8_t value);

#endif