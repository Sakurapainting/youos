#include <stdint.h>

#include "idt.h"
#include "io.h"
#include "pit.h"

#define PIT_COMMAND 0x43
#define PIT_CHANNEL0 0x40
#define PIT_BASE_FREQUENCY 1193180u

static volatile uint32_t pit_ticks = 0;
static uint32_t pit_frequency_hz = 100;

static void pit_irq_handler(registers_t* regs) {
    (void)regs;
    pit_ticks++;
}

void pit_init(uint32_t frequency_hz) {
    uint32_t divisor;

    if (frequency_hz == 0) {
        frequency_hz = 100;
    }

    pit_frequency_hz = frequency_hz;

    divisor = PIT_BASE_FREQUENCY / frequency_hz;

    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    register_interrupt_handler(IRQ0, pit_irq_handler);
}

uint32_t pit_get_ticks(void) {
    return pit_ticks;
}

uint32_t pit_get_frequency_hz(void) {
    return pit_frequency_hz;
}