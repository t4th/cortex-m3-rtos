# ARM Cortex-m3 RTOS

This is hobby project to create small RTOS with just enough features to make it interesting.
Unlike other cortex-m RTOS projects (and there are many of them) my goal is not to create fastest/safest/hackest version,
but elegant and easy to navigate version for education purpose. That's why I used this oppurtinity to test some C++17 features and different
programming paradigms/architect decisons.

No fancy build system set - simple Keil Uvision Lite project due to free simulator. (it should be easy enough to setup this project with any other compiler/system)

Check out prove of concept branches for technical stuff.  
Task tracking via github: https://github.com/t4th/cortex-m3-rtos/projects/1

## Notes
11/13/2020 - after doing this project in spare time it finally reached version 0.1! It is possible to create and remove tasks statically and in runtime. Stability is good, but I am still experimenting with optimal stack size and until stack overflow detection is done one must be careful with number of function entries in task routine!

## Goals

### 1.0 Features
- [x] task: create, delete
- [ ] software timers: create, remove
- [ ] sleep function
- [ ] events: create, remove
- [ ] function WaitForEvent or WaitForObject
- [ ] Critical section: enter, leave
- [ ] way to map HW interrupts to events

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

## Architecture
Initial preview using UML
![Alt arch](/doc/arch.png?raw=true)

## Tools
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
Visual Studio Community 2019 (set x86 in Configuration Manager) or Uvision (VS Code should do fine too).

## Experimental POC branches
Scheduler prove-of-concept
https://github.com/t4th/cortex-m3-rtos/tree/schedule_poc

kernel prove-of-concept
https://github.com/t4th/cortex-m3-rtos/tree/kernel_poc

## After POC branch decisions
* HANDLE system like in windows (generlized handle to system objects for easier user API) - better than raw pointers
* ~~API function will use win32 function names~~ - since c++ will be used this is no longer valid
* task queue circular linked list ~~or b-tree~~ - make it work, not CS degree viable

## ToDo never
- setup gcc project with cmake
- adapt HW layer to Cortex-M4

## Keil Uvision simulator preview
![Alt arch](/doc/sim.png?raw=true)