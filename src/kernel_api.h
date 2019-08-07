#pragma once

#include "kernel_types.h"

// user api

handle_t GetHandle(void);
time_ms_t GetTime(void);
void Sleep(time_ms_t);
int  CreateTask(task_routine_t _routine, task_priority_t _priority, handle_t * _handle, BOOL create_suspended);
void TerminateTask(handle_t * task);
void ResumeTask(task_handle_t task);
void printErrorMsg(const char * errMsg);

#if 0 // optional for now
int GetLastError(void); // each task has its own last error message

void SuspendThread(void);

BOOL WaitOnAddress(
    volatile uint32_t * Address,
    uint32_t CompareAddress,
    uint32_t dwMilliseconds
    );
#endif
