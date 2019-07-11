#pragma once

#include <stdint.h>

typedef enum thread_priority_e
{
    T_HIGH = 0,
    T_MEDIUM,
    T_LOW
} thread_priority_t;

typedef enum thread_state_e
{
    T_SUSPENDED,
    T_BLOCKED,
    T_READY,
    T_RUNNING
} thread_state_t;

// position of thread in thread_data_pool, so each is unique if it is never sorted
typedef uint32_t thread_handle_t;

typedef void(*thread_routine_t)(void);

typedef uint32_t time_t;

// user api
time_t GetTime(void);

int createThread(thread_routine_t _routine, thread_priority_t _priority, thread_handle_t * _handle);
//  void TerminateThread(void);
void terminateThread(thread_handle_t _handle);
