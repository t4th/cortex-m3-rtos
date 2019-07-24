#include "event.h"

typedef struct
{
    uint32_t    data_pool_status[MAX_EVENTS];
    event_t     data_pool[MAX_EVENTS];
} even_module_t;

volatile even_module_t g_events;
