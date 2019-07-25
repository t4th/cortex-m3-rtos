#include "event.h"

typedef struct
{
    uint32_t    data_pool_status[MAX_EVENTS];
    event_t     data_pool[MAX_EVENTS];
} even_module_t;

volatile even_module_t g_events;

// return non-zero if ok, 0 if fail
static int FindEmptyEvent(uint32_t * position)
{
    int pos;
    
    for (pos = 0; pos < MAX_EVENTS; pos ++) {
        if (0 == g_events.data_pool_status[pos])
        {
            *position = pos;
            break;
        }
    }
    
    if (MAX_EVENTS == pos)
        return 0;
    else
        return 1;
}

handle_t CreateEvent(void)
{
    handle_t ret;
    uint32_t free_pos;
    ret.type = E_EVENT;
    
    if (FindEmptyEvent(&free_pos))
    {
        g_events.data_pool_status[free_pos] = 1;
        // fill up free event structure
        g_events.data_pool[free_pos].signal = SIGNAL_RESET;
        
        ret.value = free_pos;
    }
    else
    {
        // max timers reached
        ret.value = INVALID_HANDLE;
    }
    
    return ret;
}

void SetEvent(handle_t * event)
{
    if (event->type == E_EVENT) {
        g_events.data_pool[event->value].signal = SIGNAL_SET;
    }
}

void ResetEvent(handle_t * event)
{
    if (event->type == E_EVENT) {
        g_events.data_pool[event->value].signal = SIGNAL_RESET;
    }
}
