#include <kernel.hpp>

#include <internal.hpp>

namespace kernel
{
    void init()
    {
        if (kernel::internal::m_context.started)
        {
            return;
        }

        hardware::init();
        
        internal::m_context.old_time = 0U;
        internal::m_context.time = 0U;
        internal::m_context.schedule_lock = 0U;

        internal::task::Id idle_task_handle;

        // Idle task is always available as system task.
        // todo: check if kernel::task::create can be used instead of internal::task::create
        internal::task::create(
            internal::m_context.m_tasks,
            internal::task_routine,
            internal::idle_routine,
            task::Priority::Idle,
            &idle_task_handle
        );
        
        internal::scheduler::addTask(internal::m_context.m_scheduler, task::Priority::Idle, idle_task_handle);
        
        internal::m_context.m_current = idle_task_handle;
        internal::m_context.m_next = idle_task_handle;
    }
    
    void start()
    {
        if (kernel::internal::m_context.started)
        {
            return;
        }

        internal::lockScheduler();
        internal::m_context.started = true;

        internal::scheduler::findHighestPrioTask(internal::m_context.m_scheduler, internal::m_context.m_next);
        
        hardware::start();
        hardware::syscall(hardware::SyscallId::LoadNextTask);
    }

    Time_ms getTime()
    {
        return internal::m_context.time;
    }
}

namespace kernel::task
{
    bool create(
        kernel::task::Routine   a_routine,
        kernel::task::Priority  a_priority,
        kernel::Handle *        a_handle,
        void *                  a_parameter,
        bool                    a_create_suspended
    )
    {
        internal::lockScheduler();
        {
            const kernel::task::Priority currentTaskPrio = internal::task::priority::get(
                internal::m_context.m_tasks,
                internal::m_context.m_current
            );

            kernel::internal::task::Id created_task_id;

            bool task_created = kernel::internal::task::create(
                internal::m_context.m_tasks,
                internal::task_routine,
                a_routine, a_priority,
                &created_task_id,
                a_parameter,
                a_create_suspended
            );
        
            if (false == task_created)
            {
                internal::unlockScheduler();
                return false;
            }

            if (false == a_create_suspended)
            {
                bool task_added = kernel::internal::scheduler::addTask(
                    kernel::internal::m_context.m_scheduler,
                    a_priority,
                    created_task_id
                );

                if (false == task_added)
                {
                    kernel::internal::task::destroy( internal::m_context.m_tasks, created_task_id);
                    internal::unlockScheduler();
                    return false;
                }
            }

            if (a_handle)
            {
                *a_handle = internal::handle::create( internal::handle::ObjectType::Task, created_task_id.m_id);
            }

            if (kernel::internal::m_context.started && (a_priority < currentTaskPrio))
            {
                kernel::internal::task::state::set(
                    internal::m_context.m_tasks,
                    internal::m_context.m_current,
                    kernel::task::State::Ready
                );

                kernel::internal::scheduler::findHighestPrioTask(
                    kernel::internal::m_context.m_scheduler,
                    kernel::internal::m_context.m_next
                );

                hardware::syscall( hardware::SyscallId::ExecuteContextSwitch);
            }
            else
            {
                internal::unlockScheduler();
            }
        }

        return true;
    }

    kernel::Handle getCurrent()
    {
        kernel::Handle new_handle{};

        internal::lockScheduler();
        {
            new_handle = internal::handle::create(
                internal::handle::ObjectType::Task,
                internal::m_context.m_current.m_id
            );
        }
        internal::unlockScheduler();

        return new_handle;
    }

    void terminate(kernel::Handle & a_handle)
    {
        switch(internal::handle::getObjectType(a_handle))
        {
        case internal::handle::ObjectType::Task:
        {
            auto terminated_task_id = internal::handle::getId<internal::task::Id>(a_handle);
            internal::terminateTask(terminated_task_id);
            break;
        }
        default:
            break;
        }
    }

    void suspend(kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Task != internal::handle::getObjectType(a_handle))
        {
            return;
        }

        const auto suspended_task_id = internal::handle::getId<internal::task::Id>(a_handle);

