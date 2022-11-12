#include <stdatomic.h>
#include <stddef.h>
#include <stdbool.h>

#include "console.h"
#include "limine.h"

atomic_bool console_lock = false;

static volatile struct limine_terminal_request terminal_request = {
    .id = LIMINE_TERMINAL_REQUEST,
    .revision = 0
};

// console_clear_lock_unsafe() -> void
// Clears the console lock unsafely.
void console_clear_lock_unsafe() {
    console_lock = false;
}

// console_puts(char*) -> void
// Prints out a string onto the terminal.
void console_puts(const char* s) {
    if (terminal_request.response == NULL || terminal_request.response->terminal_count < 1) {
        return;
    }

    bool f = false;
    while (!atomic_compare_exchange_weak(&console_lock, &f, true)) {
        f = false;
    }

    size_t length = 0;
    for (const char* s1 = s; *s1; s1++, length++);
    terminal_request.response->write(terminal_request.response->terminals[0], s, length);

    console_lock = false;
}

typedef enum {
    SIZE_INT,
    SIZE_LONG,
    SIZE_LONG_LONG
} console_printf_size_t;

// console_vprintf(void (*)(char), char*, ...) -> void
// Writes its arguments according to the format string and write function provided. Takes in a va_list.
void console_vprintf(const char* format, va_list va) {
    if (terminal_request.response == NULL || terminal_request.response->terminal_count < 1) {
        return;
    }

    bool f = false;
    while (!atomic_compare_exchange_weak(&console_lock, &f, true)) {
        f = false;
    }

    const char* s = format;
    size_t length = 0;
    for (; *format; format++) {
        // Formatters
        if (*format == '%') {
            terminal_request.response->write(terminal_request.response->terminals[0], s, length);
            format++;

            // Determine the type of the formatter
            console_printf_size_t size = SIZE_INT;
            switch (*format) {
                case 'c': {
                    char c = va_arg(va, int);
                    terminal_request.response->write(terminal_request.response->terminals[0], &c, 1);
                    break;
                }

                case 'p': {
                    unsigned long long p = (unsigned long long) va_arg(va, void*);
                    if (p == 0) {
                        terminal_request.response->write(terminal_request.response->terminals[0], "(null)", 6);
                        break;
                    }

                    static const size_t buffer_size = 18;
                    char buffer[buffer_size];
                    buffer[0] = '0';
                    buffer[1] = 'x';
                    size_t len = 0;
                    while (p != 0) {
                        len++;
                        buffer[buffer_size - len] = "0123456789abcdef"[p % 16];
                        p /= 16;
                    }

                    terminal_request.response->write(terminal_request.response->terminals[0], buffer, buffer_size);
                    break;
                }

                case 's': {
                    char* s = va_arg(va, char*);
                    size_t length = 0;
                    for (char* s1 = s; *s1; s1++, length++);
                    terminal_request.response->write(terminal_request.response->terminals[0], s, length);
                    break;
                }

                case 'l':
                    size = SIZE_LONG;
                    format++;
                    if (*format == 'l') {
                        format++;
                        size = SIZE_LONG_LONG;
                    }

                    if (*format == 'i') {
                        goto console_printf_fallthrough_int;
                    }

                    goto console_printf_fallthrough_hex;

                case 'x':
console_printf_fallthrough_hex: {
                    unsigned long long x;
                    switch (size) {
                        case SIZE_INT:
                            x = va_arg(va, unsigned int);
                            break;
                        case SIZE_LONG:
                            x = va_arg(va, unsigned long);
                            break;
                        case SIZE_LONG_LONG:
                            x = va_arg(va, unsigned long long);
                            break;
                    }

                    if (x == 0) {
                        terminal_request.response->write(terminal_request.response->terminals[0], "0", 1);
                        break;
                    }

                    static const size_t buffer_size = 16;
                    char buffer[buffer_size];
                    size_t len = 0;
                    while (x != 0) {
                        len++;
                        buffer[buffer_size - len] = "0123456789abcdef"[x % 16];
                        x /= 16;
                    }

                    terminal_request.response->write(terminal_request.response->terminals[0], buffer + buffer_size - len, len);
                    break;
                }

                case 'i':
console_printf_fallthrough_int: {
                    unsigned long long x;
                    switch (size) {
                        case SIZE_INT:
                            x = va_arg(va, unsigned int);
                            break;
                        case SIZE_LONG:
                            x = va_arg(va, unsigned long);
                            break;
                        case SIZE_LONG_LONG:
                            x = va_arg(va, unsigned long long);
                            break;
                    }

                    if (x == 0) {
                        terminal_request.response->write(terminal_request.response->terminals[0], "0", 1);
                        break;
                    }

                    static const size_t buffer_size = 20;
                    char buffer[buffer_size];
                    size_t len = 0;
                    while (x != 0) {
                        len++;
                        buffer[buffer_size - len] = "0123456789"[x % 10];
                        x /= 10;
                    }

                    terminal_request.response->write(terminal_request.response->terminals[0], buffer + buffer_size - len, len);
                    break;
                }

                case '%':
                    terminal_request.response->write(terminal_request.response->terminals[0], "%", 1);
                    break;

                default:
                    break;
            }

            s = ++format;
            length = 0;
        } else length++;
    }

    terminal_request.response->write(terminal_request.response->terminals[0], s, length);
    console_lock = false;
}

// console_printf(char*, ...) -> void
// Writes its arguments to the UART port according to the format string and write function provided.
__attribute__((format(printf, 1, 2)))
void console_printf(const char* format, ...) {
    va_list va;
    va_start(va, format);
    console_vprintf(format, va);
    va_end(va);
}

// console_log_16(unsigned long long) -> unsigned int
// Gets the base 16 log of a number as an int.
static unsigned int console_log_16(unsigned long long n) {
    unsigned int i = 0;
    while (n) {
        n >>= 4;
        i++;
    }
    return i;
}

// console_put_hexdump(void*, unsigned long long)
// Dumps a hexdump onto the UART port.
void console_put_hexdump(void* data, unsigned long long size) {
    if (terminal_request.response == NULL || terminal_request.response->terminal_count < 1) {
        return;
    }

    unsigned int num_zeros = console_log_16(size);
    unsigned char* data_char = (unsigned char*) data;

    for (unsigned long long i = 0; i < (size + 15) / 16; i++) {
        // Print out buffer zeroes
        unsigned int num_zeros_two = num_zeros - console_log_16(i) - 1;
        for (unsigned int j = 0; j < num_zeros_two; j++) {
            console_printf("%x", 0);
        }

        // Print out label
        console_printf("%llx    ", i * 16);

        // Print out values
        for (int j = 0; j < 16; j++) {
            unsigned long long index = i * 16 + j;

            // Skip values if the index is greater than the number of values to dump
            if (index >= size)
                console_puts("   ");
            else {
                // Print out the value
                if (data_char[index] < 16)
                    console_printf("%x", 0);
                console_printf("%x ", data_char[index]);
            }
        }

        // Print out characters
        char buffer[17];
        buffer[16] = '\0';
        for (int j = 0; j < 16; j++) {
            unsigned long long index = i * 16 + j;

            // Skip characters if the index is greater than the number of characters to dump
            if (index >= size)
                buffer[j] = '.';

            // Print out printable characters
            else if (32 <= data_char[index] && data_char[index] < 127)
                buffer[j] = data_char[index];

            // Nonprintable characters are represented by a period (.)
            else
                buffer[j] = '.';
        }

        console_printf("    |%s|\n", buffer);
    }

    console_lock = false;
}

