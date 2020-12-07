#include <scheduler.hpp>

#include <handle.hpp>

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
        task::Id                    a_task_id
    )
    {
        wait_list::removeTask(
            a_context.m_wait_list,
            a_task_id
        );

        kernel::task::Priority prio = internal::task::priority::get(
            a_task_context,
            a_task_id
        );

        bool task_added = addReadyTask(a_context, a_task_context, prio, a_task_id);

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

    bool setTaskToSleep(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id                    a_task_id,
        Time_ms                     a_interval
    )
    {
        bool task_added = wait_list::addTaskSleep(
            a_context.m_wait_list,
            a_task_id,
            a_interval
            );

        if (false == task_added)
        {
            return false;
        }

        kernel::task::Priority prio = internal::task::priority::get(
            a_task_context,
            a_task_id
        );

        ready_list::removeTask(
            a_context.m_ready_list,
            prio,
            a_task_id
        );

        kernel::internal::task::state::set(
            a_task_context,
            a_task_id,
            kernel::task::State::Waiting
        );

        return true;
    }

    bool setTaskToWaitForObj(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id                    a_task_id,
        kernel::Handle &            a_waitingSignal,
        bool                        a_wait_forver,
        Time_ms                     a_timeout
    )
    {
        bool task_added = wait_list::addTaskWaitObj(
            a_context.m_wait_list,
            a_task_id,
            a_waitingSignal,
            a_wait_forver,
            a_timeout
        );

        if (false == task_added)
        {
            return false;
        }

        kernel::task::Priority prio = internal::task::priority::get(
            a_task_context,
            a_task_id
        );

        ready_list::removeTask(
            a_context.m_ready_list,
            prio,
            a_task_id
        );

        kernel::internal::task::state::set(
            a_task_context,
            a_task_id,
            kernel::task::State::Waiting
        );

        return true;
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

        wait_list::removeTask(
            a_context.m_wait_list,
            a_task_id
        );

        // todo: remove task suspended list
    }

    void getCurrentTaskId(
        Context &   a_context,
        task::Id &  a_current_task_id
    )
    {
        a_current_task_id = a_context.m_current;
    }

    // Search for highest priority task in all priority queues.
    // return true when task is found.
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
            // todo: Trying to access not allocated task context.
#if 0
            kernel::internal::task::state::set(
                a_task_context,
                a_context.m_current,
                kernel::task::State::Ready
            );
#endif

            kernel::internal::task::state::set(
                a_task_context,
                a_context.m_next,
                kernel::task::State::Running
            );

            a_context.m_current = a_context.m_next;
        }

        return next_task_found;
    }

    // TODO: I dont like this lambda interface, its obfuscated simple thing
    //       like iterating through array..
    void checkWaitConditions(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        internal::timer::Context &  a_timer_context,
        internal::event::Context &  a_event_context
    )
    {
        auto wait_condition_met = [] (
            internal::scheduler::Context &  a_scheduler_context,
            internal::task::Context &       a_task_context,
            volatile task::Id &             a_task_id
            )
        {
            kernel::task::Priority prio = 
                task::priority::get(a_task_context, a_task_id);

            bool task_added = addReadyTask(a_scheduler_context, a_task_context, prio, a_task_id);

            if (task_added)
            {
                kernel::internal::task::state::set(
                    a_task_context,
                    a_task_id,
                    kernel::task::State::Ready
                );
            }
        };

        wait_list::checkConditions(
            a_context.m_wait_list,
            a_context,
            a_task_context,
            a_timer_context,
            a_event_context,
            wait_condition_met
        );
    }
}
