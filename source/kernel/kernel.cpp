#include <kernel.hpp>

#include <hardware.hpp>

#include <scheduler.hpp>
#include <handle.hpp>
#include <timer.hpp>
#include <event.hpp>
#include <system_timer.hpp>

// Kernel level critical section between thread and handler modes.
namespace kernel::internal::lock
{
    struct Context
    {
        volatile std::atomic<uint32_t> m_interlock;

        Context() : m_interlock{0U} {}
    };

    inline bool isLocked( Context & a_context)
    {
        return (0U == a_context.m_interlock);
    }
    inline void enter( Context & a_context)
    {
        ++a_context.m_interlock;
    }
    inline void leave( Context & a_context)
    {
        --a_context.m_interlock;
    }
}

namespace kernel::context
{
        internal::system_timer::Context m_systemTimer;
        internal::task::Context         m_tasks;
        internal::scheduler::Context    m_scheduler;
        internal::timer::Context        m_timers;
        internal::event::Context        m_events;
        internal::lock::Context         m_lock;

        // Indicate if kernel is started. It is used to detected
        // if system object was created before or after kernel::start.
        bool m_started = false;
}

namespace kernel
{
    inline void storeContext( kernel::internal::task::Id a_task);
    inline void loadContext( kernel::internal::task::Id a_task);

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

        kernel::internal::lock::enter(context::m_lock);
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
        kernel::internal::lock::enter(context::m_lock);
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
                kernel::internal::lock::leave(context::m_lock);
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
                kernel::internal::lock::leave(context::m_lock);
                return false;
            }

            auto current_task_id = 
                internal::scheduler::getCurrentTaskId(context::m_scheduler);

            const auto current_task_priority = internal::task::priority::get(
                context::m_tasks,
                current_task_id
            );

            if (a_handle)
            {
                *a_handle = internal::handle::create(
                    internal::handle::ObjectType::Task,
                    created_task_id
                );
            }

            // If priority of task just created is higher than current task, issue context switch.
            if (kernel::context::m_started && (a_priority < current_task_priority))
            {
                hardware::syscall( hardware::SyscallId::ExecuteContextSwitch);
            }
            else
            {
                kernel::internal::lock::leave(context::m_lock);
            }
        }

        return true;
    }

    kernel::Handle getCurrent()
    {
        kernel::Handle new_handle;

        kernel::internal::lock::enter(context::m_lock);
        {
            internal::task::Id created_task_id =
                internal::scheduler::getCurrentTaskId(context::m_scheduler);

            new_handle = internal::handle::create(
                internal::handle::ObjectType::Task,
                created_task_id
            );
        }
        kernel::internal::lock::leave(context::m_lock);

        return new_handle;
    }

    void terminate( kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Task != internal::handle::getObjectType(a_handle))
        {
            return;
        }

        auto terminated_task_id = internal::handle::getId<internal::task::Id>(a_handle);
        terminateTask(terminated_task_id);
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

        kernel::internal::lock::enter(context::m_lock);
        {
            auto suspended_task_id = internal::handle::getId<internal::task::Id>(a_handle);

            internal::scheduler::setTaskToSuspended(
                context::m_scheduler,
                context::m_tasks,
                suspended_task_id
            );

            // Reschedule in case task is suspending itself.
            const auto current_task_id = internal::scheduler::getCurrentTaskId(context::m_scheduler);

            if (current_task_id == suspended_task_id)
            {
                hardware::syscall(hardware::SyscallId::ExecuteContextSwitch);
            }
            else
            {
                kernel::internal::lock::leave(context::m_lock);
            }
        }
    }

    // Resume suspended task.
    // Expected behaviour:
    // - if task is trying to resume itself - do nothing
    // - if task is not suspended - do nothing.
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

        kernel::internal::lock::enter(context::m_lock);
        {
            auto resumedTaskId = internal::handle::getId<internal::task::Id>(a_handle);
            auto currentTaskId = internal::scheduler::getCurrentTaskId(context::m_scheduler);

            if (resumedTaskId == currentTaskId)
            {
                kernel::internal::lock::leave(context::m_lock);
                return;
            }

            bool task_resumed = internal::scheduler::resumeSuspendedTask(
                context::m_scheduler,
                context::m_tasks,
                resumedTaskId
            );

            if (false == task_resumed)
            {
                kernel::internal::lock::leave(context::m_lock);
                return;
            }

            const auto currentTaskPrio = internal::task::priority::get(
                context::m_tasks,
                currentTaskId
            );

            const auto resumedTaskPrio = internal::task::priority::get(
                context::m_tasks,
                resumedTaskId
            );

            // If resumed task is higher priority than current, issue context switch
            if (resumedTaskPrio < currentTaskPrio)
            {
                hardware::syscall( hardware::SyscallId::ExecuteContextSwitch);
            }
            else
            {
                kernel::internal::lock::leave(context::m_lock);
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

        kernel::internal::lock::enter(context::m_lock);
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
        kernel::internal::lock::enter(context::m_lock);
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
                kernel::internal::lock::leave(context::m_lock);
                return false;
            }

            a_handle = internal::handle::create(
                internal::handle::ObjectType::Timer,
                new_timer_id
            );
        }
        kernel::internal::lock::leave(context::m_lock);

        return true;
    }

    void destroy( kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Timer == internal::handle::getObjectType(a_handle))
        {
            return;
        }

        kernel::internal::lock::enter(context::m_lock);
        {
            auto timer_id = internal::handle::getId<internal::timer::Id>(a_handle);
            internal::timer::destroy(context::m_timers, timer_id);
        }
        kernel::internal::lock::leave(context::m_lock);
    }

    void start( kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Timer != internal::handle::getObjectType(a_handle))
        {
            return;
        }

        kernel::internal::lock::enter(context::m_lock);
        {
            auto timer_id = internal::handle::getId<internal::timer::Id>(a_handle);
            internal::timer::start(context::m_timers, timer_id);
        }
        kernel::internal::lock::leave(context::m_lock);
    }

    void stop( kernel::Handle & a_handle)
    {
        if (internal::handle::ObjectType::Timer != internal::handle::getObjectType(a_handle))
        {
            return;
        }

        kernel::internal::lock::enter(context::m_lock);
        {
            auto timer_id = internal::handle::getId<internal::timer::Id>(a_handle);
            internal::timer::stop(context::m_timers, timer_id);
        }
        kernel::internal::lock::leave(context::m_lock);
    }
}

