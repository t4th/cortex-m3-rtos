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

    bool setTaskToWait(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id                    a_task_id
    )
    {
        // todo: addTask can fail because there is no memory or because
        //       there is dublicate already added. Consider return value
        //       indicating that task is already added and only condition
        //        can be updated.
        bool task_added = wait_list::addTask( a_context.m_wait_list, a_task_id);

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

    // TODO: this function is just overkill
    void checkWaitConditions(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        internal::timer::Context &  a_timer_context,
        internal::event::Context &  a_event_context
    )
    {
        auto check_condition = [](
            wait_list::Context &        a_wait_list,
            ready_list::Context &       a_ready_list,
            internal::task::Context &   a_task_context,
            internal::timer::Context &  a_timer_context,
            internal::event::Context &  a_event_context,
            task::Id &                  a_task_id
            )
        {
            const kernel::Time_ms current = kernel::getTime();
            bool task_ready = false;

            volatile internal::task::wait::Conditions & conditions =
                internal::task::wait::getRef(
                    a_task_context,
                    a_task_id
                );

            // TODO: move conditions checks to wait::Conditions
            switch(conditions.m_type)
            {
                case task::wait::Conditions::Type::Sleep:
                {
                    if (current - conditions.m_start > conditions.m_interval)
                    {
                        task_ready = true;
                    }
                }
                break;
                case task::wait::Conditions::Type::WaitForObj:
                {
                    if (false == conditions.m_waitForver)
                    {
                        if (current - conditions.m_start > conditions.m_interval)
                        {
                            conditions.m_result = kernel::sync::WaitResult::TimeoutOccurred;
                            task_ready = true;
                        }
                    }

                    if (false == task_ready)
                    {
                        for (uint32_t i = 0; i < task::MAX_INPUT_SIGNALS; ++i)
                        {
                            if (false == conditions.m_inputSignals.isAllocated(i))
                            {
                                break;
                            }

                            volatile kernel::Handle & objHandle = conditions.m_inputSignals.at(i);
                            internal::handle::ObjectType objType =
                                internal::handle::getObjectType(objHandle);

                            switch (objType)
                            {
                            case internal::handle::ObjectType::Event:
                                {
                                auto event_id = internal::handle::getId<internal::event::Id>(objHandle);
                                event::State evState = event::getState(a_event_context, event_id);
                                if (event::State::Set == evState)
                                {
                                    conditions.m_result = kernel::sync::WaitResult::ObjectSet;
                                    task_ready = true;
                                }
                                break;
                                }
                            case internal::handle::ObjectType::Timer:
                                {
                                auto timer_id = internal::handle::getId<internal::timer::Id>(objHandle);
                                timer::State timerState = timer::getState(a_timer_context, timer_id);

                                if (timer::State::Finished == timerState)
                                {
                                    conditions.m_result = kernel::sync::WaitResult::ObjectSet;
                                    task_ready = true;
                                }
                                break;
                                }
                            default:
                                break;
                            };
                        }
                    }
                }
                break;
            }

            if (task_ready)
            {
                // TODO: removing waiting task should not be done from wait_list context..
                wait_list::removeTask(a_wait_list, a_task_id);

                kernel::task::Priority prio = 
                    task::priority::get(a_task_context, a_task_id);

                ready_list::addTask(a_ready_list, prio, a_task_id);
            }
        };

        wait_list::iterate(
            a_context.m_wait_list,
            a_context.m_ready_list,
            a_task_context,
            a_timer_context,
            a_event_context,
            check_condition
        );
    }
}
