#pragma once

#include <scheduler.hpp>
#include <handle.hpp>
#include <timer.hpp>
#include <event.hpp>

namespace kernel::internal::system_timer
{
    // Round-robin context switch intervals in miliseconds.
    constexpr Time_ms CONTEXT_SWITCH_INTERVAL_MS = 10U;

    struct Context
    {
        Time_ms             m_oldTime = 0U;     // Used to calculate round-robin context
                                                // switch intervals.
        volatile Time_ms    m_currentTime = 0U; // Time in miliseconds elapsed since
                                                // kernel started.
    };

    inline Time_ms get( Context & a_context)
    {
        return a_context.m_currentTime;
    }

    inline void increment( Context & a_context)
    {
        ++a_context.m_currentTime;
    }

    inline bool isIntervalElapsed( Context & a_context)
    {
        if (a_context.m_currentTime - a_context.m_oldTime > CONTEXT_SWITCH_INTERVAL_MS)
        {
            a_context.m_oldTime = a_context.m_currentTime;
            return true;
        }
        return false;
    }
};

// System API used by kernel::hardware layer.
namespace kernel::internal
{
    // TODO: kernel context needs to be volatile since most kernel function are accessed from handler mode. 
    struct Context
    {
        bool started = false;   // Indicate if kernel is started. It is mostly used to detected
                                // if system object was created before or after kernel::start.

        // Lock used to stop kernel from round-robin context switches.
        // 0 - context switch unlocked; !0 - context switch locked
        volatile uint32_t schedule_lock = 0U;

        // Data
        internal::system_timer::Context m_systemTimer;
        internal::task::Context         m_tasks;
        internal::scheduler::Context    m_scheduler;
        internal::timer::Context        m_timers;
        internal::event::Context        m_events;
    };

    // todo: get rid of m_context
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
