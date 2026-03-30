#include <stdint.h>

#include "idt.h"
#include "io.h"
#include "keyboard.h"
#include "terminal.h"

#define KEYBOARD_DATA_PORT 0x60

static const char scancode_map[128] = {
    [1] = 27,
    [2] = '1', [3] = '2', [4] = '3', [5] = '4', [6] = '5', [7] = '6',
    [8] = '7', [9] = '8', [10] = '9', [11] = '0', [12] = '-', [13] = '=',
    [14] = '\b', [15] = '\t',
    [16] = 'q', [17] = 'w', [18] = 'e', [19] = 'r', [20] = 't', [21] = 'y',
    [22] = 'u', [23] = 'i', [24] = 'o', [25] = 'p', [26] = '[', [27] = ']',
    [28] = '\n',
    [30] = 'a', [31] = 's', [32] = 'd', [33] = 'f', [34] = 'g', [35] = 'h',
    [36] = 'j', [37] = 'k', [38] = 'l', [39] = ';', [40] = '\'', [41] = '`',
    [43] = '\\',
    [44] = 'z', [45] = 'x', [46] = 'c', [47] = 'v', [48] = 'b', [49] = 'n',
    [50] = 'm', [51] = ',', [52] = '.', [53] = '/',
    [55] = '*', [57] = ' ',
    [74] = '-', [78] = '+'
};

static void keyboard_irq_handler(registers_t* regs) {
    uint8_t scancode;
    char ch;

    (void)regs;

    scancode = inb(KEYBOARD_DATA_PORT);

    if ((scancode & 0x80u) != 0) {
        return;
    }

    ch = scancode_map[scancode];

    if (ch == '\b') {
        return;
    }

    if (ch != 0) {
        terminal_putchar(ch);
    }
}

void keyboard_init(void) {
    register_interrupt_handler(IRQ1, keyboard_irq_handler);
}