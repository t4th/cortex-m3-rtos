# ARM Cortex-m3 RTOS

This is hobby project to create small RTOS with just enough features to make it interesting.
Unlike other cortex-m RTOS projects (and there are many of them) my goal is not to create fastest/safest/hackest version,
but elegant and easy to navigate version for education purpose. That's why I used this oppurtinity to test some C++17 features and different
programming paradigms/architect decisons.

No fancy build system set - simple Keil Uvision Lite project due to free simulator. (it should be easy enough to setup this project with any other compiler/system)
90% of development time was spent using simulator. Examples are avilable in the repository.

Check out prove of concept branches for technical stuff (link below).

Main source of information is official ARMv7-M Architecture Reference Manual
https://developer.arm.com/documentation/ddi0403/latest/

Task tracking via github:
https://github.com/t4th/cortex-m3-rtos/projects/1

On target template project:
https://github.com/t4th/cortex-m3-rtos-project-template

On target example projects:
https://github.com/t4th/cortex-m3-rtos-blinky-example

## Notes
12/12/2020 - initial version of waitForMultipleObjects.

12/03/2020 - initial version of timers, events, sleep, critical section and waitForSingleObject are now pushed and working

11/13/2020 - after doing this project in spare time it finally reached version 0.1! It is possible to create and remove tasks statically and in runtime. Stability is good, but I am still experimenting with optimal stack size and until stack overflow detection is done one must be careful with number of function entries in task routine!

## Goals

### 1.0 Features
- [x] task: create, delete
- [x] software timers: create, delete
- [x] sleep function
- [x] events: create, delete
- [x] synchronization functions: WaitForSingleObject, WaitForMultipleObjects
- [x] software critical section: enter, leave
- [x] hardware critical section: enter, leave
- [x] queue
- [ ] examples

### 1.0+ Features
- [ ] static way to map HW interrupts to events (compile time named events?)
- [ ] stack over/underflow detection in Idle task
- [ ] implement privilege levels
- [ ] implement priority inheritance
- [ ] setup gcc project with cmake for linux
- [ ] adapt HW layer for Cortex-M4
- [ ] adapt HW layer for Cortex-M7
- [ ] core tests

### Other
* build and simulated via keil uvision
* use ARM Compiler 6 for C++17 compatibility
* runs on stm32f103ze (no cache, no fpu, simple pipeline)

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
Preview using UML. There are no objects, but it's okay to show code dependency.
![Alt arch](/doc/arch.png?raw=true)

## Tools
* windows
* keil Uvision 5 lite 529 (Monday, November 18, 2019) or up (needed for c++17)

## Build
Install keil Uvision 5 lite 529 and set up path to install dir in **build.BAT** file, ie. **set keil_dir=d:\Keil_v5\UV4**.  

Open project **keil\rtos.uvprojx** or call:  
**build.BAT** to build  
**build.BAT clean** or **build.Bat c** to clean  
**build.BAT re** to retranslate  
**build.BAT debug** or **build.BAT d** to start debuging  

...or just open project in Uvision and build/run using IDE.

If you want to build this project without Uvision just throw it to any ARM compiler and set:
* C++ to 17
* cpu=cortex-m3
* disable c++ exceptions with no-exceptions flag
* define preprocessor variable STM32F10X_HD for stm32 vendor headers
* add include paths: source; external/arm; external/st/STM32F10x
* and of course add kernel files to compiler: source/kernel.cpp and source/hardware/armv7m/hardware.cpp

## Edit
Visual Studio Community 2019 (set x86 in Configuration Manager), Uvision or just open in VSCode.

## Experimental POC branches
Scheduler prove-of-concept
https://github.com/t4th/cortex-m3-rtos/tree/schedule_poc

kernel prove-of-concept
https://github.com/t4th/cortex-m3-rtos/tree/kernel_poc

## After POC branch decisions
* HANDLE system similiar to windows win32 (generlized handle to system objects for easier user API) - better than raw pointers
* task scheduling made with circular linked list - simplest solution for 1.0 (normally its heap or b-tree)

## API usage examples
User API functions and parameters details are descripted in **kernel.hpp**.
All working examples are in examples directory.

### Initial setup
**kernel.hpp** is the only header needed to access all kernel features.

```c++
#include <kernel.hpp>

void example_task_routine( void * a_parameter)
{
    // Tasks can be created dynamically inside other task routines.
    kernel::task::create( [](void*){ for(;;);}, kernel::task::Priority::Low);
    for(;;);
}

int main()
{
    kernel::init();

    // Tasks can be created staticly between kernel::init and kernel::start.
    kernel::task::create( example_task_routine, kernel::task::Priority::Low);

    kernel::start();
}
```

