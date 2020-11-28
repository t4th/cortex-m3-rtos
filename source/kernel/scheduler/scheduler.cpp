#include <scheduler.hpp>

#include <circular_list.hpp>

#include <array>
#include <cstdint>

namespace kernel::internal::scheduler::ready_list
{
    // private
    bool isDataAdded(
        kernel::internal::common::CircularList<
        kernel::internal::task::Id,
        kernel::internal::task::MAX_NUMBER
        > & a_list, kernel::internal::task::Id a_id)
    {
        if (a_list.count() == 0U)
        {
            return false;
        }

        uint32_t index = a_list.firstIndex();

        for (uint32_t i = 0U; i < a_list.count(); ++i)
        {
            if (a_list.at(index).m_id == a_id.m_id)
            {
                return true;
            }

            index = a_list.nextIndex(index);
        }

        return false;
    }

    // public
    bool addTask(
        ready_list::Context &       a_context,
        kernel::task::Priority      a_priority,
        kernel::internal::task::Id  a_id
    )
    {
        const uint32_t prio = static_cast<uint32_t>(a_priority);
        const uint32_t count = a_context.m_ready_list[prio].m_list.count();

        // Look for dublicate.
        if (true == isDataAdded(a_context.m_ready_list[prio].m_list, a_id))
        {
            return false;
        }

        uint32_t new_node_idx;

        bool task_added = a_context.m_ready_list[prio].m_list.add(a_id, new_node_idx);

        if (false == task_added)
        {
            return false;
        }

        if (0U == count)
        {
            a_context.m_ready_list[prio].m_current = new_node_idx;
        }

        return true;
    }

    void removeTask(
        ready_list::Context &       a_context,
        kernel::task::Priority      a_priority,
        kernel::internal::task::Id  a_id
    )
    {
        const uint32_t prio = static_cast<uint32_t>(a_priority);
        const uint32_t count = a_context.m_ready_list[prio].m_list.count();

        uint32_t node_index = a_context.m_ready_list[prio].m_list.firstIndex();

        // Search task list for provided a_id.
        for (uint32_t i = 0U; i < count; ++i)
        {
            if (a_id.m_id == a_context.m_ready_list[prio].m_list.at(node_index).m_id)
            {
                if (a_context.m_ready_list[prio].m_current == node_index) // If removed task is current task.
                {
                    if (count > 1U) // Update m_current task ID.
                    {
                        a_context.m_ready_list[prio].m_current = a_context.m_ready_list[prio].m_list.nextIndex(node_index);
                    }
                }

                a_context.m_ready_list[prio].m_list.remove(node_index);
                break;
            }
            else
            {
                node_index = a_context.m_ready_list[prio].m_list.nextIndex(node_index);
            }
        }
    }

    bool findNextTask(
        ready_list::Context &           a_context,
        kernel::task::Priority          a_priority,
        kernel::internal::task::Id &    a_id
    )
    {
        const uint32_t prio = static_cast<uint32_t>(a_priority);
        const uint32_t count = a_context.m_ready_list[prio].m_list.count();

        const uint32_t current = a_context.m_ready_list[prio].m_current;

        if (count > 1U)
        {
            const uint32_t next_index = a_context.m_ready_list[prio].m_list.nextIndex(current);

            a_context.m_ready_list[prio].m_current = next_index;

            a_id.m_id = a_context.m_ready_list[prio].m_list.at(next_index).m_id;
            return true;
        }
        else if (count == 1U)
        {
            a_id.m_id = a_context.m_ready_list[prio].m_list.at(current).m_id;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool findCurrentTask(
        ready_list::Context &           a_context,
        kernel::task::Priority          a_priority,
        kernel::internal::task::Id &    a_id
    )
    {
        const uint32_t prio = static_cast<uint32_t>(a_priority);
        const uint32_t count = a_context.m_ready_list[prio].m_list.count();
        const uint32_t current = a_context.m_ready_list[prio].m_current;

        if (count > 0U)
        {
            a_id.m_id = a_context.m_ready_list[prio].m_list.at(current).m_id;
            return true;
        }
        else
        {
            return false;
        }
    }
}

namespace kernel::internal::scheduler
{
    // public declarations
    bool addReadyTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        kernel::task::Priority      a_priority,
        task::Id                    a_task_id
    )
    {
        bool task_added = ready_list::addTask(
            a_context.m_ready_list,
            a_priority,
            a_task_id
        );

        if (!task_added)
        {
            return false;
        }

        return true;
    }

    bool addSuspendedTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        kernel::task::Priority      a_priority,
        task::Id                    a_task_id
    )
    {
        // TODO: implement suspended list
        return true;
    }

    bool setTaskToReady(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        kernel::task::Priority      a_priority,
        task::Id                    a_task_id
    )
    {
        bool task_added = addReadyTask(a_context, a_task_context, a_priority, a_task_id);

        if (task_added)
        {
            kernel::internal::task::state::set(
                a_task_context,
                a_context.m_current,
                kernel::task::State::Ready
            );
        }

        return task_added;
    }

    void setTaskToSuspended(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id                    a_task_id
    )
    {
        kernel::internal::task::state::set(
            a_task_context,
            a_task_id,
            kernel::task::State::Suspended
        );

        // TODO: implement suspended list
        removeTask(a_context, a_task_context, a_task_id);
    }

    void setTaskToWait(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id                    a_task_id
    )
    {
        // TODO: implement waiting list
    }

    // removing task, update current with next
    void removeTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id                    a_task_id
    )
    {
        kernel::task::Priority prio = internal::task::priority::get(
            a_task_context,
            a_task_id
        );

        ready_list::removeTask(
            a_context.m_ready_list,
            prio,
            a_task_id
        );
    }

    void getCurrentTaskId(
        Context & a_context,
        task::Id & a_current_task_id
    )
    {
        a_current_task_id = a_context.m_current;
    }

    bool getNextTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_next_task_id
    )
    {
        bool next_task_found = false;

        for (uint32_t prio = static_cast<uint32_t>(kernel::task::Priority::High);
            prio < kernel::internal::task::PRIORITIES_COUNT;
            ++prio)
        {
            next_task_found = ready_list::findNextTask(
                a_context.m_ready_list,
                static_cast<kernel::task::Priority>(prio),
                a_next_task_id
            );

            if (next_task_found)
            {
                a_context.m_next = a_next_task_id;
                break;
            }
        }

        if (next_task_found)
        {
            kernel::internal::task::state::set(
                a_task_context,
                a_context.m_current,
                kernel::task::State::Ready
            );

            kernel::internal::task::state::set(
                a_task_context,
                a_context.m_next,
                kernel::task::State::Running
            );

            a_context.m_current = a_context.m_next;
        }

        return next_task_found;
    }

    bool getCurrentTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_next_task_id
    )
    {
        bool next_task_found = false;

        for (uint32_t prio = static_cast<uint32_t>(kernel::task::Priority::High);
            prio < kernel::internal::task::PRIORITIES_COUNT;
            ++prio)
        {
            next_task_found = ready_list::findCurrentTask(
                a_context.m_ready_list,
                static_cast<kernel::task::Priority>(prio),
                a_next_task_id
            );

            if (next_task_found)
            {
                a_context.m_next = a_next_task_id;
                break;
            }
        }

        if (next_task_found)
        {
            kernel::internal::task::state::set(
                a_task_context,
                a_context.m_current,
                kernel::task::State::Ready
            );

            kernel::internal::task::state::set(
                a_task_context,
                a_context.m_next,
                kernel::task::State::Running
            );

            a_context.m_current = a_context.m_next;
        }

        return next_task_found;
    }
}
