#pragma once
#ifdef __cplusplus
extern "C" {
#endif
    
    #include "kernel_config.h"
    #include "kernel_types.h"

    // todo: make task list a ring-buffer queue
    // put finished task at the end of priority queue
    // todo: change arbiter name to scheduler

    typedef struct
    {
        int count;
        int current;
        task_handle_t list[MAX_USER_TASKS];
    } task_queue_t;

    typedef struct arbiter_s
    {
        task_queue_t task_list[MAX_PRIORITIES]; // todo: size is number of prorities
    } arbiter_t;

    // system APIs
    void Arbiter_Init(arbiter_t * const arbiter);
    // return IDLE_TASK_ID if not found
    task_handle_t Arbiter_GetHigestPrioTask(arbiter_t * const arbiter);

    void Arbiter_AddTask(arbiter_t * const arbiter, task_priority_t prio, task_handle_t h);
    void Arbiter_RemoveTask(arbiter_t * const arbiter, task_priority_t prio, task_handle_t h);
    // return same handle as current if no switch is needed
    task_handle_t Arbiter_FindNext(arbiter_t * const arbiter, task_priority_t prio);

#ifdef __cplusplus
} // closing brace for extern "C"

#endif
