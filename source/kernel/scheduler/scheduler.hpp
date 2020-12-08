#pragma once

#include <ready_list.hpp>
#include <wait_list.hpp>

// Scheduler is used to order wihch task is to be served next.
// It is state machine using m_current and m_next to evaluate
// arbitration queues.
namespace kernel::internal::scheduler
{
    struct Context
    {
        // Note: by design, Idle task always must be available and has Id = 0.
        volatile kernel::internal::task::Id m_current{0U}; // Indicate currently running task ID.
        volatile kernel::internal::task::Id m_next{0U};    // Indicate next task ID.

        // ready list
        ready_list::Context m_ready_list{};

        // m_wait_list
        
        wait_list::Context m_wait_list{};

        // todo: m_suspended_list
    };

    // public declarations
    bool addReadyTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id                    a_task_id
    );

    bool addSuspendedTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id                    a_task_id
    );

    bool setTaskToReady(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id                    a_task_id
    );

    void setTaskToSuspended(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id                    a_task_id
    );

    bool setTaskToSleep(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id                    a_task_id,
        Time_ms                     a_interval,
        Time_ms                     a_current
    );

    bool setTaskToWaitForObj(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id                    a_task_id,
        kernel::Handle &            a_waitingSignal,
        bool                        a_wait_forver,
        Time_ms                     a_timeout,
        Time_ms                     a_current
    );

    // removing task, update current with next
    void removeTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id                    a_task_id
    );


    inline task::Id getCurrentTaskId( Context & a_context)
    {
        return a_context.m_current;
    }

    // find next task and update current = next
    // function assume that there is at least one task available
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
        Time_ms                     a_current
    );
}
