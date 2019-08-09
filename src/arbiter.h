#pragma once
#ifdef __cplusplus
extern "C" {
#endif
    
    #include "kernel_config.h"
    #include "kernel_types.h"

    struct arbiter_s;
    typedef volatile struct arbiter_s * arbiter_t;

    void Arbiter_Init(volatile arbiter_t * const arbiter);
    
    // return INVALID_HANDLE if not found
    task_handle_t Arbiter_GetHigestPrioTask(arbiter_t const arbiter);

    void Arbiter_AddTask(arbiter_t const arbiter, enum task_priority_e prio, task_handle_t h);
    void Arbiter_RemoveTask(arbiter_t const arbiter, enum task_priority_e prio, task_handle_t h);
    
    // return same handle as current if no switch is needed
    task_handle_t Arbiter_FindNext(arbiter_t const arbiter, enum task_priority_e prio);

#ifdef __cplusplus
} // closing brace for extern "C"

#endif
