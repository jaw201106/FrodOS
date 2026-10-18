#ifndef TIMER_H
#define TIMER_H

typedef __UINT32_TYPE__ uint32_t;

// Standard clock calibration tracking configurations
#define PIT_FREQUENCY_HZ   100
#define PIT_BASE_FREQUENCY 1193182

// Core Kernel Timing Subsystem API Functions
void timer_init(void);
void timer_callback(void);
void sleep(uint32_t ms);
uint32_t get_uptime_ms(void);

#endif
