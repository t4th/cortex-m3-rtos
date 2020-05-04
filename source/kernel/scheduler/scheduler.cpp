#include <scheduler.hpp>

#include <circular_list.hpp>

#include <array>
#include <cstdint>

namespace
{
    // Track current task for given priority.
    struct TaskList
    {
        kernel::common::CircularList<uint32_t, kernel::task::MAX_TASK_NUMBER>::Context m_context;
        kernel::common::CircularList<uint32_t, kernel::task::MAX_TASK_NUMBER> m_buffer;
        
        uint32_t m_current;
        
        TaskList() : m_context{}, m_buffer{m_context}, m_current{0U} {}
    };
    
    // Create task lists, each for one priority.
    struct
    {
        std::array <TaskList, kernel::task::PRIORITIES_COUNT> m_task_list;
        
    } m_context;
}

namespace kernel::scheduler
{
    bool addTask(kernel::task::Priority a_priority, kernel::task::Id a_id)
    {
        const uint32_t prio = static_cast<uint32_t>(a_priority);
        const uint32_t count = m_context.m_task_list[prio].m_buffer.count();
        
        uint32_t new_node_idx;
        
        bool task_added = m_context.m_task_list[prio].m_buffer.add(a_id, new_node_idx);
        
        if (false == task_added)
        {
            return false;
        }

        if (0 == count)
        {
            m_context.m_task_list[prio].m_current = new_node_idx;
        }

        return true;
    }
    
    void removeTask(kernel::task::Priority a_priority, kernel::task::Id a_id)
    {
        const uint32_t prio = static_cast<uint32_t>(a_priority);
        const uint32_t count = m_context.m_task_list[prio].m_buffer.count();
        
        uint32_t node_index = m_context.m_task_list[prio].m_buffer.firstIndex();
        
        // Search task list for provided a_id.
        for (uint32_t i = 0; i < count; ++i)
        {
            if (a_id == m_context.m_task_list[prio].m_buffer.at(node_index))
            {
                if (m_context.m_task_list[prio].m_current == node_index) // If removed task i current task.
                {
                    if (count > 1) // Update current task.
                    {
                        m_context.m_task_list[prio].m_current = m_context.m_task_list[prio].m_buffer.nextIndex(node_index);
                    }
                }

                m_context.m_task_list[prio].m_buffer.remove(node_index);
                break;
            }
            else
            {
                node_index = m_context.m_task_list[prio].m_buffer.nextIndex(node_index);
            }
        }
    }
    
    bool findNextTask(kernel::task::Priority a_priority, kernel::task::Id & a_id)
    {
        const uint32_t prio = static_cast<uint32_t>(a_priority);
        const uint32_t count = m_context.m_task_list[prio].m_buffer.count();
        
        if (count > 1)
        {
            const uint32_t current = m_context.m_task_list[prio].m_current;
            const uint32_t next_index = m_context.m_task_list[prio].m_buffer.nextIndex(current);
            
            m_context.m_task_list[prio].m_current = next_index;

            a_id = m_context.m_task_list[prio].m_buffer.at(next_index);
            return true;
        }
        else
        {
            return false;
        }
    }
}
