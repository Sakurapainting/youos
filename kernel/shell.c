#include <stddef.h>
#include <stdint.h>

#include "heap.h"
#include "keyboard.h"
#include "memory.h"
#include "pit.h"
#include "shell.h"
#include "terminal.h"

#define SHELL_LINE_BUFFER_SIZE 128
#define SHELL_HISTORY_SIZE 8

static char shell_line[SHELL_LINE_BUFFER_SIZE];
static size_t shell_line_len;
static char shell_history[SHELL_HISTORY_SIZE][SHELL_LINE_BUFFER_SIZE];
static size_t shell_history_count;
static size_t shell_history_next;

static int streq(const char* lhs, const char* rhs) {
    size_t i = 0;

    while (lhs[i] != '\0' && rhs[i] != '\0') {
        if (lhs[i] != rhs[i]) {
            return 0;
        }
        i++;
    }

    return lhs[i] == '\0' && rhs[i] == '\0';
}

static int starts_with(const char* text, const char* prefix) {
    size_t i = 0;

    while (prefix[i] != '\0') {
        if (text[i] != prefix[i]) {
            return 0;
        }
        i++;
    }

    return 1;
}

static void shell_prompt(void) {
    terminal_write("youos> ");
}

static void shell_print_help(void) {
    terminal_write("Commands:\n");
    terminal_write("  about  - show build capabilities\n");
    terminal_write("  alloc N- allocate N bytes from heap (16-byte aligned)\n");
    terminal_write("  free X - free allocation by hex address\n");
    terminal_write("  help   - show this help\n");
    terminal_write("  clear  - clear screen\n");
    terminal_write("  heap   - show heap usage\n");
    terminal_write("  meminfo- show memory map summary\n");
    terminal_write("  ticks  - show PIT ticks\n");
    terminal_write("  uptime - show uptime from PIT\n");
    terminal_write("  history- show recent commands\n");
    terminal_write("  panic  - trigger int3 for exception dump\n");
    terminal_write("  echo X - print X\n");
}

static void shell_print_about(void) {
    terminal_write("youOS: i386 educational kernel\n");
    terminal_write("features: IDT, PIC, PIT, PS/2 keyboard, shell, heap, serial debug\n");
}

static void shell_history_push(const char* command) {
    size_t i = 0;

    while (command[i] != '\0' && i < SHELL_LINE_BUFFER_SIZE - 1) {
        shell_history[shell_history_next][i] = command[i];
        i++;
    }
    shell_history[shell_history_next][i] = '\0';

    shell_history_next = (shell_history_next + 1) % SHELL_HISTORY_SIZE;
    if (shell_history_count < SHELL_HISTORY_SIZE) {
        shell_history_count++;
    }
}

static void shell_write_dec_padded3(uint32_t value) {
    terminal_putchar((char)('0' + (value / 100) % 10));
    terminal_putchar((char)('0' + (value / 10) % 10));
    terminal_putchar((char)('0' + value % 10));
}

static void shell_print_uptime(void) {
    uint32_t ticks = pit_get_ticks();
    uint32_t hz = pit_get_frequency_hz();
    uint32_t seconds;
    uint32_t millis;

    if (hz == 0) {
        hz = 100;
    }

    seconds = ticks / hz;
    millis = (ticks % hz) * 1000u / hz;

    terminal_write("uptime: ");
    terminal_write_dec32(seconds);
    terminal_putchar('.');
    shell_write_dec_padded3(millis);
    terminal_write("s (ticks=");
    terminal_write_dec32(ticks);
    terminal_write(", hz=");
    terminal_write_dec32(hz);
    terminal_write(")\n");
}

static void shell_print_meminfo(void) {
    if (!memory_is_ready()) {
        terminal_write("meminfo: unavailable\n");
        return;
    }

    terminal_write("mem total: ");
    terminal_write_dec32(memory_total_kb());
    terminal_write(" KB\n");

    terminal_write("mem avail: ");
    terminal_write_dec32(memory_available_kb());
    terminal_write(" KB\n");

    terminal_write("regions  : ");
    terminal_write_dec32(memory_region_count());
    terminal_putchar('\n');
}

static void shell_print_history(void) {
    size_t i;
    size_t start;

    if (shell_history_count == 0) {
        terminal_write("history: (empty)\n");
        return;
    }

    if (shell_history_count < SHELL_HISTORY_SIZE) {
        start = 0;
    } else {
        start = shell_history_next;
    }

    for (i = 0; i < shell_history_count; i++) {
        size_t index = (start + i) % SHELL_HISTORY_SIZE;
        terminal_write_dec32((uint32_t)(i + 1));
        terminal_write(": ");
        terminal_write(shell_history[index]);
        terminal_putchar('\n');
    }
}

static void shell_print_heap(void) {
    if (!heap_is_ready()) {
        terminal_write("heap: unavailable\n");
        return;
    }

    terminal_write("heap used : ");
    terminal_write_dec32(heap_used_bytes());
    terminal_write(" bytes\n");

    terminal_write("heap total: ");
    terminal_write_dec32(heap_total_bytes());
    terminal_write(" bytes\n");
}

