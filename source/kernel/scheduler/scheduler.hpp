#pragma once

#include <circular_list.hpp>
#include <task.hpp>

namespace kernel::internal::scheduler
{
    struct TaskList
    {
        kernel::common::CircularList<kernel::task::Id,
            kernel::internal::task::MAX_TASK_NUMBER> m_list;

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
        kernel::task::Id                        a_id
    );
    
    void removeTask(
        kernel::internal::scheduler::Context &  a_context,
        kernel::task::Priority                  a_priority,
        kernel::task::Id                        a_id
    );
    
    // Find next task in selected priority group and UPDATE current task.
    bool findNextTask(
        kernel::internal::scheduler::Context &  a_context,
        kernel::task::Priority                  a_priority,
        kernel::task::Id &                      a_id
    );

    // return id of highest priority task.
    // idle task is always available as lowest possible priority thus function always success.
    void findHighestPrioTask(
        kernel::internal::scheduler::Context &  a_context,
        kernel::task::Id &                      a_id
    );
}
