#pragma once

#include <stdint.h>

typedef enum task_priority_e
{
    T_HIGH = 0,
    T_MEDIUM,
    T_LOW
} task_priority_t;

typedef enum task_state_e
{
    T_SUSPENDED,
    T_BLOCKED,
    T_READY,
    T_RUNNING
} task_state_t;

// position of thread in thread_data_pool, so each is unique if it is never sorted
#define INVALID_HANDLE 0xffffffffu
typedef uint32_t task_handle_t;

typedef void(*task_routine_t)(void);

typedef uint32_t time_t;
