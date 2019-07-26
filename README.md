# Small cortex-m3 RTOS

This is experimental fun project, so no fancy build/tool sets are used.
Simple Keil Uvision Lite project (for now).

## Goals & assumptions

### Scheduler
* Fully pre-emptive priority based multitasking
* ~~statically defined task before starting OS~~ run-time thread creation/deletion
* highest priority task are to be served first
* tasks of the same priority should be running using Round Robin
* idle task as lowest priority or separated from user task?

### Features
* events, 2-state SET/RESET
* software timers
* interrupts can be mapped to events.
* critical section
* WaitForEvent function
* sleep function

### Memory
* compile time defined reserved thread and stack space
* no dynamic allocations

## After POC branch decisions
* HANDLE system like in windows (generlized handle to system objects for easier user API)
* API function will use win32 function names (since I like it)
* task queue - linked list or b-tree?
