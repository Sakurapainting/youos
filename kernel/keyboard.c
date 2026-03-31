#include <stdint.h>

#include "idt.h"
#include "io.h"
#include "keyboard.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_BUFFER_SIZE 128

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

static volatile uint8_t keyboard_head;
static volatile uint8_t keyboard_tail;
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];

static void keyboard_buffer_push(char ch) {
    uint8_t next_head = (uint8_t)((keyboard_head + 1) & (KEYBOARD_BUFFER_SIZE - 1));

    if (next_head == keyboard_tail) {
        return;
    }

    keyboard_buffer[keyboard_head] = ch;
    keyboard_head = next_head;
}

static void keyboard_irq_handler(registers_t* regs) {
    uint8_t scancode;
    char ch;

    (void)regs;

    scancode = inb(KEYBOARD_DATA_PORT);

    if ((scancode & 0x80u) != 0) {
        return;
    }

    ch = scancode_map[scancode];

    if (ch != 0) {
        keyboard_buffer_push(ch);
    }
}

void keyboard_init(void) {
    keyboard_head = 0;
    keyboard_tail = 0;
    register_interrupt_handler(IRQ1, keyboard_irq_handler);
}

int keyboard_read_char(char* out_char) {
    if (keyboard_head == keyboard_tail) {
        return 0;
    }

    *out_char = keyboard_buffer[keyboard_tail];
    keyboard_tail = (uint8_t)((keyboard_tail + 1) & (KEYBOARD_BUFFER_SIZE - 1));
    return 1;
}