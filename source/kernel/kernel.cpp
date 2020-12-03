#include <kernel.hpp>

#include <hardware.hpp>

#include <scheduler.hpp>
#include <handle.hpp>
#include <timer.hpp>
#include <event.hpp>
#include <system_timer.hpp>

namespace kernel::context
{
        internal::system_timer::Context m_systemTimer;
        internal::task::Context         m_tasks;
        internal::scheduler::Context    m_scheduler;
        internal::timer::Context        m_timers;
        internal::event::Context        m_events;

        // Indicate if kernel is started. It is mostly used to detected
        // if system object was created before or after kernel::start.
        bool started = false;

        // Lock used to stop kernel from round-robin context switches.
        // 0 - context switch unlocked; !0 - context switch locked
        volatile uint32_t schedule_lock = 0U;
}

namespace kernel
{
    void storeContext(kernel::internal::task::Id a_task);
    void loadContext(kernel::internal::task::Id a_task);

    void lockScheduler();
    void unlockScheduler();
    void terminateTask(kernel::internal::task::Id a_id);
    void task_routine();
    void idle_routine(void * a_parameter);
}

namespace kernel
{
    void init()
    {
        if (kernel::context::started)
        {
            return;
        }

        hardware::init();
        
        {
            internal::task::Id idle_task_handle;

            // Idle task is always available as system task.
            // todo: check if kernel::task::create can be
            // used instead of internal::task::create
            internal::task::create(
                context::m_tasks,
                task_routine,
                idle_routine,
                task::Priority::Idle,
                &idle_task_handle
            );

            internal::scheduler::addReadyTask(
                context::m_scheduler,
                context::m_tasks,
                kernel::task::Priority::Idle,
                idle_task_handle
            );
        }
    }
    
    void start()
    {
        if (kernel::context::started)
        {
            return;
        }

        lockScheduler();
        context::started = true;
        
        hardware::start();

        hardware::syscall(hardware::SyscallId::LoadNextTask);
    }

    Time_ms getTime()
    {
        return internal::system_timer::get( context::m_systemTimer);
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
        lockScheduler();
        {
            internal::task::Id currentTask;
            internal::scheduler::getCurrentTaskId(context::m_scheduler, currentTask);

            const kernel::task::Priority currentTaskPrio = internal::task::priority::get(
                context::m_tasks,
                currentTask
            );

            kernel::internal::task::Id created_task_id;

            bool task_created = kernel::internal::task::create(
                context::m_tasks,
                task_routine,
                a_routine, a_priority,
                &created_task_id,
                a_parameter,
                a_create_suspended
            );
        
            if (false == task_created)
            {
                unlockScheduler();
                return false;
            }

            bool task_added = false;

            if (a_create_suspended)
            {
                task_added = internal::scheduler::addSuspendedTask(
                    context::m_scheduler,
                    context::m_tasks,
                    a_priority,
                    created_task_id
                );
            }
            else
            {
                task_added = internal::scheduler::addReadyTask(
                    context::m_scheduler,
                    context::m_tasks,
                    a_priority,
                    created_task_id
                );
            }

            if (false == task_added)
            {
                kernel::internal::task::destroy( context::m_tasks, created_task_id);
                unlockScheduler();
                return false;
            }
            

            if (a_handle)
            {
                *a_handle = internal::handle::create( internal::handle::ObjectType::Task, created_task_id.m_id);
            }

            if (kernel::context::started && (a_priority < currentTaskPrio))
            {
                hardware::syscall( hardware::SyscallId::ExecuteContextSwitch);
            }
            else
            {
                unlockScheduler();
            }
        }

        return true;
    }

    kernel::Handle getCurrent()
    {
        kernel::Handle new_handle{};

        lockScheduler();
        {
            internal::task::Id currentTask;
            internal::scheduler::getCurrentTaskId(context::m_scheduler, currentTask);

            new_handle = internal::handle::create(
                internal::handle::ObjectType::Task,
                currentTask.m_id
            );
        }
        unlockScheduler();

        return new_handle;
    }

