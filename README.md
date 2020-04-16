# Small cortex-m3 RTOS

This is experimental fun project, so no fancy build/tool sets are used.  
Simple Keil Uvision Lite project due to free simulator.

Check out prove of concept branches for technical stuff.  
Task tracking via github: https://github.com/t4th/cortex-m3-rtos/projects/1

## Goals

### 1.0 Features
* task: create, delete
* software timers: create, remove
* sleep function
* events: create, remove
* function WaitForEvent or WaitForObject
* Critical section: enter, leave
* HW interrupts must be mapped to events

### Other
* build and simulated via keil uvision
* use ARM Compiler 6 for C++17 compatibility
* runs on stm32f103ze

## Implementation decisions
* no automatic cleanup when task is terminated/closed
* tasks should not keep information about system objects created during its quanta time

### Scheduler
* fully pre-emptive priority based multitasking
* highest priority task are to be served first
* tasks of the same priority should be running using Round Robin
* idle task as lowest priority

### Memory
* static buffers for all kernel components
* no dynamic allocations

## Requirement
* windows
* keil Uvision 5 lite 529 (Monday, November 18, 2019) or up (needed for c++17)

## Build
Install keil Uvision 5 lite 529 and set up path to install dir in **build.BAT** file,
 ie. **set keil_dir=d:\Keil_v5\UV4**.  

Open project **keil\rtos.uvprojx** or call:  
**build.BAT** to build  
**build.BAT clean** or **build.Bat c** to clean  
**build.BAT re** to retranslate  
**build.BAT debug** or **build.BAT d** to start debuging  

...or just open project in Uvision and build/run using IDE.

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
