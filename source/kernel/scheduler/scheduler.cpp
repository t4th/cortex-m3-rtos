#include <scheduler.hpp>

#include <circular_list.hpp>

#include <array>
#include <cstdint>

namespace
{
    // Track current task for given priority.
    struct TaskList
    {
        kernel::common::CircularList<uint32_t, kernel::task::max_task_number>::Context m_context;
        kernel::common::CircularList<uint32_t, kernel::task::max_task_number> m_buffer;
        
        uint32_t m_current;
        
        TaskList() : m_buffer(m_context) {}
    };
    
    struct
    {
        std::array <TaskList, kernel::task::priorities_count> m_task_list;
        
    } m_context;
}

namespace kernel::scheduler
{
    void addTask(kernel::task::Priority a_priority, kernel::task::id a_id)
    {
        const uint32_t prio = static_cast<uint32_t>(a_priority);
        const uint32_t count = m_context.m_task_list[prio].m_buffer.count();
        
        uint32_t new_node_idx;
        
        m_context.m_task_list[prio].m_buffer.add(a_id, new_node_idx);
        
        if (0 == count)
        {
            m_context.m_task_list[prio].m_current = new_node_idx;
        }
    }
    
    void removeTask(kernel::task::Priority a_priority, kernel::task::id a_id)
    {
        const uint32_t prio = static_cast<uint32_t>(a_priority);
        const uint32_t count = m_context.m_task_list[prio].m_buffer.count();
        
        uint32_t node_index = m_context.m_task_list[prio].m_buffer.first();
        
        // Search task list for provided a_id.
        for (uint32_t i = 0; i < count; ++i)
        {
            if (a_id == m_context.m_task_list[prio].m_buffer.at(node_index))
            {
                m_context.m_task_list[prio].m_buffer.remove(node_index);
                break;
            }
            else
            {
                node_index = m_context.m_task_list[prio].m_buffer.nextIndex(node_index);
            }
        }
    }
    
    bool findNextTask(kernel::task::Priority a_priority, kernel::task::id & a_id)
    {
        const uint32_t prio = static_cast<uint32_t>(a_priority);
        const uint32_t count = m_context.m_task_list[prio].m_buffer.count();
        
        if (count > 1)
        {
            const uint32_t current = m_context.m_task_list[prio].m_current;
            
            uint32_t next_index = m_context.m_task_list[prio].m_buffer.nextIndex(current);
            
            a_id = m_context.m_task_list[prio].m_buffer.at(next_index);
            return true;
        }
        else
        {
            return false;
        }
    }
}
