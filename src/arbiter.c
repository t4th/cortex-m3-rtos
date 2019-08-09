// arbiter
#include "arbiter.h"
#include "assert.h"

struct task_queue_s
{
    int count;
    int current;
    task_handle_t list[MAX_USER_TASKS];
};

struct arbiter_s
{
    struct task_queue_s task_list[MAX_PRIORITIES]; // todo: size is number of prorities
};

volatile struct arbiter_s g_arbiter;

void Arbiter_Init(volatile arbiter_t * const arbiter)
{
    int prio, task;
    
    *arbiter = &g_arbiter;
    
    for (prio = 0; prio < MAX_PRIORITIES; prio++) {
        for (task = 0; task < MAX_USER_TASKS; task++) {
            (*arbiter)->task_list[prio].list[task] = INVALID_HANDLE;
        }
     }
}

task_handle_t Arbiter_GetHigestPrioTask(arbiter_t const arbiter)
{
    enum task_priority_e prio;
    task_handle_t h = INVALID_HANDLE;
    
    for (prio = T_HIGH; prio < MAX_PRIORITIES; prio++) {
        if (arbiter->task_list[prio].count > 0) {
            h = Arbiter_FindNext(arbiter, prio);
            goto skip;
        }
    }
    skip:
    return h;
}

void Arbiter_AddTask(arbiter_t const arbiter, enum task_priority_e prio, task_handle_t h)
{
    const int count = arbiter->task_list[prio].count; // just for readabilty
    
    //todo: sanity check is done by the caller!
    //assert(prio < MAX_PRIORITIES);
    //assert(h != INVALID_HANDLE);
    
    arbiter->task_list[prio].list[count] = h;
    arbiter->task_list[prio].count++;
}

void Arbiter_RemoveTask(arbiter_t const arbiter, enum task_priority_e prio, task_handle_t h)
{
    int current = INVALID_HANDLE;
    int next = INVALID_HANDLE;
    
    // find current
    {
        int i;
        for (i = 0; i < arbiter->task_list[prio].count; i ++) {
            if (arbiter->task_list[prio].list[i] == h) {
                current = i;
                next = current + 1;
                break;
            }
        }
        
        if (INVALID_HANDLE == current) {
            return;
            #if 0
            while(1); // unhandled expection
            #endif
        }
    }
    
    arbiter->task_list[prio].count--;
    
    // if current task is last item in queue
    if (arbiter->task_list[prio].list[next] == INVALID_HANDLE) {
        arbiter->task_list[prio].current = 0;
        arbiter->task_list[prio].list[current] = INVALID_HANDLE;
    }
    else {
        do
        {
            arbiter->task_list[prio].list[current] = arbiter->task_list[prio].list[next];
            arbiter->task_list[prio].list[next] = INVALID_HANDLE;
            current = next;
            next ++;
        } while (arbiter->task_list[prio].list[next] != INVALID_HANDLE);
    }
}

task_handle_t Arbiter_FindNext(arbiter_t const arbiter, enum task_priority_e prio)
{
    task_handle_t handle = INVALID_HANDLE;
    volatile struct task_queue_s * queue = &arbiter->task_list[prio];
    
    if (queue->count > 1) {
        // check if next item is at the end of queue
        int next = queue->current + 1;

        if (next < queue->count) {
        }
        else {
            next = 0; // go back to first item in queue
        }

        handle = queue->list[next];
        queue->current = next;
    } else {
        // return current task as next if no switch is needed
        queue->current = 0;
        handle = queue->list[queue->current];
    }
    
    return handle;
}
