#pragma once

#include <stdint.h>

void kernel_tick(void);

void interrupt_occurred(int);

void kernel_init(void);
void kernel_start(void);
