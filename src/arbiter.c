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
    
    //todo: sanity check is done by the caller!
    //assert(prio < MAX_PRIORITIES);
    //assert(h != INVALID_HANDLE);
    
    arbiter->task_list[prio].list[count] = h;
    arbiter->task_list[prio].count++;
}

void Arbiter_RemoveTask(arbiter_t * const arbiter, task_priority_t prio, task_handle_t h)
{
    task_handle_t current = INVALID_HANDLE;
    task_handle_t next = INVALID_HANDLE;
    
    // find current
    {
        int i;
        for (i = 0; i < arbiter->task_list[prio].count; i ++) {
            if (arbiter->task_list[prio].list[i] == h) {
                current = i;
                break;
            }
        }
        next = current + 1;
        
        if (INVALID_HANDLE == current) {
            while(1); // unhandled expection
        }
    }
    
    arbiter->task_list[prio].count--;
    
    if (arbiter->task_list[prio].count > 0) {
        int i;
        // shift all to the left
        // todo: when removing from inside create hole
        //       move last element instead of iterating all.
        for (i = 0; i < arbiter->task_list[prio].count; i ++) {
            arbiter->task_list[prio].list[current] = arbiter->task_list[prio].list[next];
            arbiter->task_list[prio].list[next] = INVALID_HANDLE;
            current = next;
            next ++;
        }
    } else {
        arbiter->task_list[prio].list[0] = INVALID_HANDLE;
        arbiter->task_list[prio].current = 0;
    }
}

void Arbiter_Sort(arbiter_t * const arbiter)
{
}

task_handle_t Arbiter_FindNext(arbiter_t * const arbiter, task_priority_t prio)
{
    task_handle_t handle = INVALID_HANDLE;
    task_queue_t * queue = &arbiter->task_list[prio];
    
    if (queue->count > 1) {
        queue->next = queue->current + 1;
        
        if (queue->next >= queue->count) {
            queue->next = 0;
        }
        
        handle = queue->list[queue->next];
        queue->current = queue->next;
    }
    else {
        // return current task as next if no switch is needed
        handle = queue->list[queue->current];
    }
    
    return handle;
}
