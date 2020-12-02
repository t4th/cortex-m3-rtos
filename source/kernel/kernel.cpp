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
        return internal::system_timer::get( internal::m_context.m_systemTimer);
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
            internal::scheduler::getCurrentTaskId(internal::m_context.m_scheduler, currentTask);

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
            internal::scheduler::getCurrentTaskId(internal::m_context.m_scheduler, currentTask);

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
            internal::scheduler::getCurrentTaskId(internal::m_context.m_scheduler, currentTask);

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
                resumedTaskId
            );

            if (false == task_resumed)
            {
                internal::unlockScheduler();
                return;
            }

            internal::task::Id currentTask;
            internal::scheduler::getCurrentTaskId(internal::m_context.m_scheduler, currentTask);

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

    // TODO: current round-robin timestamp is 1ms. In case a_time = 1ms,
    //       scheduling might make no sense.
    void Sleep(Time_ms a_time)
    {
        if (0U == a_time)
        {
            return;
        }

        internal::lockScheduler();
        {
            internal::task::Id currentTask;
            internal::scheduler::getCurrentTaskId(internal::m_context.m_scheduler, currentTask);

            internal::task::wait::Conditions & wait_condtitions = internal::task::wait::getRef(
                internal::m_context.m_tasks,
                currentTask
            );

            wait_condtitions.m_type = internal::task::wait::Conditions::Type::Sleep;
            wait_condtitions.m_interval = a_time;
            wait_condtitions.m_start = kernel::getTime();

            internal::scheduler::setTaskToWait(
                internal::m_context.m_scheduler,
                internal::m_context.m_tasks,
                currentTask
            );
        }
        hardware::syscall( hardware::SyscallId::ExecuteContextSwitch);
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
        WaitResult result = WaitResult::Abandon;

        internal::lockScheduler();
        {
            internal::task::Id currentTask;
            internal::scheduler::getCurrentTaskId(internal::m_context.m_scheduler, currentTask);

            internal::task::wait::Conditions & wait_condtitions = internal::task::wait::getRef(
                internal::m_context.m_tasks,
                currentTask
            );

            wait_condtitions.m_inputSignals.freeAll();

            wait_condtitions.m_type = internal::task::wait::Conditions::Type::WaitForObj;
            wait_condtitions.m_waitForver = a_wait_forver;
            wait_condtitions.m_interval = a_timeout;
            wait_condtitions.m_start = kernel::getTime();

            uint32_t new_item;
            if (false == wait_condtitions.m_inputSignals.allocate(new_item))
            {
                hardware::debug::setBreakpoint();
            }

            wait_condtitions.m_inputSignals.at(new_item).m_data = a_handle.m_data;

            internal::scheduler::setTaskToWait(
                internal::m_context.m_scheduler,
                internal::m_context.m_tasks,
                currentTask
            );
        }
        hardware::syscall( hardware::SyscallId::ExecuteContextSwitch);

        internal::lockScheduler();
        {
            internal::task::Id currentTask;
            internal::scheduler::getCurrentTaskId(internal::m_context.m_scheduler, currentTask);

            internal::task::wait::Conditions & wait_condtitions = internal::task::wait::getRef(
                internal::m_context.m_tasks,
                currentTask
            );

            result = wait_condtitions.m_result;
        }
        internal::unlockScheduler();

        return result;
    }
}

namespace kernel::critical_section
{
    // todo: consider memory barrier

    bool init(Context & a_context, uint32_t a_spinLock)
    {
        internal::lockScheduler();
        {
            bool initialized = event::create(a_context.m_event);
            if (false == initialized)
            {
                internal::unlockScheduler();
                return false;
            }

            internal::task::Id currentTask;
            internal::scheduler::getCurrentTaskId(internal::m_context.m_scheduler, currentTask);

            a_context.m_ownerTask = internal::handle::create(internal::handle::ObjectType::Task, currentTask.m_id);
            a_context.m_lockCount = 0U;
            a_context.m_spinLock = a_spinLock;
        }
        internal::unlockScheduler();

        return true;
    }

    void deinit(Context & a_context)
    {
        internal::lockScheduler();
        {
            event::destroy(a_context.m_event);
        }
        internal::unlockScheduler();
    }

    void enter(Context & a_context)
    {
        for (uint32_t i = 0U; i < a_context.m_spinLock; ++i)
        {
            if (0U == a_context.m_lockCount)
            {
                ++a_context.m_lockCount;
                break;
            }
        }

        sync::waitForSingleObject(a_context.m_event);
    }

    void leave(Context & a_context)
    {
        internal::lockScheduler();
        {
            --a_context.m_lockCount;
        }
        internal::unlockScheduler();
    }
}
