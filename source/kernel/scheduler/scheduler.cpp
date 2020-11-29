#include <scheduler.hpp>

#include <circular_list.hpp>

#include <array>
#include <cstdint>

namespace kernel::internal::scheduler::ready_list
{
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
        uint32_t found_index;
        bool item_found = a_context.m_ready_list[prio].m_list.find( a_id, found_index,
            [] (internal::task::Id & a_left, internal::task::Id & a_right) -> bool
            {
                return a_left.m_id == a_right.m_id;
            });

        if (item_found)
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

        uint32_t found_index;
        bool item_found = a_context.m_ready_list[prio].m_list.find(
            a_id,
            found_index,
            [] (internal::task::Id & a_left, internal::task::Id & a_right) -> bool
            {
                return a_left.m_id == a_right.m_id;
            });

        if (item_found)
        {
            if (a_context.m_ready_list[prio].m_current == found_index) // If removed task is current task.
            {
                if (count > 1U) // Update m_current task ID.
                {
                    a_context.m_ready_list[prio].m_current = a_context.m_ready_list[prio].m_list.nextIndex(found_index);
                }
            }

            a_context.m_ready_list[prio].m_list.remove(found_index);

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
        uint32_t new_node_idx;
        a_context.m_wait_list.add(a_task_id, new_node_idx);

        kernel::internal::task::state::set(
            a_task_context,
            a_context.m_current,
            kernel::task::State::Waiting
        );
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

        // todo: remove task from wait and suspended list too
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
