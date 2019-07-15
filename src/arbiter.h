#pragma once

#include "kernel_config.h"
#include "kernel_types.h"

typedef struct
{
    int count;
    int current;
    int next;
    task_handle_t list[MAX_USER_THREADS];
} task_queue_t;

typedef struct arbiter_s
{
    task_queue_t task_list[MAX_PRIORITIES]; // todo: size is number of prorities
} arbiter_t;

void Arbiter_Init(arbiter_t * const arbiter);
// return INVALID_HANDLE if not found
task_handle_t Arbiter_GetHigestPrioTask(arbiter_t * const arbiter);

void Arbiter_AddTask(arbiter_t * const arbiter, task_priority_t prio, task_handle_t h);
void Arbiter_RemoveTask(arbiter_t * const arbiter, task_priority_t prio, task_handle_t h);
void Arbiter_Sort(arbiter_t * const arbiter);
