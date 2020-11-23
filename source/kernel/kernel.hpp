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
    void terminate(kernel::Handle & a_id);

    void suspend(kernel::Handle & a_id);

    void resume(kernel::Handle & a_id);
}

namespace kernel::timer
{
    // a_signal can point to task which will be woken up or event that will be set.
    bool create( kernel::Handle & a_id, Time_ms a_interval, kernel::Handle * a_signal = nullptr);
    void destroy( kernel::Handle & a_id);
    void start( kernel::Handle & a_id);
    void stop( kernel::Handle & a_id);
}

namespace kernel::sync
{
    void waitForSingleObject(kernel::Handle & a_id);
}

// System API used by kernel::hardware layer.
namespace kernel::internal
{
    void loadNextTask(); // __attribute__((always_inline));
    void switchContext(); // __attribute__((always_inline));
    bool tick();// __attribute__((always_inline));
}
