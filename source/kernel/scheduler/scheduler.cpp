#include <scheduler.hpp>

#include <handle.hpp>

namespace kernel::internal::scheduler
{
    bool addReadyTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_task_id
    )
    {
        kernel::task::Priority priority = 
            task::priority::get(a_task_context, a_task_id);

        bool task_added = ready_list::addTask(
            a_context.m_ready_list,
            priority,
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
        task::Id &                  a_task_id
    )
    {
        // TODO: implement suspended list
        kernel::internal::task::state::set(
            a_task_context,
            a_task_id,
            kernel::task::State::Suspended
        );

        return true;
    }

    bool resumeSuspendedTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_task_id
    )
    {
        const auto resumed_task_state = kernel::internal::task::state::get(
                a_task_context,
                a_task_id
            );

        if (kernel::task::State::Suspended != resumed_task_state)
        {
            return false;
        }

        wait_list::removeTask(
            a_context.m_wait_list,
            a_task_id
        );

        bool task_added = addReadyTask(a_context, a_task_context, a_task_id);

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
        task::Id &                  a_task_id
    )
    {
        kernel::internal::task::state::set(
            a_task_context,
            a_task_id,
            kernel::task::State::Suspended
        );

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
    }

    bool setTaskToSleep(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_task_id,
        Time_ms &                   a_interval,
        Time_ms &                   a_current
    )
    {
        bool task_added = wait_list::addTaskSleep(
            a_context.m_wait_list,
            a_task_id,
            a_interval,
            a_current
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
        task::Id &                  a_task_id,
        kernel::Handle &            a_waitingSignal,
        bool &                      a_wait_forver,
        Time_ms &                   a_timeout,
        Time_ms &                   a_current
    )
    {
        bool task_added = wait_list::addTaskWaitObj(
            a_context.m_wait_list,
            a_task_id,
            a_waitingSignal,
            a_wait_forver,
            a_timeout,
            a_current
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

    void removeTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_task_id
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
            kernel::task::Priority priority = static_cast<kernel::task::Priority>(prio);

            next_task_found = ready_list::findNextTask(
                a_context.m_ready_list,
                priority,
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
            kernel::task::Priority priority = static_cast<kernel::task::Priority>(prio);

            next_task_found = ready_list::findCurrentTask(
                a_context.m_ready_list,
                priority,
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
                a_context.m_next,
                kernel::task::State::Running
            );

            a_context.m_current = a_context.m_next;
        }

        return next_task_found;
    }

    // Note: I don't like this super-function, but iterating over array should
    //       not be obfuscated by too many interface. Previous implementation
    //       included passing lambda with multitude of arguments ignoring 
    //       top-down architecture principle.
    //
    //       Now Wait list data is accessed directly skipping Wait list interface.
    void checkWaitConditions(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        internal::timer::Context &  a_timer_context,
        internal::event::Context &  a_event_context,
        Time_ms &                   a_current
    )
    {
        // Iterate over WaitItem which hold conditions used to wake up
        // waiting task.
        for (uint32_t i = 0U; i < kernel::internal::task::MAX_NUMBER; ++i)
        {
            auto & current_wait_item = a_context.m_wait_list.m_list;

            if (true == current_wait_item.isAllocated(i))
            {
                kernel::sync::WaitResult a_wait_result;
                auto & conditions = current_wait_item.at(i).m_conditions;

                bool is_condition_fulfilled =
                    wait::check(
                        conditions,
                        a_timer_context,
                        a_event_context,
                        a_wait_result,
                        a_current
                    );

                if (true == is_condition_fulfilled)
                {
                    task::Id ready_task = current_wait_item.at(i).m_id;

                    bool task_added = addReadyTask(
                        a_context,
                        a_task_context,
                        ready_task
                    );

                    if (task_added)
                    {
                        kernel::internal::task::state::set(
                            a_task_context,
                            ready_task,
                            kernel::task::State::Ready
                        );

                        task::wait::set(
                            a_task_context,
                            ready_task,
                            a_wait_result);

                        current_wait_item.free(i);
                    }
                    else
                    {
                        hardware::debug::setBreakpoint();
                    }
                }
            }
        }
    }
}
