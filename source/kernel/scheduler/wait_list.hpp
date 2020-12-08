#pragma once

// TODO: consider removing wait_list interface layer, leaving
//       only POD array used by scheduler.

#include <wait_conditions.hpp>

#include <task.hpp>

namespace kernel::internal::scheduler
{
    struct Context;
}

namespace kernel::internal::scheduler::wait_list
{
    struct WaitItem
    {
        internal::task::Id  m_id;
        wait::Conditions    m_conditions;
    };

    struct Context
    {
        common::MemoryBuffer<
            WaitItem,
            task::MAX_NUMBER
        > m_list{};
    };

    inline bool addTaskSleep(
        Context &   a_context,
        task::Id    a_task_id,
        Time_ms     a_interval,
        Time_ms     a_current
    )
    {
        uint32_t    item_index;
        bool        allocated = a_context.m_list.allocate(item_index);

        if (true == allocated)
        {
            a_context. m_list.at(item_index).m_id = a_task_id;
        }
        else
        {
            return false;
        }

        volatile wait::Conditions & conditions =
            a_context.m_list.at(item_index).m_conditions;

        wait::initSleep(conditions, a_interval, a_current);

        return true;
    }

    inline bool addTaskWaitObj(
        Context &           a_context,
        task::Id            a_task_id,
        kernel::Handle &    a_waitingSignal,
        bool                a_wait_forver,
        Time_ms             a_timeout,
        Time_ms             a_current
    )
    {
        // Note: function allocate without checking for dublicates in m_list.
        uint32_t    item_index;
        bool        allocated = a_context.m_list.allocate(item_index);

        if (true == allocated)
        {
            a_context. m_list.at(item_index).m_id = a_task_id;
        }
        else
        {
            return false;
        }

        volatile wait::Conditions & conditions =
            a_context.m_list.at(item_index).m_conditions;

        wait::initWaitForObj(
            conditions,
            a_waitingSignal,
            a_waitingSignal,
            a_timeout,
            a_current
        );

        return true;
    }

    inline void removeTask(
        Context & a_context,
        task::Id & a_task_id
    )
    {
        for (uint32_t i = 0U; i < kernel::internal::task::MAX_NUMBER; ++i)
        {
            // TODO: it doesn't matter if memmory is allocated or not. Consider removing.
            if (true == a_context.m_list.isAllocated(i))
            {
                if (a_context.m_list.at(i).m_id == a_task_id)
                {
                    a_context.m_list.free(i);
                    break;
                }
            }
        }
    }
}
