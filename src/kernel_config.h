#pragma once

#define MAX_USER_TASKS               8u
#define MAX_USER_TIMERS_PER_TASK     8u
// each task has it own timer for sleep
// todo: make something smart here like total amount of software timers available
#define MAX_TIMERS               (MAX_USER_TASKS + MAX_USER_TIMERS_PER_TASK)
#define MAX_PRIORITIES           3u     // max number of priorities
#define TASK_STACK_SIZE          32u    // should be aligned to 4 bytes boundary
#define MAX_EVENTS               8u

#if TASK_STACK_SIZE < 8
    #error "minimum stack size is 8!"
#endif
