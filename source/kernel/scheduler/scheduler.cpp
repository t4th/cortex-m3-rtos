#include <scheduler.hpp>

#include <task.hpp>
#include <memory_buffer.hpp>

#include <array>
#include <cstdint>

namespace
{
    struct Node
    {
        uint32_t m_prev;
        uint32_t m_next;
        uint32_t m_id;
    };
    
    // Each task priority group has individual circular linked list with static data buffer.
    struct List
    {
        uint32_t m_first;
        uint32_t m_last;

        uint32_t m_count;
        kernel::common::MemoryBuffer<Node, kernel::task::max_task_number> m_buffer;
    };
    
    struct
    {
        std::array<List, kernel::task::priorities_count> m_list;
    } m_context;
}

namespace kernel::scheduler
{
    
}