        internal::lockScheduler();
        {
            kernel::task::Priority prio = internal::task::priority::get(
                internal::m_context.m_tasks,
                suspended_task_id
            );

            // Remove suspended task from scheduler.
            internal::scheduler::removeTask(internal::m_context.m_scheduler, prio, suspended_task_id);

            kernel::internal::task::state::set(
                internal::m_context.m_tasks,
                suspended_task_id,
                kernel::task::State::Suspended
            );

            // Reschedule in case task is suspending itself.
            if (kernel::internal::m_context.m_current.m_id == suspended_task_id.m_id)
            {
                kernel::internal::scheduler::findHighestPrioTask(
                    kernel::internal::m_context.m_scheduler,
                    kernel::internal::m_context.m_next
                );

                if (kernel::internal::m_context.started)
                {
                    hardware::syscall(hardware::SyscallId::ExecuteContextSwitch);
                }
                else
                {
                    internal::unlockScheduler();
                }
            }
            else
            {
                internal::unlockScheduler();
            }
        }
    }

    void resume(kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Task != internal::handle::getObjectType(a_handle))
        {
            return;
        }

        internal::lockScheduler();
        {
            const auto resumedTaskId = internal::handle::getId<internal::task::Id>(a_handle);

            const kernel::task::Priority resumedTaskPrio = internal::task::priority::get(
                internal::m_context.m_tasks,
                resumedTaskId
            );

            // Add resumed task back to scheduler.
            if (false == internal::scheduler::addTask(
                internal::m_context.m_scheduler, resumedTaskPrio, resumedTaskId))
            {
                internal::unlockScheduler();
                return;
            }

            const kernel::task::Priority currentTaskPrio = internal::task::priority::get(
                internal::m_context.m_tasks,
                internal::m_context.m_current
            );

            // If resumed task is higher priority than current, issue context switch
            if (kernel::internal::m_context.started && (resumedTaskPrio < currentTaskPrio))
            {
                kernel::internal::task::state::set(
                    internal::m_context.m_tasks,
                    internal::m_context.m_current,
                    kernel::task::State::Ready
                );

                // todo: this might be changed since resumed task ID is known and stored in
                //       resumedTaskId by this point
                kernel::internal::scheduler::findHighestPrioTask(
                    kernel::internal::m_context.m_scheduler,
                    kernel::internal::m_context.m_next
                );

                hardware::syscall( hardware::SyscallId::ExecuteContextSwitch);
            }
            else
            {
                internal::unlockScheduler();
            }
        }
    }

    void Sleep(Time_ms a_time)
    {
        // get current task ID
        // create timer
        // set task to waiting
    }
}

namespace kernel::timer
{
    bool create(
        kernel::Handle &    a_handle,
        Time_ms             a_interval,
        kernel::Handle *    a_signal
    )
    {
        kernel::internal::lockScheduler();
        {
            kernel::internal::timer::Id new_timer_id;

            bool timer_created = internal::timer::create(
                internal::m_context.m_timers,
                new_timer_id,
                a_interval,
                a_signal
            );

            if (false == timer_created)
            {
                kernel::internal::unlockScheduler();
                return false;
            }

            a_handle = internal::handle::create(
                internal::handle::ObjectType::Timer,
                new_timer_id.m_id
            );
        }
        kernel::internal::unlockScheduler();

        return true;
    }

    void destroy( kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Timer == internal::handle::getObjectType(a_handle))
        {
            return;
        }

        kernel::internal::lockScheduler();
        {
            auto timer_id = internal::handle::getId<internal::timer::Id>(a_handle);
            internal::timer::destroy(internal::m_context.m_timers, timer_id);
        }
        kernel::internal::unlockScheduler();
    }

    void start( kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Timer != internal::handle::getObjectType(a_handle))
        {
            return;
        }

        kernel::internal::lockScheduler();
        {
            auto timer_id = internal::handle::getId<internal::timer::Id>(a_handle);
            internal::timer::start(internal::m_context.m_timers, timer_id);
        }
        kernel::internal::unlockScheduler();
    }

    void stop( kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Timer != internal::handle::getObjectType(a_handle))
        {
            return;
        }

        kernel::internal::lockScheduler();
        {
            auto timer_id = internal::handle::getId<internal::timer::Id>(a_handle);
            internal::timer::stop(internal::m_context.m_timers, timer_id);
        }
        kernel::internal::unlockScheduler();
    }
}

namespace kernel::event
{
    bool create( kernel::Handle & a_handle, bool a_manual_reset)
    {
        kernel::internal::lockScheduler();
        {
            kernel::internal::event::Id new_event_id;
            bool event_created = internal::event::create(
                internal::m_context.m_events,
                new_event_id,
                a_manual_reset
            );

            if (false == event_created)
            {
                kernel::internal::unlockScheduler();
                return false;
            }

            a_handle = internal::handle::create(
                internal::handle::ObjectType::Event,
                new_event_id.m_id
            );
        }
        kernel::internal::unlockScheduler();

        return true;
    }
    void destroy( kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Event != internal::handle::getObjectType(a_handle))
        {
            return;
        }

        kernel::internal::lockScheduler();
        {
            auto event_id = internal::handle::getId<internal::event::Id>(a_handle);
            internal::event::destroy(internal::m_context.m_events, event_id);
        }
        kernel::internal::unlockScheduler();
    }

    void set( kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Event != internal::handle::getObjectType(a_handle))
        {
            return;
        }

        kernel::internal::lockScheduler();
        {
            auto event_id = internal::handle::getId<internal::event::Id>(a_handle);
            internal::event::set(internal::m_context.m_events, event_id);
        }
        kernel::internal::unlockScheduler();
    }

    void reset( kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Event != internal::handle::getObjectType(a_handle))
        {
            return;
        }

        kernel::internal::lockScheduler();
        {
            auto event_id = internal::handle::getId<internal::event::Id>(a_handle);
            internal::event::reset(internal::m_context.m_events, event_id);
        }
        kernel::internal::unlockScheduler();
    }
}

namespace kernel::sync
{
    bool waitForSingleObject(
        kernel::Handle &    a_handle,
        bool                a_wait_forver,
        Time_ms             a_timeout
    )
    {
        // Get current task ID
        // set task to waiting
        return false;
    }
}
