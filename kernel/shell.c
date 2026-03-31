#include <stddef.h>
#include <stdint.h>

#include "keyboard.h"
#include "pit.h"
#include "shell.h"
#include "terminal.h"

#define SHELL_LINE_BUFFER_SIZE 128

static char shell_line[SHELL_LINE_BUFFER_SIZE];
static size_t shell_line_len;

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
    terminal_write("  help   - show this help\n");
    terminal_write("  clear  - clear screen\n");
    terminal_write("  ticks  - show PIT ticks\n");
    terminal_write("  echo X - print X\n");
}

static void shell_execute(const char* command) {
    const char* cursor = command;

    while (*cursor == ' ' || *cursor == '\t') {
        cursor++;
    }

    if (*cursor == '\0') {
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

    if (streq(cursor, "ticks")) {
        terminal_write("ticks: ");
        terminal_write_dec32(pit_get_ticks());
        terminal_putchar('\n');
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

    if (ch == '\b' || ch == '\t') {
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

    terminal_write("[shell] ready. Type 'help'.\n");
    shell_prompt();
}

void shell_poll(void) {
    char ch;

    while (keyboard_read_char(&ch)) {
        shell_on_char(ch);
    }
}