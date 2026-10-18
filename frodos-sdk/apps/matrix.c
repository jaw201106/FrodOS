// apps/matrix.c - Standalone Matrix Terminal screensaver for FrodOS FXE Toolchain
#include "libfrodos.h"

__attribute__((section(".text.main")))
void main(void) {
    sys_clear();
    sys_print("=== Matrix Screen Saver Enabled ===\n");
    sys_print("Press 'q' at any time to exit program stream...\n\n");
    sys_delay(5000000);

    unsigned int pseudo_rand = 42;

    for (;;) {
        // Non-blocking query to monitor for an exit key request
        char input = sys_read_char();
        if (input == 'q' || input == 'Q') {
            break;
        }

        // Advance numbers along a fast chaotic multiplier pattern
        pseudo_rand = (pseudo_rand * 1103515245 + 12345);
        unsigned int column_pick = pseudo_rand % 4;

        if (column_pick == 0)      sys_print(" 0 1 0   1 0   0 1 \n");
        else if (column_pick == 1) sys_print("   1   0 1   1 0   \n");
        else if (column_pick == 2) sys_print(" 0   0   1 0   0 1 \n");
        else                       sys_print("   1 0   0   1   1 \n");

        sys_delay(1500000);
        sys_yield();
    }

    sys_clear();
    sys_print("Matrix screen saver halted.\n");
    sys_delay(3000000);
}
