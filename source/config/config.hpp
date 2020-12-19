#pragma once

#include <kernel.hpp>

// This is kernel configuration file, where number of
// options can be changed depending of specific project
// goal.

namespace kernel::hardware
{
    // Core clock frequency used to drive SysTick (kernel timer).
    constexpr uint32_t CORE_CLOCK_FREQ_HZ{ 72'000'000U};

    // Define maximum stack size for each task.
    // Setting this too low or decreasing compiler optimization levels
    // can easly cause undefined behaviour due to stack over/under-flow.
    constexpr uint32_t TASK_STACK_SIZE = 192U;
}

namespace kernel::internal::task
{
    // Define maximum number of tasks.
    constexpr uint32_t MAX_NUMBER = 16U;
}

namespace kernel::internal::system_timer
{
    // Round-robin context switch intervals in miliseconds.
    constexpr Time_ms CONTEXT_SWITCH_INTERVAL_MS = 10U;
}

namespace kernel::internal::event
{
    // Define maximum number of events.
    // It must be noted that kernel itself can implicitly
    // create and delete events for internal use and setting
    // this value to 0 will brick some kernel functionality.
    constexpr uint32_t MAX_NUMBER = 32U;
}

namespace kernel::internal::timer
{
    // Define maximum number of software timers.
    constexpr uint32_t MAX_NUMBER = 16U;
}

namespace kernel::internal::scheduler::wait
{
    // Define maximum waitable signals by task.
    // Main use: WaitForObject functions.
    constexpr uint32_t MAX_INPUT_SIGNALS = 8U;
}
