#pragma once

#include <cstdint>

// User API
namespace kernel
{
    // Initialize kernel.
    void init();

    // Start kernel.
    // Pre-condition: run kernel::init() before.
    void start();
}

// User API for controling tasks.
namespace kernel::task
{
    typedef struct // This is struct for typesafety
    {
        uint32_t m_id;
    } Id;

    typedef void(*Routine)(void * a_parameter);  // TODO: Add argument.

    enum Priority
    {
        High = 0U,
        Medium,
        Low,
        Idle
    };

    enum State
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
        kernel::task::Id *      a_handle = nullptr,
        void *                  a_parameter = nullptr,
        bool                    a_create_suspended = false
    );

    // Brute force terminate task.
    void terminate(kernel::task::Id a_id);
}

// System API used by kernel::hardware layer.
namespace kernel::internal
{
    void loadNextTask(); // __attribute__((always_inline));
    void switchContext(); // __attribute__((always_inline));
    bool tick();// __attribute__((always_inline));
}