    void terminate(kernel::Handle & a_handle)
    {
        switch(internal::handle::getObjectType(a_handle))
        {
        case internal::handle::ObjectType::Task:
        {
            auto terminated_task_id = internal::handle::getId<internal::task::Id>(a_handle);
            terminateTask(terminated_task_id);
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

        if (!context::started)
        {
            return;
        }

        const auto suspended_task_id = internal::handle::getId<internal::task::Id>(a_handle);

        lockScheduler();
        {
            // Remove suspended task from scheduler.
            internal::scheduler::setTaskToSuspended(
                context::m_scheduler,
                context::m_tasks,
                suspended_task_id
            );

            // Reschedule in case task is suspending itself.
            internal::task::Id currentTask;
            internal::scheduler::getCurrentTaskId(context::m_scheduler, currentTask);

            if (currentTask.m_id == suspended_task_id.m_id)
            {
                hardware::syscall(hardware::SyscallId::ExecuteContextSwitch);
            }
            else
            {
                unlockScheduler();
            }
        }
    }

    void resume(kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Task != internal::handle::getObjectType(a_handle))
        {
            return;
        }

        if (!context::started)
        {
            return;
        }

        lockScheduler();
        {
            const auto resumedTaskId = internal::handle::getId<internal::task::Id>(a_handle);

            const kernel::task::Priority resumedTaskPrio = internal::task::priority::get(
                context::m_tasks,
                resumedTaskId
            );

            // Resume task in scheduler
            bool task_resumed = internal::scheduler::setTaskToReady(
                context::m_scheduler,
                context::m_tasks,
                resumedTaskId
            );

            if (false == task_resumed)
            {
                unlockScheduler();
                return;
            }

            internal::task::Id currentTask;
            internal::scheduler::getCurrentTaskId(context::m_scheduler, currentTask);

            const kernel::task::Priority currentTaskPrio = internal::task::priority::get(
                context::m_tasks,
                currentTask
            );

            // If resumed task is higher priority than current, issue context switch
            if (resumedTaskPrio < currentTaskPrio)
            {
                hardware::syscall( hardware::SyscallId::ExecuteContextSwitch);
            }
            else
            {
                unlockScheduler();
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

        lockScheduler();
        {
            internal::task::Id currentTask;
            internal::scheduler::getCurrentTaskId(context::m_scheduler, currentTask);

            internal::task::wait::Conditions & wait_condtitions = internal::task::wait::getRef(
                context::m_tasks,
                currentTask
            );

            wait_condtitions.m_type = internal::task::wait::Conditions::Type::Sleep;
            wait_condtitions.m_interval = a_time;
            wait_condtitions.m_start = kernel::getTime();

            internal::scheduler::setTaskToWait(
                context::m_scheduler,
                context::m_tasks,
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
        kernel::lockScheduler();
        {
            kernel::internal::timer::Id new_timer_id;

            bool timer_created = internal::timer::create(
                context::m_timers,
                new_timer_id,
                a_interval,
                a_signal
            );

            if (false == timer_created)
            {
                kernel::unlockScheduler();
                return false;
            }

            a_handle = internal::handle::create(
                internal::handle::ObjectType::Timer,
                new_timer_id.m_id
            );
        }
        kernel::unlockScheduler();

        return true;
    }

    void destroy( kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Timer == internal::handle::getObjectType(a_handle))
        {
            return;
        }

        kernel::lockScheduler();
        {
            auto timer_id = internal::handle::getId<internal::timer::Id>(a_handle);
            internal::timer::destroy(context::m_timers, timer_id);
        }
        kernel::unlockScheduler();
    }

    void start( kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Timer != internal::handle::getObjectType(a_handle))
        {
            return;
        }

        kernel::lockScheduler();
        {
            auto timer_id = internal::handle::getId<internal::timer::Id>(a_handle);
            internal::timer::start(context::m_timers, timer_id);
        }
        kernel::unlockScheduler();
    }

    void stop( kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Timer != internal::handle::getObjectType(a_handle))
        {
            return;
        }

        kernel::lockScheduler();
        {
            auto timer_id = internal::handle::getId<internal::timer::Id>(a_handle);
            internal::timer::stop(context::m_timers, timer_id);
        }
        kernel::unlockScheduler();
    }
}

namespace kernel::event
{
    bool create( kernel::Handle & a_handle, bool a_manual_reset)
    {
        kernel::lockScheduler();
        {
            kernel::internal::event::Id new_event_id;
            bool event_created = internal::event::create(
                context::m_events,
                new_event_id,
                a_manual_reset
            );

            if (false == event_created)
            {
                kernel::unlockScheduler();
                return false;
            }

            a_handle = internal::handle::create(
                internal::handle::ObjectType::Event,
                new_event_id.m_id
            );
        }
        kernel::unlockScheduler();

        return true;
    }
    void destroy( kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Event != internal::handle::getObjectType(a_handle))
        {
            return;
        }

        kernel::lockScheduler();
        {
            auto event_id = internal::handle::getId<internal::event::Id>(a_handle);
            internal::event::destroy(context::m_events, event_id);
        }
        kernel::unlockScheduler();
    }

    void set( kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Event != internal::handle::getObjectType(a_handle))
        {
            return;
        }

        kernel::lockScheduler();
        {
            auto event_id = internal::handle::getId<internal::event::Id>(a_handle);
            internal::event::set(context::m_events, event_id);
        }
        kernel::unlockScheduler();
    }

