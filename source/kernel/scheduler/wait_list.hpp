#pragma once

// Scheduler is used to order wihch task is to be served next.
// For each priority there is circular list holding task IDs.

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

    bool addTaskSleep(
        Context &   a_context,
        task::Id    a_task_id,
        Time_ms     a_interval
    );

    bool addTaskWaitObj(
        Context &           a_context,
        task::Id            a_task_id,
        kernel::Handle &    a_waitingSignal,
        bool                a_wait_forver,
        Time_ms             a_timeout
    );

    //check condition and removeTask if true
    void checkConditions(
        Context &                       a_context,
        internal::scheduler::Context &  a_scheduler_context,
        internal::task::Context &       a_task_context,
        internal::timer::Context &      a_timer_context,
        internal::event::Context &      a_event_context,
        void condition_fulfilled(
            internal::scheduler::Context &  a_scheduler_context,
            internal::task::Context &       a_task_context,
            volatile task::Id &             a_task_id
        )
    );

    void removeTask(
        Context & a_context,
        task::Id a_task_id
    );

}
