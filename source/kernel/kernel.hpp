#pragma once

#include <cstdint>

// User API
namespace kernel
{    
    void init();
    void start();
}

namespace kernel::task
{
    typedef struct // This is struct for typesafety
    {
        uint32_t m_id;
    } Id;

    typedef void(*Routine)(void);  // TODO: Add argument.

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

    bool create(
        kernel::task::Routine   a_routine,
        kernel::task::Priority  a_priority = kernel::task::Priority::Low,
        kernel::task::Id *      a_handle = nullptr,
        bool                    a_create_suspended = false
    );

    void terminate(kernel::task::Id a_id);
}

namespace kernel::internal
{
    void loadNextTask(); // __attribute__((always_inline));
    void switchContext(); // __attribute__((always_inline));
    bool tick();// __attribute__((always_inline));
}