    void reset( kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Event != internal::handle::getObjectType(a_handle))
        {
            return;
        }

        kernel::lockScheduler();
        {
            auto event_id = internal::handle::getId<internal::event::Id>(a_handle);
            internal::event::reset(context::m_events, event_id);
        }
        kernel::unlockScheduler();
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

        lockScheduler();
        {
            internal::task::Id currentTask;
            internal::scheduler::getCurrentTaskId(context::m_scheduler, currentTask);

            internal::task::wait::Conditions & wait_condtitions = internal::task::wait::getRef(
                context::m_tasks,
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
                context::m_scheduler,
                context::m_tasks,
                currentTask
            );
        }
        hardware::syscall( hardware::SyscallId::ExecuteContextSwitch);

        lockScheduler();
        {
            internal::task::Id currentTask;
            internal::scheduler::getCurrentTaskId(context::m_scheduler, currentTask);

            internal::task::wait::Conditions & wait_condtitions = internal::task::wait::getRef(
                context::m_tasks,
                currentTask
            );

            result = wait_condtitions.m_result;
        }
        unlockScheduler();

        return result;
    }
}

namespace kernel::critical_section
{
    // todo: consider memory barrier

    bool init(Context & a_context, uint32_t a_spinLock)
    {
        lockScheduler();
        {
            bool initialized = event::create(a_context.m_event);
            if (false == initialized)
            {
                unlockScheduler();
                return false;
            }

            internal::task::Id currentTask;
            internal::scheduler::getCurrentTaskId(context::m_scheduler, currentTask);

            a_context.m_ownerTask = internal::handle::create(internal::handle::ObjectType::Task, currentTask.m_id);
            a_context.m_lockCount = 0U;
            a_context.m_spinLock = a_spinLock;
        }
        unlockScheduler();

        return true;
    }

    void deinit(Context & a_context)
    {
        lockScheduler();
        {
            event::destroy(a_context.m_event);
        }
        unlockScheduler();
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
        lockScheduler();
        {
            --a_context.m_lockCount;
        }
        unlockScheduler();
    }
}

namespace kernel
{
    // Must only be called from handler mode (MSP stack) since it is modifying psp.
    void storeContext(kernel::internal::task::Id a_task)
    {
        const uint32_t sp = hardware::sp::get();
        internal::task::sp::set( context::m_tasks, a_task, sp);

        kernel::hardware::task::Context * current_task = internal::task::context::get(context::m_tasks, a_task);
        kernel::hardware::context::current::set(current_task);
    }

