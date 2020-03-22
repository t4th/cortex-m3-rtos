# Small cortex-m3 RTOS

This is experimental fun project, so no fancy build/tool sets are used.  
Simple Keil Uvision Lite project due to free simulator.

Check out prove of concept branches for technical stuff. Master will be empty until 1.0.  
Task tracking via github: https://github.com/users/t4th/projects/1

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

## Requirement
* windows
* keil Uvision 5 lite

## Build
Open project **keil\rtos.uvprojx** or call:  
**build.BAT** to build  
**build.BAT clean** or **build.Bat c** to clean  
**build.BAT re** to retranslate  
**build.BAT debug** or **build.BAT d** to start debuging  

In build.BAT file you must set Keil uvision location, ie. **set keil_dir=d:\Keil_v5\UV4**.  

## Edit
VS Code or Uvision.

## Experimental POC branches
Scheduler prove-of-concept
https://github.com/t4th/cortex-m3-rtos/tree/schedule_poc

kernel prove-of-concept
https://github.com/t4th/cortex-m3-rtos/tree/kernel_poc

## After POC branch decisions
* HANDLE system like in windows (generlized handle to system objects for easier user API) - better than raw pointers
* ~~API function will use win32 function names (since I like it)~~ - since c++ will be used this is no longer valid
* task queue circular linked list ~~or b-tree~~ - make it work, not CS degree viable

## ToDo never
- setup gcc project with cmake
