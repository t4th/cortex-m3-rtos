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
        bool m_started = false;

        // Lock used to stop kernel from round-robin context switches.
        // 0 - context switch unlocked; !0 - context switch locked
        volatile std::atomic<uint32_t> m_schedule_lock = 0U;
}

namespace kernel
{
    inline void storeContext( kernel::internal::task::Id a_task);
    inline void loadContext( kernel::internal::task::Id a_task);

    inline void lockScheduler();
    inline void unlockScheduler();

    void task_routine();
    void idle_routine( void * a_parameter);
    void terminateTask( kernel::internal::task::Id a_id);
}

namespace kernel
{
    void init()
    {
        if (kernel::context::m_started)
        {
            return;
        }

        hardware::init();
        
        {
            internal::task::Id idle_task_handle;

            bool idle_task_created = kernel::task::create(
                idle_routine,
                task::Priority::Idle
            );

            if (false == idle_task_created)
            {
                hardware::debug::setBreakpoint();
            }
        }
    }
    
    void start()
    {
        if (kernel::context::m_started)
        {
            return;
        }

        lockScheduler();
        context::m_started = true;
        
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
                    created_task_id
                );
            }
            else
            {
                task_added = internal::scheduler::addReadyTask(
                    context::m_scheduler,
                    context::m_tasks,
                    created_task_id
                );
            }

            if (false == task_added)
            {
                kernel::internal::task::destroy(
                    context::m_tasks,
                    created_task_id
                );
                unlockScheduler();
                return false;
            }

            internal::task::Id currentTask = 
                internal::scheduler::getCurrentTaskId(context::m_scheduler);

            const kernel::task::Priority currentTaskPrio = internal::task::priority::get(
                context::m_tasks,
                currentTask
            );

            if (a_handle)
            {
                *a_handle = internal::handle::create(
                    internal::handle::ObjectType::Task,
                    created_task_id
                );
            }

            // If priority of task just created is higher than current task, issue context switch.
            if (kernel::context::m_started && (a_priority < currentTaskPrio))
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
            internal::task::Id currentTask =
                internal::scheduler::getCurrentTaskId(context::m_scheduler);

            new_handle = internal::handle::create(
                internal::handle::ObjectType::Task,
                currentTask
            );
        }
        unlockScheduler();

        return new_handle;
    }

    void terminate( kernel::Handle & a_handle)
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

    void suspend( kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Task != internal::handle::getObjectType(a_handle))
        {
            return;
        }

        if (!context::m_started)
        {
            return;
        }

        lockScheduler();
        {
            auto suspended_task_id = internal::handle::getId<internal::task::Id>(a_handle);

            internal::scheduler::setTaskToSuspended(
                context::m_scheduler,
                context::m_tasks,
                suspended_task_id
            );

            // Reschedule in case task is suspending itself.
            internal::task::Id currentTask = 
                internal::scheduler::getCurrentTaskId(context::m_scheduler);

            if (currentTask == suspended_task_id)
            {
                hardware::syscall(hardware::SyscallId::ExecuteContextSwitch);
            }
            else
            {
                unlockScheduler();
            }
        }
    }

    void resume( kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Task != internal::handle::getObjectType(a_handle))
        {
            return;
        }

        if (!context::m_started)
        {
            return;
        }

        lockScheduler();
        {
            auto resumedTaskId = internal::handle::getId<internal::task::Id>(a_handle);

            kernel::task::Priority resumedTaskPrio = internal::task::priority::get(
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

            internal::task::Id currentTask =
                internal::scheduler::getCurrentTaskId(context::m_scheduler);

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

    void sleep( Time_ms a_time)
    {
        // Note: Skip sleeping if provided time is smaller or equal
        //       to single context switch interval.
        if (a_time <= kernel::internal::system_timer::CONTEXT_SWITCH_INTERVAL_MS)
        {
            return;
        }

        lockScheduler();
        {
            internal::task::Id currentTask = 
                internal::scheduler::getCurrentTaskId(context::m_scheduler);

            Time_ms currentTime = internal::system_timer::get( context::m_systemTimer);

            internal::scheduler::setTaskToSleep(
                context::m_scheduler,
                context::m_tasks,
                currentTask,
                a_time,
                currentTime
            );
        }
        hardware::syscall( hardware::SyscallId::ExecuteContextSwitch);
    }
}

namespace kernel::timer
{
    bool create(
        kernel::Handle &    a_handle,
        Time_ms             a_interval
    )
    {
        kernel::lockScheduler();
        {
            Time_ms currentTime = internal::system_timer::get( context::m_systemTimer);

            kernel::internal::timer::Id new_timer_id;

            bool timer_created = internal::timer::create(
                context::m_timers,
                new_timer_id,
                currentTime,
                a_interval
            );

            if (false == timer_created)
            {
                kernel::unlockScheduler();
                return false;
            }

            a_handle = internal::handle::create(
                internal::handle::ObjectType::Timer,
                new_timer_id
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
                new_event_id
            );
        }
        kernel::unlockScheduler();

        return true;
    }
    void destroy( kernel::Handle & a_handle)
    {
        const internal::handle::ObjectType objectType = internal::handle::getObjectType(a_handle);

        if (internal::handle::ObjectType::Event != objectType)
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
        const internal::handle::ObjectType objectType = internal::handle::getObjectType(a_handle);

        if (internal::handle::ObjectType::Event != objectType)
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
        const internal::handle::ObjectType objectType = internal::handle::getObjectType(a_handle);

        if (internal::handle::ObjectType::Event != objectType)
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
        internal::task::Id currentTask;
        WaitResult result = WaitResult::Abandon;

        lockScheduler();
        {
            // TODO: create spin lock to test below some times.

            // Test a_handle condition first.
            {
                bool condition_fulfilled = false;
                const auto objectType = internal::handle::getObjectType(a_handle);

                switch (objectType)
                {
                case internal::handle::ObjectType::Event:
                {
                    auto event_id = internal::handle::getId<internal::event::Id>(a_handle);
                    auto evState = internal::event::getState(context::m_events, event_id);
                    if (internal::event::State::Set == evState)
                    {
                        condition_fulfilled = true;
                    }
                    break;
                }
                case internal::handle::ObjectType::Timer:
                {
                    auto timer_id = internal::handle::getId<internal::timer::Id>(a_handle);
                    auto timerState = internal::timer::getState(context::m_timers, timer_id);
                    if (internal::timer::State::Finished == timerState)
                    {
                        condition_fulfilled = true;
                    }
                    break;
                }
                default:
                {
                    // Note: currently not implemented.
                    unlockScheduler();
                    return result;
                }
                };

                if(true == condition_fulfilled)
                {
                    result = kernel::sync::WaitResult::ObjectSet;
                    unlockScheduler();
                    return result;
                }
            }

            // Set task to Wait state for object pointed by a_handle.
            {
                currentTask = internal::scheduler::getCurrentTaskId(context::m_scheduler);
                Time_ms currentTime = internal::system_timer::get( context::m_systemTimer);

                bool operation_result = internal::scheduler::setTaskToWaitForObj(
                    context::m_scheduler,
                    context::m_tasks,
                    currentTask,
                    a_handle,
                    a_wait_forver,
                    a_timeout,
                    currentTime
                );

                if (false == operation_result)
                {
                    hardware::debug::setBreakpoint();
                }
            }
        }
        hardware::syscall( hardware::SyscallId::ExecuteContextSwitch);

        lockScheduler();
        {
            result = internal::task::wait::get(context::m_tasks, currentTask);
        }
        unlockScheduler();

        return result;
    }
}

namespace kernel::critical_section
{
    // todo: consider memory barrier

    bool init( Context & a_context, uint32_t a_spinLock)
    {
        lockScheduler();
        {
            bool initialized = event::create(a_context.m_event);
            if (false == initialized)
            {
                unlockScheduler();
                return false;
            }

            internal::task::Id currentTask =
                internal::scheduler::getCurrentTaskId(context::m_scheduler);

            a_context.m_ownerTask = internal::handle::create(
                internal::handle::ObjectType::Task,
                currentTask
            );
            a_context.m_lockCount = 0U;
            a_context.m_spinLock = a_spinLock;
        }
        unlockScheduler();

        return true;
    }

    void deinit( Context & a_context)
    {
        lockScheduler();
        {
            event::destroy(a_context.m_event);
        }
        unlockScheduler();
    }

    void enter( Context & a_context)
    {
        // Test critical state condition m_spinLock times, before
        // creating any system object and context switching.
        for (volatile uint32_t i = 0U; i < a_context.m_spinLock; ++i)
        {
            if (0U == a_context.m_lockCount)
            {
                ++a_context.m_lockCount;
                break;
            }
        }

        sync::waitForSingleObject(a_context.m_event);
    }

    void leave( Context & a_context)
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

        volatile kernel::hardware::task::Context * current_task = internal::task::context::get(context::m_tasks, a_task);
        kernel::hardware::context::current::set(current_task);
    }

    // Must only be called from handler mode (MSP stack) since it is modifying psp.
    void loadContext(kernel::internal::task::Id a_task)
    {
        volatile kernel::hardware::task::Context * next_task = internal::task::context::get(context::m_tasks, a_task);
        kernel::hardware::context::next::set(next_task);

        const uint32_t next_sp = kernel::internal::task::sp::get(context::m_tasks, a_task);
        kernel::hardware::sp::set(next_sp);
    }

    // TODO: Lock/unlock need more elegant implementation.
    //       Most likely each kernel task ended with some kind of
    //       context switch would have its own SVC call.
    void lockScheduler()
    {
        ++context::m_schedule_lock;
    }

    void unlockScheduler()
    {
        --context::m_schedule_lock;
    }

    void terminateTask( kernel::internal::task::Id a_id)
    {
        lockScheduler();
        {
            // Reschedule in case task is killing itself.
            internal::task::Id currentTask =
                internal::scheduler::getCurrentTaskId( context::m_scheduler);

            internal::scheduler::removeTask(
                context::m_scheduler,
                context::m_tasks,
                a_id
            );

            internal::task::destroy( context::m_tasks, a_id);

            if (currentTask == a_id)
            {
                if (true == context::m_started)
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

    // Task routine wrapper used by kernel.
    void task_routine()
    {
        internal::task::Id currentTask;
        kernel::task::Routine routine;
        void * parameter;

        lockScheduler();
        {
            currentTask = internal::scheduler::getCurrentTaskId( context::m_scheduler);
            routine = internal::task::routine::get( context::m_tasks, currentTask);
            parameter = internal::task::parameter::get( context::m_tasks, currentTask);
        }
        unlockScheduler();

        routine(parameter); // Call the actual task routine.

        terminateTask(currentTask); // Cleanup task data.
    }

    __attribute__((weak)) void idle_routine( void * a_parameter)
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
        internal::task::Id currentTask = scheduler::getCurrentTaskId(context::m_scheduler);
        internal::task::Id nextTask;

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
        Time_ms current_time = system_timer::get(context::m_systemTimer);

        timer::tick( context::m_timers, current_time);

        // TODO: if task of priority higher than currently running
        //       has woken up - reschedule everything.
        scheduler::checkWaitConditions(
            context::m_scheduler,
            context::m_tasks,
            context::m_timers,
            context::m_events,
            current_time
        );

        // If lock is enabled, increment time, but delay scheduler.
        if (0U == context::m_schedule_lock)
        {
            // Calculate Round-Robin time stamp
            bool interval_elapsed = system_timer::isIntervalElapsed(
                context::m_systemTimer
            );

            if (interval_elapsed)
            {
                internal::task::Id currentTask =
                    scheduler::getCurrentTaskId(context::m_scheduler);

                // Find next task in priority group.
                internal::task::Id nextTask;
                bool task_found = scheduler::getNextTask(
                    context::m_scheduler,
                    context::m_tasks,
                    nextTask
                );

                if(task_found)
                {
                    // TODO: move this check to scheduler
                    //       and integrate result to getNextTask
                    //       return value.
                    if (currentTask != nextTask)
                    {
                        storeContext(currentTask);
                        loadContext(nextTask);

                        execute_context_switch = true;
                    }
                }
            }
        }

        system_timer::increment( context::m_systemTimer);

        return execute_context_switch;
    }
}