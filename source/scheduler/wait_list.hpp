#pragma once

#include "scheduler/wait_conditions.hpp"

#include "task/task.hpp"

// Wait List keep Identifiers of tasks in Wait state and their waking up
// conditions.

// Note: I tried keeping those information within internal::task,
//       but it just bloated task structure and needed internal::task
//       to have information about the whole kernel context, because
//       of conditon check in kernel::internal::scheduler::wait::check.

// TODO: consider simplyfing this. Maybe iteration through tasks buffer
//       and just checking if task::state is waiting more clear?
namespace kernel::internal::scheduler::wait_list
{
    struct WaitItem
    {
        internal::task::Id  m_id;
        wait::Conditions    m_conditions;
    };

    struct Context
    {
        // TODO: consider this a list to reduce search iterations.
        volatile common::MemoryBuffer<
            WaitItem,
            task::MAX_NUMBER
        > m_list{};
    };

    inline bool addTaskSleep(
        Context &   a_context,
        task::Id    a_task_id,
        Time_ms &   a_interval,
        Time_ms &   a_current
    )
    {
        uint32_t    item_index;
        bool        allocated = a_context.m_list.allocate( item_index);

        if ( true == allocated)
        {
            a_context. m_list.at( item_index).m_id = a_task_id;
        }
        else
        {
            return false;
        }

        volatile wait::Conditions & conditions =
            a_context.m_list.at( item_index).m_conditions;

        wait::initSleep( conditions, a_interval, a_current);

        return true;
    }

    inline bool addTaskWaitObj(
        Context &           a_context,
        task::Id            a_task_id,
        kernel::Handle *    a_wait_signals,
        uint32_t            a_number_of_signals,
        bool &              a_wait_for_all_signals,
        bool &              a_wait_forver,
        Time_ms &           a_timeout,
        Time_ms &           a_current
    )
    {
        // Note: function allocate without checking for dublicates in m_list.
        uint32_t    item_index;
        bool        allocated = a_context.m_list.allocate( item_index);

        if ( true == allocated)
        {
            a_context. m_list.at( item_index).m_id = a_task_id;
        }
        else
        {
            return false;
        }

        volatile wait::Conditions & conditions =
            a_context.m_list.at( item_index).m_conditions;

        bool init_succesful = wait::initWaitForObj(
            conditions,
            a_wait_signals,
            a_number_of_signals,
            a_wait_for_all_signals,
            a_wait_forver,
            a_timeout,
            a_current
        );

        if ( false == init_succesful)
        {
            assert( true);
        }

        return true;
    }

    inline void removeTask(
        Context & a_context,
        task::Id & a_task_id
    )
    {
        for ( uint32_t i = 0U; i < kernel::internal::task::MAX_NUMBER; ++i)
        {
            // TODO: it doesn't matter if memory is allocated or not. Consider removing.
            if ( true == a_context.m_list.isAllocated( i))
            {
                if ( a_context.m_list.at( i).m_id == a_task_id)
                {
                    a_context.m_list.free( i);
                    break;
                }
            }
        }
    }
}
