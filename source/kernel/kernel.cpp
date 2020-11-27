#include <kernel.hpp>

#include <hardware.hpp>
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
        internal::init();
    }
    
    void start()
    {
        if (kernel::internal::m_context.started)
        {
            return;
        }

        internal::lockScheduler();
        internal::m_context.started = true;
        
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
            internal::task::Id currentTask;
            internal::scheduler::getCurrentTask(internal::m_context.m_scheduler, currentTask);

            const kernel::task::Priority currentTaskPrio = internal::task::priority::get(
                internal::m_context.m_tasks,
                currentTask
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

            bool task_added = false;

            if (a_create_suspended)
            {
                task_added = internal::scheduler::addSuspendedTask(
                    internal::m_context.m_scheduler,
                    internal::m_context.m_tasks,
                    a_priority,
                    created_task_id
                );
            }
            else
            {
                task_added = internal::scheduler::addReadyTask(
                    internal::m_context.m_scheduler,
                    internal::m_context.m_tasks,
                    a_priority,
                    created_task_id
                );
            }

                if (false == task_added)
                {
                    kernel::internal::task::destroy( internal::m_context.m_tasks, created_task_id);
                    internal::unlockScheduler();
                    return false;
                }
            

            if (a_handle)
            {
                *a_handle = internal::handle::create( internal::handle::ObjectType::Task, created_task_id.m_id);
            }

            if (kernel::internal::m_context.started && (a_priority < currentTaskPrio))
            {
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
            internal::task::Id currentTask;
            internal::scheduler::getCurrentTask(internal::m_context.m_scheduler, currentTask);

            new_handle = internal::handle::create(
                internal::handle::ObjectType::Task,
                currentTask.m_id
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

        if (!internal::m_context.started)
        {
            return;
        }

        const auto suspended_task_id = internal::handle::getId<internal::task::Id>(a_handle);

        internal::lockScheduler();
        {
            // Remove suspended task from scheduler.
            internal::scheduler::setTaskToSuspended(
                internal::m_context.m_scheduler,
                internal::m_context.m_tasks,
                suspended_task_id
            );

            // Reschedule in case task is suspending itself.
            internal::task::Id currentTask;
            internal::scheduler::getCurrentTask(internal::m_context.m_scheduler, currentTask);

            if (currentTask.m_id == suspended_task_id.m_id)
            {
                hardware::syscall(hardware::SyscallId::ExecuteContextSwitch);
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

        if (!internal::m_context.started)
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

            // Resume task in scheduler
            bool task_resumed = internal::scheduler::setTaskToReady(
                internal::m_context.m_scheduler,
                internal::m_context.m_tasks,
                resumedTaskPrio,
                resumedTaskId
            );

            if (false == task_resumed)
            {
                internal::unlockScheduler();
                return;
            }

            internal::task::Id currentTask;
            internal::scheduler::getCurrentTask(internal::m_context.m_scheduler, currentTask);

            const kernel::task::Priority currentTaskPrio = internal::task::priority::get(
                internal::m_context.m_tasks,
                currentTask
            );

            // If resumed task is higher priority than current, issue context switch
            if (resumedTaskPrio < currentTaskPrio)
            {
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
    WaitResult waitForSingleObject(
        kernel::Handle &    a_handle,
        bool                a_wait_forver,
        Time_ms             a_timeout
    )
    {
        // Get current task ID
        // set task to waiting
        return WaitResult::Abandon;
    }
}
