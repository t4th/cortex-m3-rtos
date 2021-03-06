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

    // Type strong memory index for allocated WaitItem type.
    typedef common::MemoryBuffer< WaitItem, task::max_number>::Id MemoryBufferIndex;

    struct Context
    {
        // TODO: consider this a list to reduce search iterations.
        volatile common::MemoryBuffer<
            WaitItem,
            task::max_number
        > m_list{};
    };

    inline bool addTaskSleep(
        Context &   a_context,
        task::Id    a_task_id,
        TimeMs &    a_interval,
        TimeMs &    a_current
    )
    {
        MemoryBufferIndex item_index;

        bool allocated = a_context.m_list.allocate( item_index);

        if ( true == allocated)
        {
            a_context. m_list.at( item_index).m_id = a_task_id;
        }
        else
        {
            return false;
        }

        auto & conditions = a_context.m_list.at( item_index).m_conditions;

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
        TimeMs &            a_timeout,
        TimeMs &            a_current
    )
    {
        // Note: function allocate without checking for dublicates in m_list.
        MemoryBufferIndex item_index;

        bool allocated = a_context.m_list.allocate( item_index);

        if ( true == allocated)
        {
            a_context. m_list.at( item_index).m_id = a_task_id;
        }
        else
        {
            return false;
        }

        auto & conditions = a_context.m_list.at( item_index).m_conditions;

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
            return false;
        }

        return true;
    }

    inline void removeTask( Context & a_context, task::Id & a_task_id)
    {
        for ( uint32_t i = 0U; i < kernel::internal::task::max_number; ++i)
        {
            // TODO: it doesn't matter if memory is allocated or not. Consider removing.
            if ( true == a_context.m_list.isAllocated( static_cast< MemoryBufferIndex>( i)))
            {
                if ( a_context.m_list.at( static_cast< MemoryBufferIndex>( i)).m_id == a_task_id)
                {
                    a_context.m_list.free( static_cast< MemoryBufferIndex>( i));
                    break;
                }
            }
        }
    }
}
