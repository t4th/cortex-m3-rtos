#pragma once

#include "scheduler/ready_list.hpp"
#include "scheduler/wait_list.hpp"

#include "handle/handle.hpp"

// Scheduler is used to order which task is to be served next.
// It is state machine using m_current and m_next to evaluate
// arbitration queues.

// Scheduler is not responsible for allocating Task memory, but
// only uses internal::task:Id type, which is internal::task
// unique identifier.

// For scheduler it does not matter if provided task ID is valid or not.

namespace kernel::internal::scheduler
{
    struct Context
    {
        // Note: by design, Idle task must always be available
        //       ,has Id = 0 and Idle priority. Otherwise UB.
        volatile kernel::internal::task::Id m_current{ 0U};
        volatile kernel::internal::task::Id m_next{ 0U};

        // Ready list.
        ready_list::Context m_ready_list{};

        // Wait list.
        wait_list::Context m_wait_list{};
    };

    inline bool addReadyTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_task_id
    )
    {
        kernel::task::Priority priority = 
            task::priority::get( a_task_context, a_task_id);

        bool task_added = ready_list::addTask(
            a_context.m_ready_list,
            priority,
            a_task_id
        );

        if ( false == task_added)
        {
            return false;
        }

        return true;
    }

    inline bool addSuspendedTask(
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

    inline bool resumeSuspendedTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_task_id
    )
    {
        const auto resumed_task_state = kernel::internal::task::state::get(
            a_task_context,
            a_task_id
        );

        if ( kernel::task::State::Suspended != resumed_task_state)
        {
            return false;
        }

        wait_list::removeTask(
            a_context.m_wait_list,
            a_task_id
        );

        bool task_added = addReadyTask(a_context, a_task_context, a_task_id);

        if ( true == task_added)
        {
            kernel::internal::task::state::set(
                a_task_context,
                a_context.m_current,
                kernel::task::State::Ready
            );
        }

        return task_added;
    }
    
    inline void setTaskToSuspended(
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

    inline bool setTaskToSleep(
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

        if ( false == task_added)
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

    inline bool setTaskToWaitForObj(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_task_id,
        kernel::Handle *            a_wait_signals,
        uint32_t                    a_number_of_signals,
        bool                        a_wait_for_all_signals,
        bool &                      a_wait_forver,
        Time_ms &                   a_timeout,
        Time_ms &                   a_current
    )
    {
        assert(a_wait_signals);

        bool task_added = wait_list::addTaskWaitObj(
            a_context.m_wait_list,
            a_task_id,
            a_wait_signals,
            a_number_of_signals,
            a_wait_for_all_signals,
            a_wait_forver,
            a_timeout,
            a_current
        );

        if ( false == task_added)
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

    inline void removeTask(
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

    inline bool getNextTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_next_task_id
    )
    {
        bool next_task_found = false;

        for ( uint32_t prio = static_cast<uint32_t>( kernel::task::Priority::High);
            prio < kernel::internal::task::priorities_count;
            ++prio)
        {
            kernel::task::Priority priority = static_cast< kernel::task::Priority>( prio);

            next_task_found = ready_list::findNextTask(
                a_context.m_ready_list,
                priority,
                a_next_task_id
            );

            if ( next_task_found)
            {
                a_context.m_next = a_next_task_id;
                break;
            }
        }

        if ( next_task_found)
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

    inline bool getCurrentTask(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        task::Id &                  a_next_task_id
    )
    {
        bool next_task_found = false;

        for ( uint32_t prio = static_cast< uint32_t>( kernel::task::Priority::High);
            prio < kernel::internal::task::priorities_count;
            ++prio)
        {
            kernel::task::Priority priority = static_cast< kernel::task::Priority>( prio);

            next_task_found = ready_list::findCurrentTask(
                a_context.m_ready_list,
                priority,
                a_next_task_id
            );

            if ( next_task_found)
            {
                a_context.m_next = a_next_task_id;
                break;
            }
        }

        if ( next_task_found)
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

    inline task::Id getCurrentTaskId( Context & a_context)
    {
        return a_context.m_current;
    }

    // Note: I don't like this super-function, but iterating over array should
    //       not be obfuscated by too many interfaces. Previous implementation
    //       included passing lambda with multitude of arguments ignoring 
    //       top-down architecture principle.

    //       Now Wait list data is accessed directly skipping Wait list interface
    //       in between.
    inline void checkWaitConditions(
        Context &                   a_context,
        internal::task::Context &   a_task_context,
        internal::timer::Context &  a_timer_context,
        internal::event::Context &  a_event_context,
        internal::queue::Context &  a_queue_context,
        Time_ms &                   a_current
    )
    {
        // Iterate over WaitItem which hold conditions used to wake up
        // waiting task.
        for ( uint32_t i = 0U; i < kernel::internal::task::max_number; ++i)
        {
            auto & current_wait_item = a_context.m_wait_list.m_list;

            if ( true == current_wait_item.isAllocated( i))
            {
                uint32_t signaled_item_index = 0U;

                kernel::sync::WaitResult a_wait_result;
                auto & conditions = current_wait_item.at( i).m_conditions;

                bool is_condition_fulfilled =
                    wait::check(
                        conditions,
                        a_timer_context,
                        a_event_context,
                        a_queue_context,
                        a_wait_result,
                        a_current,
                        signaled_item_index
                    );

                if ( true == is_condition_fulfilled)
                {
                    task::Id ready_task = current_wait_item.at( i).m_id;

                    bool task_added = addReadyTask(
                        a_context,
                        a_task_context,
                        ready_task
                    );

                    if ( task_added)
                    {
                        kernel::internal::task::state::set(
                            a_task_context,
                            ready_task,
                            kernel::task::State::Ready
                        );

                        // Store results in internal::task context
                        // for specific task.
                        task::wait::result::set(
                            a_task_context,
                            ready_task,
                            a_wait_result
                        );

                        task::wait::last_signal_index::set(
                            a_task_context,
                            ready_task,
                            signaled_item_index
                        );

                        current_wait_item.free( i);
                    }
                    else
                    {
                        assert( true);
                    }
                }
            }
        }
    }
}
