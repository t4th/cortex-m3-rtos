#pragma once

#include "kernel_config.h"
#include "kernel_types.h"

typedef struct
{
    handle_t id;
    
    #if 0 // optional for now
    // options
    uint32_t options;
    // name
    const char * name;
    #endif
} event_t;

handle_t CreateEvent(void); // todo: start state, autoreset options
void SetEvent(handle_t * event);

#if 0 // optional for now
void DestroyEvent(handle_t * event);
void OpenEvent(char * name);
#endif
