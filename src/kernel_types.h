#pragma once

#include <stdint.h>

typedef unsigned char BOOL;
#define TRUE   (1u)
#define FALSE  (0u)

typedef enum task_priority_e
{
    T_HIGH = 0,
    T_MEDIUM,
    T_LOW
} task_priority_t;

typedef enum task_state_e
{
    T_TASK_SUSPENDED,
    T_TASK_WAITING,
    T_TASK_READY,
    T_TASK_RUNNING
} task_state_t;

typedef enum event_state_e
{
    T_RESET = 0,
    T_SET
} event_state_t;

// position of thread in thread_data_pool, so each is unique if it is never sorted
#define INVALID_HANDLE 0xffffffffu
#define IDLE_TASK_ID   0xfffffffeu

typedef uint32_t task_handle_t;


typedef enum handle_type_e
{
    E_INVALID = 0,
    E_TASK,
    E_TIMER,
    E_EVENT
} handle_type_t;

typedef struct // universal 'handle' type for easier API usage (like windows :D...)
{
    uint32_t value;
    handle_type_t type;
} handle_t; // todo: change to HANDLE? :)

typedef void(*task_routine_t)(void);

typedef uint32_t time_ms_t;
