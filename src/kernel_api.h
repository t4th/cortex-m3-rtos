#pragma once

#include "kernel_types.h"

// user api

task_handle_t GetHandle(void);
time_ms_t GetTime(void);
void Sleep(time_ms_t);
int  CreateTask(task_routine_t _routine, task_priority_t _priority, handle_t * _handle, BOOL create_suspended);
void TerminateTask(void);
void ResumeTask(task_handle_t task);
//void TerminateThread(task_handle_t _handle);


void printErrorMsg(const char * errMsg);