### Waitable objects and task synchronization
Kernel objects are objects maintained by kernel and controlled by user API.
All kernel objects created by user can be tracked and controlled by unique **kernel::handle**.
This handle is an abstract construct which can point to ANY object type.
This solution allows simple API for common functionalities like **waitForSingleObject** and **waitForMultipleObjects** functions.

The objects that don't use handles are software and hardware critical sections which keep data in local scope.

```c++
#include <kernel.hpp>

void example_task_routine( void * a_parameter)
{
    kernel::handle all_objects[ 3];

    // Create timer.
    kernel::timer::create( all_objects[ 0], 1000U);

    // Create event.
    kernel::event::create( all_objects[ 1]));

    // Create static queue.
    kernel::static_queue::Buffer< char, 10> memory_buffer;
    kernel::queue( all_objects[ 2]), memory_buffer);

    while( true)
    {
        // Wait forever for at least one pointed object to be in Signaled State.
        // Signaled State state is condition unique for each underlying object:
        // for timer its FINISHED state; for event its SET state;
        // for queue it is when there is at least one element available to read.
        while ( ObjectSet == waitForMultipleObjects( all_objects, 3))
        {
            // Do something after waking up.
        }
    };
}

int main()
{
    kernel::init();

    kernel::task::create( example_task_routine, kernel::task::Priority::Low);

    kernel::start();
}
```

### Data synchronization
Data access can be maintained by kernel::critical_section and kernel::hardware::critical_section.

kernel::critical_section is software critical section and it will only work for data shared between tasks.

```c++
#include <kernel.hpp>
#include <cstring>

// You can also group this data in a struct and pass it as parameter to tasks - see example projects.
kernel::critical_section::context cs_context;
int shared_data[100];

void task1( void * a_parameter)
{
    while ( true)
    {
        kernel::critical_section::enter( cs_context);
        {
            std::memset( shared_data, 1, 100);
        }
        kernel::critical_section::leave( cs_context);
    };
}

void task2( void * a_parameter)
{
    while ( true)
    {
        kernel::critical_section::enter( cs_context);
        {
            std::memset( shared_data, 0, 100);
        }
        kernel::critical_section::leave( cs_context);
    };
}

int main()
{
    kernel::init();

    kernel::critical_section::init( cs_context);

    kernel::task::create( task1, kernel::task::Priority::Low);
    kernel::task::create( task2, kernel::task::Priority::Low);

    kernel::start();
}
```
See **examples/critical_section** for practical use.

kernel::hardware::critical_section is hardware level critical section which can protect data between task and hardware interrupt.
Unlike software version, context is initialized by **enter** function and it require Preemption priority as an argument.
Provided priority must be equal or higher than hardware interrupt to be effective.

```c++
kernel::hardware::critical_section::context cs_context;
int shared_data[100];

void IRQ_HANDLER()
{
    kernel::hardware::critical_section::enter( cs_context, interrupt::priority::Preemption::Critical);
    {
        std::memset( shared_data, 0, 100);
    }
    kernel::hardware::critical_section::leave( cs_context);
}

void task1( void * a_parameter)
{
    while ( true)
    {
        kernel::hardware::critical_section::enter( cs_context, interrupt::priority::Preemption::Critical);
        {
            std::memset( shared_data, 1, 100);
        }
        kernel::hardware::critical_section::leave( cs_context);
    };
}
```

System objects like **event** and **static_queue** are already protected by hardware critical section and are safe to be used in interrupt routine.

```c++
#include <kernel.hpp>

kernel::handle rx_queue;

void IRQ_HANDLER()
{
    // Send some data to the rx_queue.
    kernel::static_queue::send( usart_rx_queue, 0x1234);
}

void example_task_routine( void * a_parameter)
{
    // Enable interrupt with ID 30.
    // Note: remember to create queue before enabling interrupt!
    kernel::hardware::interrupt::enable( 30);

    while( true)
    {
        // Wait forever for queue not empty.
        while ( ObjectSet == waitForSingleObject( all_objects, 3))
        {
            int received_data{};

            // Get data from the queue.
            kernel::static_queue::receive( rx_queue, received_data);
        }
    };
}

int main()
{
    kernel::init();

    // Create static queue.
    kernel::static_queue::Buffer< int, 100> memory_buffer;
    kernel::queue( rx_queue), memory_buffer);

    kernel::task::create( example_task_routine, kernel::task::Priority::Low);

    kernel::start();
}
```

See **examples/serial_interrupt** for practical example with USART peripheral.

## Keil Uvision simulator preview
Overview of simulator view.
![Alt arch](/doc/sim.png?raw=true)
