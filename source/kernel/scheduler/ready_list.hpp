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
        volatile TaskList m_ready_list[internal::task::PRIORITIES_COUNT];
    };

    // declarations
    bool addTask(
        ready_list::Context &       a_context,
        kernel::task::Priority      a_priority,
        kernel::internal::task::Id  a_id
    );

    void removeTask(
        ready_list::Context &       a_context,
        kernel::task::Priority      a_priority,
        kernel::internal::task::Id  a_id
    );

    // Find next task in selected priority group and UPDATE current task.
    bool findNextTask(
        ready_list::Context &           a_context,
        kernel::task::Priority          a_priority,
        kernel::internal::task::Id &    a_id
    );

    bool findCurrentTask(
        ready_list::Context &           a_context,
        kernel::task::Priority          a_priority,
        kernel::internal::task::Id &    a_id
    );
}
