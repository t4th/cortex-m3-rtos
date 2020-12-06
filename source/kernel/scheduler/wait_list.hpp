#pragma once

// Scheduler is used to order wihch task is to be served next.
// For each priority there is circular list holding task IDs.

#include <circular_list.hpp>

#include <ready_list.hpp>
#include <task.hpp>
#include <timer.hpp>
#include <event.hpp>

namespace kernel::internal::scheduler::wait_list
{
    struct Context
    {
        volatile kernel::internal::common::CircularList<
            kernel::internal::task::Id,
            kernel::internal::task::MAX_NUMBER
        > m_wait_list{};
    };

    bool addTask( Context & a_context, task::Id a_task_id);
    void removeTask( Context & a_context, task::Id a_task_id);
    void iterate(
        wait_list::Context &        a_wait_list,
        ready_list::Context &       a_ready_list,
        internal::task::Context &   a_task_context,
        internal::timer::Context &  a_timer_context,
        internal::event::Context &  a_event_context,
        void        found(
            wait_list::Context &,
            ready_list::Context &,
            internal::task::Context &,
            internal::timer::Context &,
            internal::event::Context &,
            task::Id &
        )
    );
}
