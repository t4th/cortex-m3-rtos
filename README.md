# ARM Cortex-m3 RTOS

This is a hobby project to create small RTOS with just enough features to make it interesting.

It is also a great opportunity to test some new C++17 features and different architectural design decisions for an embedded application.

Typical big RTOS projects tend to grow exponentially with increasing number of new features, board/compiler supports and code optimizations.
All these practices are effectively hiding the simple underlying principles of how RTOS is actually implemented.

My goal is to create elegant and easy to navigate project for educational purpose.

Main source of information is official ARMv7-M Architecture Reference Manual
https://developer.arm.com/documentation/ddi0403/latest/

Task tracking via github: https://github.com/t4th/cortex-m3-rtos/projects/1

On target template project: https://github.com/t4th/cortex-m3-rtos-project-template

On target example projects: https://github.com/t4th/cortex-m3-rtos-blinky-example

## Table of content
- [Goals](#goals)
- [Architecture](#architecture)
- [Build](#build)
- [API usage examples](#api-usage)
- [API software examples](#api-software-usage)

## Goals <a name="goals"/>

### 1.0 Features
- [x] task: create, delete
- [x] software timers: create, delete
- [x] sleep function
- [x] events: create, delete
- [x] synchronization functions: WaitForSingleObject, WaitForMultipleObjects
- [x] software critical section: enter, leave
- [x] hardware critical section: enter, leave
- [x] queue
- [x] examples
- [ ] design documents in readme
- [ ] write 1.0 summary (bad/good)

### 1.0+ Features
- [ ] port examples to popular boards (discovery, nucleo)
- [ ] static way to map HW interrupts to events
- [ ] stack over/underflow detection
- [ ] implement privilege levels
- [ ] implement priority inheritance
- [ ] PC application tracker
- [ ] setup gcc project with cmake for linux
- [ ] adapt HW layer for Cortex-M4
- [ ] adapt HW layer for Cortex-M7
- [ ] kernel API tests

### Other
* simple build system with core simulator - Keil uVision for Windows
* use ARM Compiler 6 for C++17 compatibility
* runs on stm32f103ze (no cache, no fpu, simple pipeline)

## Implementation decisions
* tasks should not keep information about life time of system objects created during its quanta time
* KISS principle, due to educational purpose of the project. Simple data structures, linear searches, no bit hacking, no implicit code tricks.
* HANDLE system similiar to windows win32 (generalized handle to system objects for easier user API)
* task scheduling made with circular linked list

### Scheduler
* fully pre-emptive priority based multitasking
* highest priority tasks are to be served first
* tasks of the same priority should be running using Round Robin
* idle task is always available at lowest priority

### Memory
* simple memory model; no dynamic allocations, ie. no classic heap
* fixed size static buffers for all kernel components creating during runtime

### Experimental POC branches
Scheduler in C: https://github.com/t4th/cortex-m3-rtos/tree/schedule_poc
Basic kernel in C: https://github.com/t4th/cortex-m3-rtos/tree/kernel_poc

## Architecture <a name="architecture"/>
Dependency preview using UML.
![Alt arch](/doc/arch.png?raw=true)

## Build <a name="build"/>
Install keil Uvision 5 lite 529 (or up) and set up path to install dir in **build.BAT** file, ie. **set keil_dir=d:\Keil_v5\UV4**.  

Open project **keil\rtos.uvprojx** or call:  
**build.BAT** to build  
**build.BAT clean** or **build.Bat c** to clean  
**build.BAT re** to retranslate  
**build.BAT debug** or **build.BAT d** to start debuging  

...or just open project in Uvision and build/run using IDE.

If you want to build this project without Uvision just use any gcc ARM compiler and set:
* C++ to 17
* cpu=cortex-m3
* disable c++ exceptions with no-exceptions flag
* define preprocessor variable STM32F10X_HD for stm32 vendor headers
* add include paths: source; external/arm; external/st/STM32F10x
* and of course add kernel files to compiler: source/kernel.cpp and source/hardware/armv7m/hardware.cpp

## Edit
I am using Visual Studio Community 2019 (set x86 in Configuration Manager) as editor and Uvision as debugger.

## API usage examples <a name="api-usage"/>
User API functions and parameters details are descripted in **kernel.hpp**.
All working examples are in **examples** directory.

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
    kernel::Handle all_objects[ 3];

    // Create timer.
    kernel::timer::create( all_objects[ 0], 1000U);

    // Create event.
    kernel::event::create( all_objects[ 1]);

    // Create static queue.
    kernel::static_queue::Buffer< char, 10> memory_buffer;
    kernel::static_queue::create( all_objects[ 2], memory_buffer);

    while( true)
    {
        using namespace kernel::sync;

        // Wait forever for at least one pointed object to be in Signaled State.
        // Signaled State state is condition unique for each underlying object:
        // for timer its FINISHED state; for event its SET state;
        // for queue it is when there is at least one element available to read.
        while ( WaitResult::ObjectSet == waitForMultipleObjects( all_objects, 3))
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
Data access can be maintained by **kernel::critical_section** and **kernel::hardware::critical_section**.

**kernel::critical_section** is software critical section and it will only work for data shared between tasks.

```c++
#include <kernel.hpp>
#include <cstring>

// You can also group this data in a struct and pass it as parameter to tasks - see example projects.
kernel::critical_section::Context cs_context;
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

**kernel::hardware::critical_section** is hardware level critical section which can protect data between task and hardware interrupt.
Unlike software version, context is initialized by **enter** function and it require **Preemption priority** as an argument.
Provided priority must be equal or higher than hardware interrupt to be effective.

Also hardware critical section context doesn't has to be shared between elements - it is only required in local scope.

```c++
#include <kernel.hpp>

int shared_data[ 100];

void IRQ_HANDLER()
{
    using namespace kernel::hardware::interrupt;

    kernel::hardware::critical_section::Context cs_context;
    
    kernel::hardware::critical_section::enter( cs_context, priority::Preemption::Critical);
    {
        std::memset( shared_data, 0, 100);
    }
    kernel::hardware::critical_section::leave( cs_context);
}

void task1( void * a_parameter)
{
    kernel::hardware::critical_section::Context cs_context;
    
    while ( true)
    {
        using namespace kernel::hardware::interrupt;

        kernel::hardware::critical_section::enter( cs_context, priority::Preemption::Critical);
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

kernel::Handle rx_queue;

void IRQ_HANDLER()
{
    // Send some data to the rx_queue.
    int data_to_be_sent = 0x1234;

    kernel::static_queue::send( rx_queue, data_to_be_sent);
}

void example_task_routine( void * a_parameter)
{
    // Enable interrupt with ID 30.
    // Note: remember to create queue before enabling interrupt!
    kernel::hardware::interrupt::enable( 30);

    while( true)
    {
        using namespace kernel::sync;
        
        // Wait forever for queue not empty.
        while ( WaitResult::ObjectSet == waitForSingleObject( rx_queue, 3))
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
    kernel::static_queue::create( rx_queue, memory_buffer);

    kernel::task::create( example_task_routine, kernel::task::Priority::Low);

    kernel::start();
}
```

See **examples/serial_interrupt** for practical example with USART peripheral.

## API software examples <a name="api-software-usage"/>
Kernel is printing log message through **kernel::hardware::debug** (ITM) which can be received and read by View->Serial windows->Debug (printf) Viewer both in simulator and on target examples in Keil Uvision.

| Directory name | Description | Kernel API used |
| --- | --- | --- |
| create_task | Create tasks statically and dynamically with different priorities and blocking delay to illustrate scheduling. | kernel, kernel::task | 
| critical_section | Illustrate how to use software critical section. Enable or disable **use_critical_section** variable to see the difference in access of shared data via the program output. | kernel, kernel::task, kernel::critical_section |
| serial_interrupt | This is on-target example using **kernel::static_queue** to receive and transmit data over USART peripheral. | kernel, kernel::task, kernel::static_queue, kernel::sync, kernel::hardware::debug, kernel::hardware::interrupt::priority, kernel::hardware::interrupt |
| software_timers | Use software timers to wake-up tasks in selected time intervals. | kernel, kernel::task, kernel::timer, kernel::sync |
| task_sleep | Use **task::sleep** to wake-up tasks in selected time intervals. | kernel, kernel::task |
| using_interrupt | This is on-target example using hardware interrupt to wake-up a sleeping task. It can also run on simulator and selected interrupt can be set to Pending via NVIC peripheral.  | kernel, kernel::task, kernel::event, kernel::sync |
| waitForMultipleObjects | Worker task is waiting for five automatic-reset events and one manual-reset event. Order tasks is setting those events step-by-step to wake-up worker tasks when all events are in SET state. | kernel, kernel::task, kernel::sync |
| waitForSingleObject | Task is being toggled by other task using single event synchronization object. | kernel, kernel::task, kernel::sync |

## Keil Uvision simulator preview
Overview of simulator view used for development.
![Alt arch](/doc/sim.png?raw=true)
