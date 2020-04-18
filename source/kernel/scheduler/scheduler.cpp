#include <scheduler.hpp>

#include <circular_list.hpp>

#include <array>
#include <cstdint>

namespace
{

    
    struct
    {
        std::array
        <
            kernel::common::CircularList<uint32_t, kernel::task::max_task_number>,
            kernel::task::priorities_count
        >
        m_task_list;
        
    } m_context;
}

namespace kernel::scheduler
{
    void addTask(kernel::task::Priority a_priority, kernel::task::id a_id)
    {
        uint32_t i = static_cast<uint32_t>(a_priority);
        
        m_context.m_task_list[i].add(a_id);
    }
    
    void removeTask(kernel::task::Priority a_priority, kernel::task::id a_id)
    {
        uint32_t i = static_cast<uint32_t>(a_priority);
        
        m_context.m_task_list[i].remove(a_id);
    }
    
    bool findNextTask(kernel::task::Priority a_priority, kernel::task::id & a_id)
    {
        return false;
    }
}
