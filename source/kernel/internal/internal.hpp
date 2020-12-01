#pragma once

#include <scheduler.hpp>
#include <handle.hpp>
#include <timer.hpp>
#include <event.hpp>

// System API used by kernel::hardware layer.
namespace kernel::internal
{
    constexpr Time_ms CONTEXT_SWITCH_INTERVAL_MS = 10U;

    // TODO: kernel context needs to be volatile since most kernel function are accessed from handler mode.
    struct Context
    {
        Time_ms old_time = 0U;      // Used to calculate round-robin context switch intervals.
        volatile Time_ms time = 0U; // Time in miliseconds elapsed since kernel started.
        Time_ms interval = CONTEXT_SWITCH_INTERVAL_MS; // Round-robin context switch intervals in miliseconds.

        bool started = false;   // Indicate if kernel is started. It is mostly used to detected
                                // if system object was created before or after kernel::start.

        // Lock used to stop kernel from round-robin context switches.
        // 0 - context switch unlocked; !0 - context switch locked
        volatile uint32_t schedule_lock = 0U;

        // Data
        internal::task::Context         m_tasks;
        internal::scheduler::Context    m_scheduler;
        internal::timer::Context        m_timers;
        internal::event::Context        m_events;
    };

    extern Context m_context;

    // Kernel Mode interfaces
    void init();
    void lockScheduler();
    void unlockScheduler();
    void terminateTask(kernel::internal::task::Id a_id);
    void task_routine();
    void idle_routine(void * a_parameter);

    // Handler Mode interfaces
    void loadNextTask(); // __attribute__((always_inline));
    void switchContext(); // __attribute__((always_inline));
    bool tick();// __attribute__((always_inline));
}
