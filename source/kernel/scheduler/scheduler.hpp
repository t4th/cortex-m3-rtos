#pragma once

#include <ready_list.hpp>
#include <wait_list.hpp>

// Scheduler is used to order wihch task is to be served next.
// It is state machine using m_current and m_next to evaluate
// arbitration queues.

// Scheduler is not responsible for allocating Task memory, but
// only uses internal::task:Id type, which is internal::task
// unique identifier.

// For scheduler it does not matter if provided task ID is valid or not.

namespace kernel::internal::scheduler
{
    struct Context
    {
        // Note: by design, Idle task must always be available
        //       and has Id = 0 and Idle priority.
        volatile kernel::internal::task::Id m_current{0U};
        volatile kernel::internal::task::Id m_next{0U};

        // Ready list.
        ready_list::Context m_ready_list{};

        // Wait list.
        wait_list::Context m_wait_list{};
    };

    // Public declarations:
    bool addReadyTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_task_id
    );

    bool addSuspendedTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_task_id
    );

    bool resumeSuspendedTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_task_id
    );

    void setTaskToSuspended(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_task_id
    );

    bool setTaskToSleep(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_task_id,
        Time_ms &                   a_interval,
        Time_ms &                   a_current
    );

    bool setTaskToWaitForObj(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_task_id,
        kernel::Handle *            a_wait_signals,
        uint32_t                    a_number_of_signals,
        bool                        a_wait_for_all_signals,
        bool &                      a_wait_forver,
        Time_ms &                   a_timeout,
        Time_ms &                   a_current
    );

    void removeTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_task_id
    );

    inline task::Id getCurrentTaskId( Context & a_context)
    {
        return a_context.m_current;
    }

    bool getNextTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_next_task_id
    );

    bool getCurrentTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_next_task_id
    );

    void checkWaitConditions(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        internal::timer::Context &  a_timer_context,
        internal::event::Context &  a_event_context,
        Time_ms &                   a_current
    );
}
