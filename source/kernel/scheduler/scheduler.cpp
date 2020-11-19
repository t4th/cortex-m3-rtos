#include <scheduler.hpp>

#include <circular_list.hpp>

#include <array>
#include <cstdint>

namespace kernel::internal::scheduler
{
    // private
    bool isDataAdded(
        kernel::internal::common::CircularList<
        kernel::internal::task::Id,
        kernel::internal::task::MAX_TASK_NUMBER
    > & a_list, kernel::internal::task::Id a_id)
    {
        if (a_list.count() == 0U)
        {
            return false;
        }

        uint32_t index = a_list.firstIndex();

        for (uint32_t i = 0U; i < a_list.count(); ++i)
        {
            if (a_list.at(index).m_id == a_id.m_id)
            {
                return true;
            }

            index = a_list.nextIndex(index);
        }

        return false;
    }

    // public
    bool addTask(
        kernel::internal::scheduler::Context &  a_context,
        kernel::task::Priority                  a_priority,
        kernel::internal::task::Id              a_id
    )
    {
        const uint32_t prio = static_cast<uint32_t>(a_priority);
        const uint32_t count = a_context.m_task_list[prio].m_list.count();
        
        // Look for dublicate.
        if (true == isDataAdded(a_context.m_task_list[prio].m_list, a_id))
        {
            return false;
        }

        uint32_t new_node_idx;
        
        bool task_added = a_context.m_task_list[prio].m_list.add(a_id, new_node_idx);
        
        if (false == task_added)
        {
            return false;
        }

        if (0U == count)
        {
            a_context.m_task_list[prio].m_current = new_node_idx;
        }

        return true;
    }
    
    void removeTask(
        kernel::internal::scheduler::Context &  a_context,
        kernel::task::Priority                  a_priority,
        kernel::internal::task::Id              a_id
    )
    {
        const uint32_t prio = static_cast<uint32_t>(a_priority);
        const uint32_t count = a_context.m_task_list[prio].m_list.count();
        
        uint32_t node_index = a_context.m_task_list[prio].m_list.firstIndex();
        
        // Search task list for provided a_id.
        for (uint32_t i = 0U; i < count; ++i)
        {
            if (a_id.m_id == a_context.m_task_list[prio].m_list.at(node_index).m_id)
            {
                if (a_context.m_task_list[prio].m_current == node_index) // If removed task i current task.
                {
                    if (count > 1) // Update current task.
                    {
                        a_context.m_task_list[prio].m_current = a_context.m_task_list[prio].m_list.nextIndex(node_index);
                    }
                }

                a_context.m_task_list[prio].m_list.remove(node_index);
                break;
            }
            else
            {
                node_index = a_context.m_task_list[prio].m_list.nextIndex(node_index);
            }
        }
    }
    
    bool findNextTask(
        kernel::internal::scheduler::Context &  a_context,
        kernel::task::Priority                  a_priority,
        kernel::internal::task::Id &            a_id
    )
    {
        const uint32_t prio = static_cast<uint32_t>(a_priority);
        const uint32_t count = a_context.m_task_list[prio].m_list.count();
        
        if (count > 1)
        {
            const uint32_t current = a_context.m_task_list[prio].m_current;
            const uint32_t next_index = a_context.m_task_list[prio].m_list.nextIndex(current);
            
            a_context.m_task_list[prio].m_current = next_index;

            a_id.m_id = a_context.m_task_list[prio].m_list.at(next_index).m_id;
            return true;
        }
        else
        {
            return false;
        }
    }

    void findHighestPrioTask(
        kernel::internal::scheduler::Context &  a_context,
        kernel::internal::task::Id &            a_id
    )
    {
        for (uint32_t prio = static_cast<uint32_t>(kernel::task::Priority::High);
            prio < kernel::internal::task::PRIORITIES_COUNT;
            ++prio)
        {
            const uint32_t count = a_context.m_task_list[prio].m_list.count();
            const uint32_t current = a_context.m_task_list[prio].m_current;

            if (count >= 1)
            {
                a_id.m_id = a_context.m_task_list[prio].m_list.at(current).m_id;
                break;
            }
            else
            {
                // No task in queue.
            }
        }
    }
}
