#pragma once

#include <cstdint>

// User API
namespace kernel
{
    typedef uint32_t Time_ms;

    // Abstract pointing to system object. Should only be used with kernel API.
    typedef struct
    {
        uint32_t m_data;
    } Handle;

    // Initialize kernel.
    void init();

    // Start kernel.
    // Pre-condition: run kernel::init() before.
    void start();

    // Get time elapsed since kernel started in miliseconds.
    Time_ms getTime();
}

// User API for controling tasks.
namespace kernel::task
{
    typedef void(*Routine)(void * a_parameter);

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

    // Create new task.
    bool create(
        kernel::task::Routine   a_routine,
        kernel::task::Priority  a_priority = kernel::task::Priority::Low,
        kernel::Handle *        a_handle = nullptr,
        void *                  a_parameter = nullptr,
        bool                    a_create_suspended = false
    );

    // Get Handle to currently running task.
    kernel::Handle getCurrent();

    // Brute force terminate task.
    void terminate(kernel::Handle & a_handle);

    void suspend(kernel::Handle & a_handle);

    void resume(kernel::Handle & a_handle);

    void Sleep(Time_ms a_time);
}

namespace kernel::timer
{
    bool create(
        kernel::Handle &    a_handle,
        Time_ms             a_interval,
        kernel::Handle *    a_signal = nullptr // Can point to Event object.
    );
    void destroy( kernel::Handle & a_handle);
    void start( kernel::Handle & a_handle);
    void stop( kernel::Handle & a_handle);
}

namespace kernel::event
{
    bool create( kernel::Handle & a_handle, bool a_manual_reset = false);
    void destroy( kernel::Handle & a_handle);
    void set( kernel::Handle & a_handle);
    void reset( kernel::Handle & a_handle);
}

namespace kernel::critical_section
{
    struct Context
    {
        volatile uint32_t m_lockCount; // todo: interlocked
        volatile uint32_t m_spinLock;
        kernel::Handle m_event;
        kernel::Handle m_ownerTask; // debug information
    };

    bool init(Context & a_context, uint32_t a_spinLock = 100U);
    void deinit(Context & a_context);

    void enter(Context & a_context);
    void leave(Context & a_context);
}

namespace kernel::sync
{
    enum class WaitResult
    {
        ObjectSet,
        TimeoutOccurred,
        Abandon,
    };

    // Can wait for:
    // Event, timer, task
    // NOTE: destorying items used by this function will result in undefined behaviour
    WaitResult waitForSingleObject(
        kernel::Handle &    a_handle,
        bool                a_wait_forver = true,
        Time_ms             a_timeout = 0U
    );
}
