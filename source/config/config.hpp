#pragma once

#include <kernel.hpp>

// This is kernel configuration file, where number of
// options can be changed depending of specific project
// goal.

namespace kernel::internal::hardware
{
    // Core clock frequency used to drive SysTick (kernel timer).
    constexpr uint32_t core_clock_freq_hz{ 72'000'000U};

    namespace task
    {
        // Define maximum stack size for each task.
        // Setting this too low or decreasing compiler optimization levels
        // can easly cause undefined behaviour due to stack over/under-flow.
        constexpr uint32_t stack_size{ 256U};
    }
}

namespace kernel::internal::task
{
    // Define maximum number of tasks.
    constexpr uint32_t max_number{ 10U};
}

namespace kernel::internal::system_timer
{
    // Round-robin context switch intervals in miliseconds.
    constexpr TimeMs context_switch_interval_ms{ 10U};
}

namespace kernel::internal::event
{
    // Define maximum number of events.
    // It must be noted that kernel itself can implicitly
    // create and delete events for internal use and setting
    // this value to 0 will brick some kernel functionality.
    constexpr uint32_t max_number{ 8U};
}

namespace kernel::internal::timer
{
    // Define maximum number of software timers.
    constexpr uint32_t max_number{ 8U};
}

namespace kernel::internal::queue
{
    // Define maximum number of static queues.
    constexpr size_t max_number{ 4U};
}

namespace kernel::internal::scheduler::wait
{
    // Define maximum waitable signals by task.
    // Main use: WaitForObject functions.
    constexpr uint32_t max_input_signals{ 8U};
}

namespace kernel::error
{
    // Enable kernel API error messages.
    constexpr bool debug_messages_enable{ true};
}
