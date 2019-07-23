#pragma once

#include "kernel_types.h"
#include "arbiter.h"

typedef enum timer_type_e
{
    E_PULSE,
} timer_type_t;

typedef enum timer_state_e
{
    E_TIMER_OFF = 0,
    E_TIMER_ON
} timer_state_t;

typedef uint32_t timer_handle_t;

typedef struct timer_s
{
    time_ms_t start;
    time_ms_t interval;
    // type: one-time, continous
    timer_type_t type;
    // state: on, off
    timer_state_t state;
    // signal handle
    handle_t signal; // todo: more than 1 handle to signal
    // todo: add routine
} timer_t;

// system APIs
void RunTimers(void);

// user APIs
timer_handle_t CreateTimer(time_ms_t interval, timer_type_t type, handle_t * handle);
void StartTimer(timer_handle_t handle);
void StopTimer(timer_handle_t handle);
void RemoveTimer(handle_t * handle);