namespace kernel::event
{
    bool create( kernel::Handle & a_handle, bool a_manual_reset)
    {
        kernel::internal::lock::enter(context::m_lock);
        {
            kernel::internal::event::Id new_event_id;
            bool event_created = internal::event::create(
                context::m_events,
                new_event_id,
                a_manual_reset
            );

            if (false == event_created)
            {
                kernel::internal::lock::leave(context::m_lock);
                return false;
            }

            a_handle = internal::handle::create(
                internal::handle::ObjectType::Event,
                new_event_id
            );
        }
        kernel::internal::lock::leave(context::m_lock);

        return true;
    }
    void destroy( kernel::Handle & a_handle)
    {
        const internal::handle::ObjectType objectType = internal::handle::getObjectType(a_handle);

        if (internal::handle::ObjectType::Event != objectType)
        {
            return;
        }

        kernel::internal::lock::enter(context::m_lock);
        {
            auto event_id = internal::handle::getId<internal::event::Id>(a_handle);
            internal::event::destroy(context::m_events, event_id);
        }
        kernel::internal::lock::leave(context::m_lock);
    }

    void set( kernel::Handle & a_handle)
    {
        const internal::handle::ObjectType objectType = internal::handle::getObjectType(a_handle);

        if (internal::handle::ObjectType::Event != objectType)
        {
            return;
        }

        kernel::internal::lock::enter(context::m_lock);
        {
            auto event_id = internal::handle::getId<internal::event::Id>(a_handle);
            internal::event::set(context::m_events, event_id);
        }
        kernel::internal::lock::leave(context::m_lock);
    }

    void reset( kernel::Handle & a_handle)
    {
        const internal::handle::ObjectType objectType = internal::handle::getObjectType(a_handle);

        if (internal::handle::ObjectType::Event != objectType)
        {
            return;
        }

        kernel::internal::lock::enter(context::m_lock);
        {
            auto event_id = internal::handle::getId<internal::event::Id>(a_handle);
            internal::event::reset(context::m_events, event_id);
        }
        kernel::internal::lock::leave(context::m_lock);
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
        WaitResult result = waitForMultipleObjects(
            &a_handle,
            1U,
            false,
            a_wait_forver,
            a_timeout,
            nullptr
        );

        return result;
    }

