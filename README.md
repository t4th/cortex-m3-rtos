# Small cortex-m3 RTOS

This is experimental fun project, so no fancy build/tool sets are used.
Simple Keil Uvision Lite project (for now).

## Goals

### 1.0 Features
* run-time thread creation/deletion ~~statically defined task before starting OS~~
* events, 2-state SET/RESET
* software timers: create, remove
* HW interrupts must be mapped to events
* critical section
* WaitForEvent function (or wait for object, ie. task, timer, event)
* sleep function

### Other
* C++ used, "c with namespaces" style though
* build and simulated via keil uvision
* runs on stm32f103ze

## Implementation decisions
* no automatic cleanup when task is terminated/closed
* tasks should not keep information about system objects created during it quanta time

### Scheduler
* fully pre-emptive priority based multitasking
* highest priority task are to be served first
* tasks of the same priority should be running using Round Robin
* idle task as lowest priority ~~or separated from user task?~~

### Memory
* compile time defined reserved thread and stack space
* no dynamic allocations

## Experimental POC branches
Scheduler prove-of-concept
https://github.com/t4th/cortex-m3-rtos/tree/schedule_poc

kernel prove-of-concept
https://github.com/t4th/cortex-m3-rtos/tree/kernel_poc

## After POC branch decisions
* HANDLE system like in windows (generlized handle to system objects for easier user API) - better than raw pointers
* ~~API function will use win32 function names (since I like it)~~ - since c++ will be used this is no longer valid
* task queue cirular linked list ~~or b-tree~~ - make it work, not CS degree viable
