#pragma once

#include <scheduler.hpp>
#include <handle.hpp>
#include <timer.hpp>
#include <event.hpp>

// task can wait for:
// - sleep - timer elapsed then resume
// - wait for object - object set, object abandon or timeout elapsed, then resume
//

// TODO: add wait queue to scheduler
namespace kernel::internal::wait_queue
{
    // TODO: calculate this from number of tasks and MAX_CONDITIONS.
    constexpr uint32_t MAX_NUMBER = 32;

    enum class Condition
    {
        Timer,
        Event,
        Task
    };

    struct WaitState
    {
        task::Id    m_sourceTask;
    };

    bool addTask(
        task::Id  a_sourceTask,
        task::WaitConditions::Type a_condition,
        Handle    a_sourceCondition,
        Time_ms   a_interval,
        bool      a_waitForver
    );

    // When task or source condition source is Destroyed
    // conditions should be too with Abandon code.
    void removeTask();
    
        // remove task from wait queue
    

    // must be run when:
    // - any system object used as condition cease to exsist
    // - every tick, before round-robin scheduling
    void checkTimeConditions();
}

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

        kernel::internal::task::Id m_current; // Indicate currently running task ID.
        kernel::internal::task::Id m_next;    // Indicate next task ID.

                                              // TODO: make status register
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
