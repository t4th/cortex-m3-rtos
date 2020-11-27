#pragma once

// Scheduler is used to order wihch task is to be served next.
// For each priority there is circular list holding task IDs.

#include <circular_list.hpp>
#include <task.hpp>

namespace kernel::internal::scheduler::ready_list
{
    struct TaskList
    {
        kernel::internal::common::CircularList<
            kernel::internal::task::Id,
            kernel::internal::task::MAX_NUMBER
        > m_list;

        uint32_t m_current;

        TaskList() : m_list{}, m_current{0U} {}
    };

    struct Context
    {
        // Each priority has its own Ready list.
        std::array <TaskList, kernel::internal::task::PRIORITIES_COUNT> m_ready_list;
    };

    // declarations
    bool addTask(
        kernel::internal::scheduler::ready_list::Context &  a_context,
        kernel::task::Priority                  a_priority,
        kernel::internal::task::Id              a_id
    );

    void removeTask(
        kernel::internal::scheduler::ready_list::Context &  a_context,
        kernel::task::Priority                  a_priority,
        kernel::internal::task::Id              a_id
    );

    // Find next task in selected priority group and UPDATE current task.
    bool findNextTask(
        kernel::internal::scheduler::ready_list::Context &  a_context,
        kernel::task::Priority                  a_priority,
        kernel::internal::task::Id &            a_id
    );
}

namespace kernel::internal::scheduler
{
    struct Context
    {
        // Each priority has its own Ready list.
        kernel::internal::task::Id m_current; // Indicate currently running task ID.
        kernel::internal::task::Id m_next;    // Indicate next task ID.

        // ready list
        ready_list::Context m_ready_list;

        // m_wait_list

        // m_suspended_list
    };

    // public declarations
    bool addReadyTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        kernel::task::Priority      a_priority,
        task::Id                    a_task_id
    );

    bool addSuspendedTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        kernel::task::Priority      a_priority,
        task::Id                    a_task_id
    );

    bool setTaskToReady(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        kernel::task::Priority      a_priority,
        task::Id                    a_task_id
    );

    void setTaskToSuspended(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id                    a_task_id
    );

    void setTaskToWait(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id                    a_task_id
    );

    // removing task, update current with next
    void removeTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id                    a_task_id
    );

    void getCurrentTask(Context & a_context, task::Id & a_current_task_id);

    // find next task and set current = next
    bool getNextTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_next_task_id
    );
}
