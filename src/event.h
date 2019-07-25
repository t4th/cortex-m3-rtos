#pragma once

#include "kernel_config.h"
#include "kernel_types.h"

typedef enum
{
    SIGNAL_RESET = 0,
    SIGNAL_SET = 1
} event_signal_e;

typedef struct
{
    event_signal_e signal;
    
    #if 0 // optional for now
    // options
    uint32_t options;
    // name
    const char * name;
    #endif
} event_t;

handle_t CreateEvent(void); // todo: start state, autoreset options
void SetEvent(handle_t * event);
void ResetEvent(handle_t * event);

#if 0 // optional for now
void DestroyEvent(handle_t * event);
void OpenEvent(char * name);
#endif
