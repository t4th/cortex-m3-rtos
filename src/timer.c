#include "timer.h"

#include "kernel_config.h"
#include "kernel_api.h" // GetTime, WakeTask

typedef struct
{
    uint32_t    data_pool_status[MAX_TIMERS];
    timer_t     data_pool[MAX_TIMERS];
} timer_module_t;

volatile timer_module_t g_timer;

// return non-zero if ok, 0 if fail
static int FindEmptyTimer(uint32_t * position)
{
    int pos;
    
    for (pos = 0; pos < MAX_TIMERS; pos ++) {
        if (0 == g_timer.data_pool_status[pos])
        {
            *position = pos;
            break;
        }
    }
    
    if (MAX_TIMERS == pos)
        return 0;
    else
        return 1;
}

// iterate through all timers and signal what is to be signaled
void RunTimers(void)
{
    // todo: add 'count' variable to skip searching
    int current;
    
    for (current = 0; current < MAX_TIMERS; current++) { // force search through all
        if (g_timer.data_pool_status[current])
        {
            if (E_TIMER_ON == g_timer.data_pool[current].state)
            {
                time_ms_t current_time = GetTime();
                
                // calculate time interval
                if (current_time - g_timer.data_pool[current].start > g_timer.data_pool[current].interval)
                {
                    g_timer.data_pool[current].state = E_TIMER_OFF;
                    
                    switch(g_timer.data_pool[current].handle.type)
                    {
                        case E_TASK:
                            ResumeTask(g_timer.data_pool[current].handle.handle);
                            break;
                        default:
                            // do nothing
                            while(1); // not supported
                            break;
                    }
                    if (E_PULSE == g_timer.data_pool[current].type)
                        RemoveTimer(current);
                }
            }
        }
    }
}

// timer handle is timer position in data_pool
timer_handle_t CreateTimer(time_ms_t interval, timer_type_t type, handle_t * handle)
{
    uint32_t free_pos;
    
    if (FindEmptyTimer(&free_pos))
    {
        g_timer.data_pool_status[free_pos] = 1;
        // fill up free timer structure
        g_timer.data_pool[free_pos].start = GetTime();
        g_timer.data_pool[free_pos].interval = interval;
        g_timer.data_pool[free_pos].type = type;
        g_timer.data_pool[free_pos].state = E_TIMER_OFF;
        g_timer.data_pool[free_pos].handle = *handle;
        
        return free_pos;
    }
    else
    {
        return INVALID_HANDLE;
    }
}

void StartTimer(timer_handle_t handle)
{
    // todo: sanity
    g_timer.data_pool[handle].state = E_TIMER_ON;
}

void StopTimer(timer_handle_t handle)
{
    // todo: sanity
    g_timer.data_pool[handle].state = E_TIMER_OFF;
}

void RemoveTimer(timer_handle_t handle)
{
    // todo: sanity
    g_timer.data_pool_status[handle] = 0;
}
