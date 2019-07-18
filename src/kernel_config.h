#pragma once

#define MAX_USER_THREADS    8u
#define MAX_USER_TIMERS     8u
// each task has it own timer for sleep
#define MAX_TIMERS          (MAX_USER_THREADS + MAX_USER_TIMERS)
#define MAX_PRIORITIES       3u
#define STACK_SIZE          32u
