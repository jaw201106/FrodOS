#include "io.h"

// Types matching freestanding architectures cleanly
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT8_TYPE__  uint8_t;

// Standard clock tracking configurations
#define PIT_FREQUENCY_HZ 100
#define PIT_BASE_FREQUENCY 1193182

// Global runtime ticks (volatile ensures it survives compiler -O2 optimization)
volatile uint32_t system_ticks = 0;

// 1. Asynchronous Hardware Interrupt Routine
void timer_callback(void) {
    system_ticks++;

    // Optional: Inject background execution hooks here
    // e.g., if (system_ticks % 50 == 0) blink_vga_cursor();
}

// 2. Robust Hardware Channel Allocation
void timer_init(void) {
    uint32_t divisor = PIT_BASE_FREQUENCY / PIT_FREQUENCY_HZ;

    // Command Register (0x43):
    // Bit 7-6: 00 (Select Channel 0)
    // Bit 5-4: 11 (Access Mode: Lobyte/Hibyte)
    // Bit 3-1: 011 (Operating Mode 3: Square Wave Generator)
    // Bit 0  : 0 (Binary Counter)
    outb(0x43, 0x36);

    // Split divisor cleanly across target data port lanes
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

// 3. Robust Thread Delay Loop (With explicit hardware memory barriers)
void sleep(uint32_t ms) {
    // Each tick equals exactly 10ms (at 100Hz baseline frequency)
    uint32_t ticks_to_wait = ms / 10;

    // Prevent short delay truncation drops (Force at least 1 tick)
    if (ticks_to_wait == 0 && ms > 0) {
        ticks_to_wait = 1;
    }

    uint32_t target_ticks = system_ticks + ticks_to_wait;

    while (system_ticks < target_ticks) {
        // Safe, energy-efficient idle state keeps CPU from overheating
        asm volatile("hlt");

        // !!! THE MEMORY BARRIER CRITICAL PIECE !!!
        // This explicitly forbids GCC from optimizing this loop away under -O2.
        // It guarantees the compiler re-reads system_ticks directly from RAM.
        asm volatile("" : : : "memory");
    }
}

// 4. Time Metric Helper Engine
uint32_t get_uptime_ms(void) {
    // Memory barrier ensures value is snapshotted atomically
    asm volatile("" : : : "memory");
    return system_ticks * 10;
}
