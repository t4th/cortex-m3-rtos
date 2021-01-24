#pragma once

// Ready list is used to order which task is to be served next.
// For each priority there is circular list holding task IDs.

#include "common/circular_list.hpp"
#include "task/task.hpp"

namespace kernel::internal::scheduler::ready_list
{
    struct TaskList
    {
        kernel::internal::common::CircularList<
            kernel::internal::task::Id,
            kernel::internal::task::max_number
        > m_list{};

        uint32_t m_current{ 0U};
    };

    struct Context
    {
        volatile TaskList m_ready_list[ internal::task::priorities_count]{};
    };

    inline bool addTask(
        ready_list::Context &        a_context,
        kernel::task::Priority &     a_priority,
        kernel::internal::task::Id & a_id
    )
    {
        const uint32_t priority = static_cast< uint32_t>( a_priority);
        const uint32_t count = a_context.m_ready_list[ priority].m_list.count();

        // Look for dublicate.
        uint32_t found_index;

        bool item_found = a_context.m_ready_list[ priority].m_list.find( a_id, found_index);

        if ( true == item_found)
        {
            return false;
        }

        // Add task to ready list.
        uint32_t new_node_idx;

        bool task_added = a_context.m_ready_list[ priority].m_list.add( a_id, new_node_idx);

        if ( false == task_added)
        {
            return false;
        }

        // Note: When first task is added to the ready list m_current must be updated.
        if ( 0U == count)
        {
            a_context.m_ready_list[ priority].m_current = new_node_idx;
        }

        return true;
    }

    inline void removeTask(
        ready_list::Context &        a_context,
        kernel::task::Priority &     a_priority,
        kernel::internal::task::Id & a_id
    )
    {
        const uint32_t priority = static_cast< uint32_t>( a_priority);
        const uint32_t count = a_context.m_ready_list[ priority].m_list.count();

        uint32_t found_index;

        bool item_found = a_context.m_ready_list[ priority].m_list.find( a_id, found_index);

        if ( true == item_found)
        {
            // If removed task is current task.
            if ( a_context.m_ready_list[ priority].m_current == found_index)
            {
                // Update m_current task ID.
                if ( count > 1U)
                {
                    a_context.m_ready_list[ priority].m_current = a_context.m_ready_list[ priority].m_list.nextIndex(found_index);
                }
            }

            a_context.m_ready_list[ priority].m_list.remove( found_index);
        }
    }

    // Find next task in selected priority group and UPDATE current task.
    inline bool findNextTask(
        ready_list::Context &           a_context,
        kernel::task::Priority &        a_priority,
        kernel::internal::task::Id &    a_id
    )
    {
        const uint32_t priority = static_cast< uint32_t>( a_priority);
        const uint32_t count = a_context.m_ready_list[ priority].m_list.count();
        const uint32_t current_task_index = a_context.m_ready_list[ priority].m_current;

        if ( count > 1U)
        {
            const uint32_t next_task_index = a_context.m_ready_list[ priority].m_list.nextIndex( current_task_index);

            a_context.m_ready_list[ priority].m_current = next_task_index;
            a_id = a_context.m_ready_list[ priority].m_list.at( next_task_index);

            return true;
        }
        else if ( 1U == count)
        {
            a_id = a_context.m_ready_list[ priority].m_list.at( current_task_index);

            return true;
        }
        else
        {
            return false;
        }
    }

    inline bool findCurrentTask(
        ready_list::Context &           a_context,
        kernel::task::Priority &        a_priority,
        kernel::internal::task::Id &    a_id
    )
    {
        const uint32_t priority = static_cast< uint32_t>( a_priority);
        const uint32_t count = a_context.m_ready_list[ priority].m_list.count();
        const uint32_t current_task_index = a_context.m_ready_list[ priority].m_current;

        if ( count > 0U)
        {
            a_id = a_context.m_ready_list[ priority].m_list.at( current_task_index);

            return true;
        }
        else
        {
            return false;
        }
    }
}