    WaitResult waitForMultipleObjects(
        kernel::Handle *    a_array_of_handles,
        uint32_t            a_number_of_elements,
        bool                a_wait_for_all,
        bool                a_wait_forver,
        Time_ms             a_timeout,
        uint32_t *          a_signaled_item_index
    )
    {
        WaitResult result = WaitResult::WaitFailed;

        assert(a_number_of_elements >= 1U);

        if (nullptr == a_array_of_handles)
        {
            return kernel::sync::WaitResult::InvalidHandle;
        }

        // Note: Before creating system object, this function used to check
        //       all wait conditions SpinLock times, but testing it with
        //       test project didn't show any performance boost, so it was 
        //       removed.

        kernel::internal::lock::enter(context::m_lock);
        {
            // Set task to Wait state for object pointed by a_handle
            Time_ms currentTime = internal::system_timer::get( context::m_systemTimer);

            auto current_task_id = 
                internal::scheduler::getCurrentTaskId( context::m_scheduler);

            bool operation_result = internal::scheduler::setTaskToWaitForObj(
                context::m_scheduler,
                context::m_tasks,
                current_task_id,
                a_array_of_handles,
                a_number_of_elements,
                a_wait_for_all,
                a_wait_forver,
                a_timeout,
                currentTime
            );

            if (false == operation_result)
            {
                hardware::debug::setBreakpoint();
            }
        }
        hardware::syscall( hardware::SyscallId::ExecuteContextSwitch);

        kernel::internal::lock::enter(context::m_lock);
        {
            auto current_task_id = 
                internal::scheduler::getCurrentTaskId(context::m_scheduler);

            result = internal::task::wait::result::get(
                context::m_tasks,
                current_task_id
            );

            if (a_signaled_item_index)
            {
                *a_signaled_item_index = internal::task::wait::last_signal_index::get(
                    context::m_tasks,
                    current_task_id
                );
            }
        }
        kernel::internal::lock::leave(context::m_lock);

        return result;
    }
}

namespace kernel::critical_section
{
    bool init( Context & a_context, uint32_t a_spinLock)
    {
        kernel::internal::lock::enter(context::m_lock);
        {
            bool initialized = event::create(a_context.m_event);
            if (false == initialized)
            {
                kernel::internal::lock::leave(context::m_lock);
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
        kernel::internal::lock::leave(context::m_lock);

        return true;
    }

    void deinit( Context & a_context)
    {
        kernel::internal::lock::enter(context::m_lock);
        {
            event::destroy(a_context.m_event);
        }
        kernel::internal::lock::leave(context::m_lock);
    }

    void enter( Context & a_context)
    {
        // Test critical state condition m_spinLock times, before
        // creating any system object and context switching.
        bool is_condition_met = false;

        for (volatile uint32_t i = 0U; i < a_context.m_spinLock; ++i)
        {
            if (0U == a_context.m_lockCount)
            {
                is_condition_met = true;
                break;
            }
        }

        if (true == is_condition_met)
        {
            ++a_context.m_lockCount;
        }
        else
        {
            sync::waitForSingleObject(a_context.m_event);
        }
    }

    void leave( Context & a_context)
    {
        kernel::internal::lock::enter(context::m_lock);
        {
            --a_context.m_lockCount;
        }
        kernel::internal::lock::leave(context::m_lock);
    }
}

namespace kernel
{
    // Must only be called from handler mode (MSP stack) since it is modifying psp.
    void storeContext(kernel::internal::task::Id a_task)
    {
        const uint32_t sp = hardware::sp::get();
        internal::task::sp::set( context::m_tasks, a_task, sp);

        auto current_task_context = internal::task::context::get(context::m_tasks, a_task);
        kernel::hardware::context::current::set(current_task_context);
    }

    // Must only be called from handler mode (MSP stack) since it is modifying psp.
    void loadContext(kernel::internal::task::Id a_task)
    {
        auto next_task_context = internal::task::context::get(context::m_tasks, a_task);
        kernel::hardware::context::next::set(next_task_context);

        const uint32_t next_sp = kernel::internal::task::sp::get(context::m_tasks, a_task);
        kernel::hardware::sp::set(next_sp);
    }

    void terminateTask( kernel::internal::task::Id a_id)
    {
        kernel::internal::lock::enter(context::m_lock);
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
                    kernel::internal::lock::leave(context::m_lock);
                }
            }
            else
            {
                kernel::internal::lock::leave(context::m_lock);
            }
        }
    }

    // Task routine wrapper used by kernel.
    void task_routine()
    {
        internal::task::Id currentTask;
        kernel::task::Routine routine;
        void * parameter;

        kernel::internal::lock::enter(context::m_lock);
        {
            currentTask = internal::scheduler::getCurrentTaskId( context::m_scheduler);
            routine = internal::task::routine::get( context::m_tasks, currentTask);
            parameter = internal::task::parameter::get( context::m_tasks, currentTask);
        }
        kernel::internal::lock::leave(context::m_lock);

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

        kernel::internal::lock::leave(context::m_lock);
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

        kernel::internal::lock::leave(context::m_lock);
    }

    bool tick() 
    {
        bool execute_context_switch = false;

        // If lock is enabled, increment time, but delay scheduler.
        if (kernel::internal::lock::isLocked(context::m_lock))
        {
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