static int shell_parse_uint32(const char* text, uint32_t* value_out) {
    uint32_t value = 0;
    int seen = 0;

    while (*text == ' ' || *text == '\t') {
        text++;
    }

    while (*text >= '0' && *text <= '9') {
        value = (value * 10u) + (uint32_t)(*text - '0');
        text++;
        seen = 1;
    }

    if (!seen) {
        return 0;
    }

    *value_out = value;
    return 1;
}

static int shell_parse_hex32(const char* text, uint32_t* value_out) {
    uint32_t value = 0;
    int seen = 0;

    while (*text == ' ' || *text == '\t') {
        text++;
    }

    if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        text += 2;
    }

    while ((*text >= '0' && *text <= '9') ||
           (*text >= 'a' && *text <= 'f') ||
           (*text >= 'A' && *text <= 'F')) {
        uint32_t digit;

        if (*text >= '0' && *text <= '9') {
            digit = (uint32_t)(*text - '0');
        } else if (*text >= 'a' && *text <= 'f') {
            digit = 10u + (uint32_t)(*text - 'a');
        } else {
            digit = 10u + (uint32_t)(*text - 'A');
        }

        value = (value * 16u) + digit;
        text++;
        seen = 1;
    }

    if (!seen) {
        return 0;
    }

    *value_out = value;
    return 1;
}

static void shell_execute(const char* command) {
    const char* cursor = command;

    while (*cursor == ' ' || *cursor == '\t') {
        cursor++;
    }

    if (*cursor == '\0') {
        return;
    }

    shell_history_push(cursor);

    if (streq(cursor, "about")) {
        shell_print_about();
        return;
    }

    if (starts_with(cursor, "alloc")) {
        uint32_t size = 0;
        void* addr;

        if (!shell_parse_uint32(cursor + 5, &size)) {
            terminal_write("usage: alloc <bytes>\n");
            return;
        }

        addr = heap_alloc(size, 16u);
        if (addr == NULL) {
            terminal_write("alloc failed\n");
            return;
        }

        terminal_write("alloc: addr=0x");
        terminal_write_hex32((uint32_t)(uintptr_t)addr);
        terminal_write(" size=");
        terminal_write_dec32(size);
        terminal_putchar('\n');
        return;
    }

    if (starts_with(cursor, "free")) {
        uint32_t addr = 0;

        if (!shell_parse_hex32(cursor + 4, &addr)) {
            terminal_write("usage: free <hex_addr>\n");
            return;
        }

        heap_free((void*)(uintptr_t)addr);
        terminal_write("free: ok\n");
        return;
    }

    if (streq(cursor, "help")) {
        shell_print_help();
        return;
    }

    if (streq(cursor, "clear")) {
        terminal_clear();
        return;
    }

    if (streq(cursor, "heap")) {
        shell_print_heap();
        return;
    }

    if (streq(cursor, "ticks")) {
        terminal_write("ticks: ");
        terminal_write_dec32(pit_get_ticks());
        terminal_putchar('\n');
        return;
    }

    if (streq(cursor, "uptime")) {
        shell_print_uptime();
        return;
    }

    if (streq(cursor, "history")) {
        shell_print_history();
        return;
    }

    if (streq(cursor, "meminfo")) {
        shell_print_meminfo();
        return;
    }

    if (streq(cursor, "panic")) {
        terminal_write("triggering int3...\n");
        __asm__ volatile ("int $0x03");
        return;
    }

    if (starts_with(cursor, "echo")) {
        const char* message = cursor + 4;

        while (*message == ' ') {
            message++;
        }

        terminal_write(message);
        terminal_putchar('\n');
        return;
    }

    terminal_write("Unknown command: ");
    terminal_write(cursor);
    terminal_putchar('\n');
}

static void shell_on_char(char ch) {
    if (ch == '\r') {
        ch = '\n';
    }

    if (ch == '\n') {
        shell_line[shell_line_len] = '\0';
        terminal_putchar('\n');
        shell_execute(shell_line);
        shell_line_len = 0;
        shell_prompt();
        return;
    }

    if (ch == '\b') {
        if (shell_line_len > 0) {
            shell_line_len--;
            terminal_backspace();
        }
        return;
    }

    if (ch == '\t') {
        return;
    }

    if ((unsigned char)ch < 32 || (unsigned char)ch > 126) {
        return;
    }

    if (shell_line_len >= SHELL_LINE_BUFFER_SIZE - 1) {
        return;
    }

    shell_line[shell_line_len++] = ch;
    terminal_putchar(ch);
}

void shell_init(void) {
    shell_line_len = 0;
    shell_history_count = 0;
    shell_history_next = 0;

    terminal_write("[shell] ready. Type 'help'.\n");
    shell_prompt();
}

void shell_poll(void) {
    char ch;

    while (keyboard_read_char(&ch)) {
        shell_on_char(ch);
    }
}