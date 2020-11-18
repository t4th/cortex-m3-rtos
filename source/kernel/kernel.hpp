#pragma once

#include <cstdint>

// User API
namespace kernel
{
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
}

// User API for controling tasks.
namespace kernel::task
{
    typedef void(*Routine)(void * a_parameter);  // TODO: Add argument.

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
    void terminate(kernel::Handle a_id);
}

// System API used by kernel::hardware layer.
namespace kernel::internal
{
    void loadNextTask(); // __attribute__((always_inline));
    void switchContext(); // __attribute__((always_inline));
    bool tick();// __attribute__((always_inline));
}
