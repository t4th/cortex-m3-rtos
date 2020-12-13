#pragma once

#include <cstdint>
#include <atomic>

// User API.
namespace kernel
{
    typedef uint32_t Time_ms;

    // Abstract handle to system object.
    // Should only be used with kernel API.
    // Direct modification is UB.
    typedef volatile void * Handle;

    // Initialize kernel.
    // This is PRE-CONDITION to all other kernel functions!
    // If not called first, kernel behaviour is UNDEFINED.
    void init();

    // Start kernel. If no user tasks were created, jumps to IDLE task.
    void start();

    // Get time elapsed since kernel started in miliseconds.
    Time_ms getTime();
}

// User API for controling tasks.
namespace kernel::task
{
    typedef void(*Routine)(void * a_parameter);

    // Task priority. Idle task MUST always be available and MUST
    // always be last in Priority enum. Otherwise UB.
    enum class Priority
    {
        High = 0U,
        Medium,
        Low,
        Idle
    };

    enum class State
    {
        Suspended,
        Waiting,
        Ready,
        Running
    };

    // Create new task. Can be create statically (before calling kernel::start()),
    // and in run-time, by other tasks.
    bool create(
        kernel::task::Routine   a_routine,
        kernel::task::Priority  a_priority = kernel::task::Priority::Low,
        kernel::Handle *        a_handle = nullptr,
        void *                  a_parameter = nullptr,
        bool                    a_create_suspended = false
    );

    // Get Handle to currently running task.
    kernel::Handle getCurrent();

    // Brute force terminate task. Can cause UB if task is system object owner, or
    // task was inside critical section.
    void terminate( kernel::Handle & a_handle);

    // Suspend task execution. Task can suspend itself.
    void suspend( kernel::Handle & a_handle);

    // Works only with Suspended tasks. Has no effect on Ready or Waiting tasks.
    void resume( kernel::Handle & a_handle);

    void sleep( Time_ms a_time);
}

// User API for controling software timers.
namespace kernel::timer
{
    bool create(
        kernel::Handle &    a_handle,
        Time_ms             a_interval
    );
    void destroy( kernel::Handle & a_handle);
    void start( kernel::Handle & a_handle);
    void stop( kernel::Handle & a_handle);
}

// User API for controling events.
namespace kernel::event
{
    // If a_manual_reset is set to false, event will be reset when waitForObject
    // function completes. In other cause, you have to manualy call reset.
    bool create( kernel::Handle & a_handle, bool a_manual_reset = false);
    void destroy( kernel::Handle & a_handle);
    void set( kernel::Handle & a_handle);
    void reset( kernel::Handle & a_handle);
}

// User API for controling critical section.
namespace kernel::critical_section
{
    // Modyfing this outsice critical_section API is UB.
    struct Context
    {
        volatile std::atomic<uint32_t> m_lockCount;
        volatile uint32_t m_spinLock;
        kernel::Handle m_event;
        kernel::Handle m_ownerTask; // debug information
    };

    // Spinlock argument define number of ticks used to check critical section
    // condition. Experiment with its value can improve performance, since it
    // is reducing number of context switches.
    bool init( Context & a_context, uint32_t a_spinLock = 100U);
    void deinit( Context & a_context);

    // Calling enter/leave without calling init first will cause errors.
    void enter( Context & a_context);
    void leave( Context & a_context);
}

namespace kernel::sync
{
    enum class WaitResult
    {
        ObjectSet,
        TimeoutOccurred,
        WaitFailed,         // Return if any internal error occurred.
        InvalidHandle
    };

    // Can wait for system objects of type: Event, Timer.
    // For event it is SET state, for Timer it is FINISHED state.
    // If used with invalid handle, error is returned.

    // The moment function is called, and prowivded signals are not set
    // task will stop execution and enter Waiting state until conditions
    // are not met.

    // If a_wait_forver is true, then a_timeout is not used.

    // Function return ObjectSet if signal condittion is met.

    // NOTE: Destroying system objects used by this function will result
    //       in undefined behaviour
    WaitResult waitForSingleObject(
        kernel::Handle &    a_handle,
        bool                a_wait_forver = true,
        Time_ms             a_timeout = 0U
    );

    // Wait for multiple system objects provided as an array.

    // If a_wait_for_all is set, task will wait until ALL signals are set.
    // If a_wait_for_all is not set, optional argument a_signaled_item_index,
    // will return first signaled handle index.
    WaitResult waitForMultipleObjects(
        kernel::Handle *    a_array_of_handles,
        uint32_t            a_number_of_elements,
        bool                a_wait_for_all = true,
        bool                a_wait_forver = true,
        Time_ms             a_timeout = 0U,
        uint32_t *          a_signaled_item_index = nullptr
    );
}