    // Must only be called from handler mode (MSP stack) since it is modifying psp.
    void loadContext(kernel::internal::task::Id a_task)
    {
        kernel::hardware::task::Context * next_task = internal::task::context::get(context::m_tasks, a_task);
        kernel::hardware::context::next::set(next_task);

        const uint32_t next_sp = kernel::internal::task::sp::get(context::m_tasks, a_task);
        kernel::hardware::sp::set(next_sp);
    }

    // TODO: Lock/unlock need more elegant implementation.
    //       Most likely each kernel task ended with some kind of
    //       context switch would have its own SVC call.
    void lockScheduler()
    {
        ++context::schedule_lock;
    }

    void unlockScheduler()
    {
        // TODO: analyze DSB and DMB here
        // TODO: thinker about removing it.
        --context::schedule_lock;
    }

    void terminateTask(kernel::internal::task::Id a_id)
    {
        lockScheduler();
        {
            // Reschedule in case task is killing itself.
            internal::task::Id currentTask;
            internal::scheduler::getCurrentTaskId( context::m_scheduler, currentTask);

            internal::scheduler::removeTask(
                context::m_scheduler,
                context::m_tasks,
                a_id
            );

            internal::task::destroy( context::m_tasks, a_id);

            if (currentTask.m_id == a_id.m_id)
            {
                if (true == context::started)
                {
                    hardware::syscall(hardware::SyscallId::LoadNextTask);
                }
                else
                {
                    unlockScheduler();
                }
            }
            else
            {
                unlockScheduler();
            }
        }
    }

    // User task routine wrapper used by kernel.
    void task_routine()
    {
        kernel::task::Routine routine{};
        void * parameter{};
        internal::task::Id currentTask;

        lockScheduler();
        {
            internal::scheduler::getCurrentTaskId( context::m_scheduler, currentTask);
            routine = internal::task::routine::get( context::m_tasks, currentTask);
            parameter = internal::task::parameter::get( context::m_tasks, currentTask);
        }
        unlockScheduler();

        routine(parameter); // Call the actual task routine.

        terminateTask(currentTask); // Cleanup task data.
    }

    void idle_routine(void * a_parameter)
    {
        volatile int i = 0;
        while(1)
        {
            kernel::hardware::debug::print("idle\r\n");
            for (i = 0; i < 100000; i++);
            // TODO: calculate CPU load
        }
    }
}

namespace kernel::internal
{
    void loadNextTask()
    {
        internal::task::Id nextTask;

        bool task_available = scheduler::getCurrentTask(
            context::m_scheduler,
            context::m_tasks,
            nextTask
        );

        assert(true == task_available);

        loadContext(nextTask);

        unlockScheduler();
    }

    void switchContext()
    {
        internal::task::Id currentTask;
        internal::task::Id nextTask;

        scheduler::getCurrentTaskId(
            context::m_scheduler,
            currentTask
        );

        bool task_available = scheduler::getCurrentTask(
            context::m_scheduler,
            context::m_tasks,
            nextTask
        );

        assert(true == task_available);

        storeContext(currentTask);
        loadContext(nextTask);

        unlockScheduler();
    }

    bool tick()
    {
        bool execute_context_switch = false;

        timer::tick( context::m_timers);
        scheduler::checkWaitConditions(
            context::m_scheduler,
            context::m_tasks,
            context::m_timers,
            context::m_events
        );

        // If lock is enabled, increment time, but delay scheduler.
        if (0U == context::schedule_lock)
        {
            // Calculate Round-Robin time stamp
            bool interval_elapsed = system_timer::isIntervalElapsed(
                context::m_systemTimer
            );

            if (interval_elapsed)
            {
                internal::task::Id currentTask;
                scheduler::getCurrentTaskId(
                    context::m_scheduler,
                    currentTask
                );

                // Find next task in priority group.
                internal::task::Id nextTask;
                bool task_found = scheduler::getNextTask(
                    context::m_scheduler,
                    context::m_tasks,
                    nextTask
                );

                if(task_found)
                {
                    storeContext(currentTask);
                    loadContext(nextTask);

                    execute_context_switch = true;
                }
            }
        }

        system_timer::increment( context::m_systemTimer);

        return execute_context_switch;
    }
}