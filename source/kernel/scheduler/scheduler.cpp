#include <scheduler.hpp>

#include <circular_list.hpp>
#include <handle.hpp>

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
            [] (internal::task::Id & a_left, volatile internal::task::Id & a_right) -> bool
            {
                return a_left == a_right;
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
        bool item_found = a_context.m_ready_list[prio].m_list.find( a_id, found_index,
            [] (internal::task::Id & a_left, volatile internal::task::Id & a_right) -> bool
            {
                return a_left == a_right;
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

            a_id = a_context.m_ready_list[prio].m_list.at(next_index);
            return true;
        }
        else if (count == 1U)
        {
            a_id = a_context.m_ready_list[prio].m_list.at(current);
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
            a_id = a_context.m_ready_list[prio].m_list.at(current);
            return true;
        }
        else
        {
            return false;
        }
    }
}

namespace kernel::internal::scheduler::wait_list
{
    bool addTask( Context & a_context, task::Id a_task_id)
    {
        uint32_t new_node_idx;
        return a_context.m_wait_list.add(a_task_id, new_node_idx);
    }

    void removeTask( Context & a_context, task::Id a_task_id)
    {
        uint32_t found_index;
        bool item_found = a_context.m_wait_list.find(
            a_task_id,
            found_index,
            [] (volatile internal::task::Id & a_left, volatile internal::task::Id & a_right) -> bool
            {
                return a_left == a_right;
            });

        if (item_found)
        {
            a_context.m_wait_list.remove(found_index);
        }
    }

    void iterate(
        wait_list::Context &        a_wait_list,
        ready_list::Context &       a_ready_list,
        internal::task::Context &   a_task_context,
        internal::timer::Context &  a_timer_context,
        internal::event::Context &  a_event_context,
        void        found(
            wait_list::Context &,
            ready_list::Context &,
            internal::task::Context &,
            internal::timer::Context &,
            internal::event::Context &,
            task::Id &
        )
    )
    {
        const uint32_t count = a_wait_list.m_wait_list.count();
        uint32_t item_index = a_wait_list.m_wait_list.firstIndex();

        for (uint32_t i = 0U; i < count; ++i)
        {
            internal::task::Id found_id = a_wait_list.m_wait_list.at(item_index);
            item_index = a_wait_list.m_wait_list.nextIndex(item_index);

            found(
                a_wait_list,
                a_ready_list,
                a_task_context,
                a_timer_context,
                a_event_context,
                found_id);
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
            // kernel::internal::task::state::set(
            //     a_task_context,
            //     a_context.m_current,
            //     kernel::task::State::Ready
            // );

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

            // TODO: move conditions checks to wait::Condtitions
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
