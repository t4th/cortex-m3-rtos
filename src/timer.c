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
    handle_t current = {0,E_TIMER};
    
    
    // force search through all since timers are not sorted
    for (current.value = 0; current.value < MAX_TIMERS; current.value++) {
        if (g_timer.data_pool_status[current.value])
        {
            if (E_TIMER_ON == g_timer.data_pool[current.value].state)
            {
                time_ms_t current_time = GetTime();
                
                // calculate time interval
                if (current_time - g_timer.data_pool[current.value].start > g_timer.data_pool[current.value].interval)
                {
                    g_timer.data_pool[current.value].state = E_TIMER_OFF;
                    
                    // find what to signal
                    switch(g_timer.data_pool[current.value].signal.type)
                    {
                        case E_TASK:
                            ResumeTask(g_timer.data_pool[current.value].signal.value);
                            break;
                        default:
                            // do nothing
                            while(1); // not supported
                    }
                    
                    // if timer is ONE-TIME type remove it
                    if (E_PULSE == g_timer.data_pool[current.value].type) {
                        RemoveTimer(&current);
                    }
                }
            }
        }
    }
}

// timer handle is timer position in data_pool
// return timer handle if created ok.
timer_handle_t CreateTimer(time_ms_t interval, timer_type_t type, handle_t * handle)
{
    // todo: sanity of handle argument -> nothing to signal means timer is not needed
    uint32_t free_pos;
    
    if (FindEmptyTimer(&free_pos))
    {
        g_timer.data_pool_status[free_pos] = 1;
        // fill up free timer structure
        g_timer.data_pool[free_pos].start = GetTime();
        g_timer.data_pool[free_pos].interval = interval;
        g_timer.data_pool[free_pos].type = type;
        g_timer.data_pool[free_pos].state = E_TIMER_OFF;
        g_timer.data_pool[free_pos].signal = *handle;
        
        return free_pos;
    }
    else
    {
        // max timers reached
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

void RemoveTimer(handle_t * handle)
{
    //todo: sanity
    switch (handle->type) {
        case E_TASK:
            {
                timer_handle_t current;
                
                // find handle signal
                for (current = 0; current < MAX_TIMERS; current++) {
                    if (g_timer.data_pool[current].signal.value == handle->value) {
                        g_timer.data_pool_status[current] = 0;
                    }
                }
                // remove it
            }
            break;
        case E_TIMER:
            g_timer.data_pool_status[handle->value] = 0;
            break;
        default:
            break;
    }
    // todo: sanity
}
