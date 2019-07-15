// arbiter
#include "arbiter.h"
#include "assert.h"

void Arbiter_Init(arbiter_t * const arbiter)
{
    int prio, task;
    
    for (prio = 0; prio < MAX_PRIORITIES; prio++) {
        for (task = 0; task < MAX_USER_THREADS; task++) {
            arbiter->task_list[prio].list[task] = INVALID_HANDLE;
        }
     }
}

task_handle_t Arbiter_GetHigestPrioTask(arbiter_t * const arbiter)
{
    int prio, task;
    task_handle_t h = INVALID_HANDLE;
    
    for (prio = 0; prio < MAX_PRIORITIES; prio++) {
        for (task = 0; task < MAX_USER_THREADS; task++) {
            h = arbiter->task_list[prio].list[task];
            if (INVALID_HANDLE != h) {
                goto skip;
            }
            
        }
    }
    skip:
    return h;
}

void Arbiter_AddTask(arbiter_t * const arbiter, task_priority_t prio, task_handle_t h)
{
    const int count = arbiter->task_list[prio].count; // just for readabilty
    
    //assert(prio < MAX_PRIORITIES);
    //assert(h != INVALID_HANDLE);
    
    arbiter->task_list[prio].list[count] = h;
    arbiter->task_list[prio].count++;
}

void Arbiter_RemoveTask(arbiter_t * const arbiter, task_priority_t prio, task_handle_t h)
{
    // find task
    // remove
}

void Arbiter_Sort(arbiter_t * const arbiter)
{
}

