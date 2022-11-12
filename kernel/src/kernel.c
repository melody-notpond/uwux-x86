#include <stdint.h>
#include <stddef.h>

#include "console.h"
#include "limine.h"

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent.

static void done(void) {
    for (;;) {
        __asm__("hlt");
    }
}

// The following will be our kernel's entry point.
void _start(void) {
    console_printf("hewo\nmagic number is %i\n", 69);

    // We're done, just hang...
    done();
}