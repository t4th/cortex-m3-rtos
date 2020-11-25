#pragma once

// Scheduler is used to order wihch task is to be served next.
// For each priority there is circular list holding task IDs.

#include <circular_list.hpp>
#include <task.hpp>

namespace kernel::internal::scheduler
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
        // Each priority has its own TaskList.
        std::array <TaskList, kernel::internal::task::PRIORITIES_COUNT> m_task_list;
    };

    bool addTask(
        kernel::internal::scheduler::Context &  a_context,
        kernel::task::Priority                  a_priority,
        kernel::internal::task::Id              a_id
    );
    
    void removeTask(
        kernel::internal::scheduler::Context &  a_context,
        kernel::task::Priority                  a_priority,
        kernel::internal::task::Id              a_id
    );
    
    // Find next task in selected priority group and UPDATE current task.
    bool findNextTask(
        kernel::internal::scheduler::Context &  a_context,
        kernel::task::Priority                  a_priority,
        kernel::internal::task::Id &            a_id
    );

    // return id of highest priority task.
    // idle task is always available as lowest possible priority thus function always success.
    void findHighestPrioTask(
        kernel::internal::scheduler::Context &  a_context,
        kernel::internal::task::Id &            a_id
    );
}
