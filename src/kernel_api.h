#pragma once

#include "kernel_types.h"

// user api

task_handle_t GetHandle(void);
time_t GetTime(void);
void Sleep_ms(time_t);
int  CreateTask(task_routine_t _routine, task_priority_t _priority, task_handle_t * _handle);
void TerminateThread(void);
void TerminateThread(task_handle_t _handle);
