#include <kernel.hpp>

#include <hardware.hpp>

#include <system_timer.hpp>
#include <task.hpp>
#include <scheduler.hpp>
#include <timer.hpp>
#include <event.hpp>
#include <lock.hpp>

namespace kernel::internal::context
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

namespace kernel::internal
{
    void task_routine();
    void idle_routine( void * a_parameter);
    void terminateTask( task::Id a_id);
}

namespace kernel
{
    void init()
    {
        if ( true == internal::context::m_started)
        {
            return;
        }

        hardware::init();
        
        {
            bool idle_task_created = task::create(
                internal::idle_routine,
                task::Priority::Idle
            );

            if ( false == idle_task_created)
            {
                assert( true);
            }
        }
    }
    
    void start()
    {
        if ( true == internal::context::m_started)
        {
            return;
        }

        internal::lock::enter( internal::context::m_lock);
        internal::context::m_started = true;
        
        hardware::start();

        hardware::syscall( hardware::SyscallId::LoadNextTask);
    }

    Time_ms getTime()
    {
        return internal::system_timer::get( internal::context::m_systemTimer);
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
        internal::lock::enter( internal::context::m_lock);
        {
            kernel::internal::task::Id created_task_id;

            bool task_created = internal::task::create(
                internal::context::m_tasks,
                internal::task_routine,
                a_routine, a_priority,
                &created_task_id,
                a_parameter,
                a_create_suspended
            );
        
            if ( false == task_created)
            {
                internal::lock::leave( internal::context::m_lock);
                return false;
            }

            bool task_added = false;

            if ( a_create_suspended)
            {
                task_added = internal::scheduler::addSuspendedTask(
                    internal::context::m_scheduler,
                    internal::context::m_tasks,
                    created_task_id
                );
            }
            else
            {
                task_added = internal::scheduler::addReadyTask(
                    internal::context::m_scheduler,
                    internal::context::m_tasks,
                    created_task_id
                );
            }

            if ( false == task_added)
            {
                internal::task::destroy(
                    internal::context::m_tasks,
                    created_task_id
                );

                kernel::internal::lock::leave(
                    internal::context::m_lock
                );

                return false;
            }

            auto current_task_id = 
                internal::scheduler::getCurrentTaskId( internal::context::m_scheduler);

            const auto current_task_priority = internal::task::priority::get(
                internal::context::m_tasks,
                current_task_id
            );

            if ( a_handle)
            {
                *a_handle = internal::handle::create(
                    internal::handle::ObjectType::Task,
                    created_task_id
                );
            }

            // If priority of task just created is higher than current task, issue context switch.
            if ( internal::context::m_started && (a_priority < current_task_priority))
            {
                hardware::syscall( hardware::SyscallId::ExecuteContextSwitch);
            }
            else
            {
                internal::lock::leave( internal::context::m_lock);
            }
        }

        return true;
    }

    kernel::Handle getCurrent()
    {
        Handle new_handle;

        internal::lock::enter( internal::context::m_lock);
        {
            internal::task::Id created_task_id =
                internal::scheduler::getCurrentTaskId( internal::context::m_scheduler);

            new_handle = internal::handle::create(
                internal::handle::ObjectType::Task,
                created_task_id
            );
        }
        internal::lock::leave( internal::context::m_lock);

        return new_handle;
    }

    void terminate( kernel::Handle & a_handle)
    {
        const auto objectType = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Task != objectType)
        {
            return;
        }

        auto terminated_task_id = internal::handle::getId<internal::task::Id>(a_handle);
        internal::terminateTask( terminated_task_id);
    }

    void suspend( kernel::Handle & a_handle)
    {
        const auto objectType = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Task != objectType)
        {
            return;
        }

        if ( false == internal::context::m_started)
        {
            return;
        }

        internal::lock::enter( internal::context::m_lock);
        {
            auto suspended_task_id = internal::handle::getId< internal::task::Id>( a_handle);

            internal::scheduler::setTaskToSuspended(
                internal::context::m_scheduler,
                internal::context::m_tasks,
                suspended_task_id
            );

            // Reschedule in case task is suspending itself.
            const auto current_task_id = internal::scheduler::getCurrentTaskId( internal::context::m_scheduler);

            if ( current_task_id == suspended_task_id)
            {
                hardware::syscall( hardware::SyscallId::ExecuteContextSwitch);
            }
            else
            {
                internal::lock::leave( internal::context::m_lock);
            }
        }
    }

    // Resume suspended task.
    // Expected behaviour:
    // - if task is trying to resume itself - do nothing
    // - if task is not suspended - do nothing.
    void resume( kernel::Handle & a_handle)
    {
        const auto objectType = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Task != objectType)
        {
            return;
        }

        if ( false == internal::context::m_started)
        {
            return;
        }

        kernel::internal::lock::enter( internal::context::m_lock);
        {
            auto resumedTaskId = internal::handle::getId< internal::task::Id>( a_handle);
            auto currentTaskId = internal::scheduler::getCurrentTaskId( internal::context::m_scheduler);

            if (resumedTaskId == currentTaskId)
            {
                internal::lock::leave( internal::context::m_lock);
                return;
            }

            bool task_resumed = internal::scheduler::resumeSuspendedTask(
                internal::context::m_scheduler,
                internal::context::m_tasks,
                resumedTaskId
            );

            if (false == task_resumed)
            {
                internal::lock::leave( internal::context::m_lock);
                return;
            }

            const auto currentTaskPrio = internal::task::priority::get(
                internal::context::m_tasks,
                currentTaskId
            );

            const auto resumedTaskPrio = internal::task::priority::get(
                internal::context::m_tasks,
                resumedTaskId
            );

            // If resumed task is higher priority than current, issue context switch
            if ( resumedTaskPrio < currentTaskPrio)
            {
                hardware::syscall( hardware::SyscallId::ExecuteContextSwitch);
            }
            else
            {
                internal::lock::leave( internal::context::m_lock);
            }
        }
    }

    void sleep( Time_ms a_time)
    {
        // Note: Skip sleeping if provided time is smaller or equal
        //       to single context switch interval.
        if ( a_time <= internal::system_timer::CONTEXT_SWITCH_INTERVAL_MS)
        {
            return;
        }

        internal::lock::enter( internal::context::m_lock);
        {
            internal::task::Id currentTask = 
                internal::scheduler::getCurrentTaskId( internal::context::m_scheduler);

            Time_ms currentTime = internal::system_timer::get( internal::context::m_systemTimer);

            internal::scheduler::setTaskToSleep(
                internal::context::m_scheduler,
                internal::context::m_tasks,
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
        internal::lock::enter( internal::context::m_lock);
        {
            Time_ms currentTime = internal::system_timer::get( internal::context::m_systemTimer);

            internal::timer::Id new_timer_id;

            bool timer_created = internal::timer::create(
                internal::context::m_timers,
                new_timer_id,
                currentTime,
                a_interval
            );

            if ( false == timer_created)
            {
                internal::lock::leave( internal::context::m_lock);
                return false;
            }

            a_handle = internal::handle::create(
                internal::handle::ObjectType::Timer,
                new_timer_id
            );
        }
        internal::lock::leave( internal::context::m_lock);

        return true;
    }

    void destroy( kernel::Handle & a_handle)
    {
        const auto objectType = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Timer != objectType)

        internal::lock::enter( internal::context::m_lock);
        {
            auto timer_id = internal::handle::getId< internal::timer::Id>( a_handle);
            internal::timer::destroy( internal::context::m_timers, timer_id);
        }
        internal::lock::leave( internal::context::m_lock);
    }

    void start( kernel::Handle & a_handle)
    {
        const auto objectType = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Timer != objectType)
        {
            return;
        }

        internal::lock::enter( internal::context::m_lock);
        {
            auto timer_id = internal::handle::getId< internal::timer::Id>( a_handle);
            internal::timer::start( internal::context::m_timers, timer_id);
        }
        internal::lock::leave( internal::context::m_lock);
    }

    void stop( kernel::Handle & a_handle)
    {
        const auto objectType = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Timer != objectType)
        {
            return;
        }

        internal::lock::enter( internal::context::m_lock);
        {
            auto timer_id = internal::handle::getId< internal::timer::Id>( a_handle);
            internal::timer::stop( internal::context::m_timers, timer_id);
        }
        internal::lock::leave( internal::context::m_lock);
    }
}

namespace kernel::event
{
    bool create( kernel::Handle & a_handle, bool a_manual_reset)
    {
        internal::lock::enter( internal::context::m_lock);
        {
            internal::event::Id new_event_id;
            bool event_created = internal::event::create(
                internal::context::m_events,
                new_event_id,
                a_manual_reset
            );

            if ( false == event_created)
            {
                internal::lock::leave( internal::context::m_lock);
                return false;
            }

            a_handle = internal::handle::create(
                internal::handle::ObjectType::Event,
                new_event_id
            );
        }
        internal::lock::leave( internal::context::m_lock);

        return true;
    }
    void destroy( kernel::Handle & a_handle)
    {
        const auto objectType = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Event != objectType)
        {
            return;
        }

        internal::lock::enter( internal::context::m_lock);
        {
            auto event_id = internal::handle::getId< internal::event::Id>( a_handle);
            internal::event::destroy( internal::context::m_events, event_id);
        }
        internal::lock::leave( internal::context::m_lock);
    }

    void set( kernel::Handle & a_handle)
    {
        const auto objectType = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Event != objectType)
        {
            return;
        }

        internal::lock::enter( internal::context::m_lock);
        {
            auto event_id = internal::handle::getId< internal::event::Id>( a_handle);
            internal::event::set( internal::context::m_events, event_id);
        }
        internal::lock::leave( internal::context::m_lock);
    }

    void setFromInterrupt( kernel::Handle & a_handle)
    {
        const auto objectType = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Event != objectType)
        {
            return;
        }

        auto event_id = internal::handle::getId< internal::event::Id>( a_handle);

        hardware::critical_section::Context cs_context;
        hardware::critical_section::enter(
            cs_context,
            hardware::interrupt::priority::Preemption::Critical
        );
        {
            internal::event::set( internal::context::m_events, event_id);
        }
        hardware::critical_section::leave( cs_context);
    }

    void reset( kernel::Handle & a_handle)
    {
        const auto objectType = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Event != objectType)
        {
            return;
        }

        internal::lock::enter( internal::context::m_lock);
        {
            auto event_id = internal::handle::getId< internal::event::Id>( a_handle);
            internal::event::reset( internal::context::m_events, event_id);
        }
        internal::lock::leave( internal::context::m_lock);
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

        assert( a_number_of_elements >= 1U);

        if ( nullptr == a_array_of_handles)
        {
            return kernel::sync::WaitResult::InvalidHandle;
        }

        // Note: Before creating system object, this function used to check
        //       all wait conditions SpinLock times, but testing it with
        //       test project didn't show any performance boost, so it was 
        //       removed.

        internal::lock::enter( internal::context::m_lock);
        {
            // Set task to Wait state for object pointed by a_handle
            Time_ms currentTime = internal::system_timer::get( internal::context::m_systemTimer);

            auto current_task_id = 
                internal::scheduler::getCurrentTaskId( internal::context::m_scheduler);

            bool operation_result = internal::scheduler::setTaskToWaitForObj(
                internal::context::m_scheduler,
                internal::context::m_tasks,
                current_task_id,
                a_array_of_handles,
                a_number_of_elements,
                a_wait_for_all,
                a_wait_forver,
                a_timeout,
                currentTime
            );

            if ( false == operation_result)
            {
                assert( true);
            }
        }
        hardware::syscall( hardware::SyscallId::ExecuteContextSwitch);

        internal::lock::enter( internal::context::m_lock);
        {
            auto current_task_id = 
                internal::scheduler::getCurrentTaskId( internal::context::m_scheduler);

            result = internal::task::wait::result::get(
                internal::context::m_tasks,
                current_task_id
            );

            if ( a_signaled_item_index)
            {
                *a_signaled_item_index = internal::task::wait::last_signal_index::get(
                    internal::context::m_tasks,
                    current_task_id
                );
            }
        }
        internal::lock::leave( internal::context::m_lock);

        return result;
    }
}

namespace kernel::critical_section
{
    bool init( Context & a_context, uint32_t a_spinLock)
    {
        internal::lock::enter( internal::context::m_lock);
        {
            internal::event::Id new_event_id;

            // Create event used to wake up tasks waiting for
            // critical section.
            bool event_created = internal::event::create(
                internal::context::m_events,
                new_event_id,
                false
            );

            if ( false == event_created)
            {
                internal::lock::leave( internal::context::m_lock);
                return false;
            }

            a_context.m_event = internal::handle::create(
                internal::handle::ObjectType::Event,
                new_event_id
            );

            internal::event::set( internal::context::m_events, new_event_id);

            internal::task::Id currentTask =
                internal::scheduler::getCurrentTaskId( internal::context::m_scheduler);

            a_context.m_ownerTask = internal::handle::create(
                internal::handle::ObjectType::Task,
                currentTask
            );

            a_context.m_lockCount = 0U;
            a_context.m_spinLock = a_spinLock;
        }
        internal::lock::leave( internal::context::m_lock);

        return true;
    }

    void deinit( Context & a_context)
    {
        internal::lock::enter( internal::context::m_lock);
        {
            event::destroy( a_context.m_event);
        }
        internal::lock::leave( internal::context::m_lock);
    }

    void enter( Context & a_context)
    {
        // Test critical state condition m_spinLock times, before
        // creating any system object and context switching.
        volatile uint32_t i = 0U;

        // Note: function is constructed this way, because time between kernel lock/unlock and
        //       waking from waitForSingleObject is big enough for context switches to occur
        //       and modify m_lockCount even if m_event was set. So after waking up task,
        //       m_lockCount test should be done again.
        while ( true)
        {
            internal::lock::enter( internal::context::m_lock);
            {
                if ( 0U == a_context.m_lockCount)
                {
                    ++a_context.m_lockCount;
                    internal::lock::leave( internal::context::m_lock);
                    return;
                }
            }
            internal::lock::leave( internal::context::m_lock);

            if ( i >= a_context.m_spinLock)
            {
                i = 0U;
                sync::WaitResult result = sync::waitForSingleObject( a_context.m_event);

                if ( sync::WaitResult::ObjectSet != result)
                {
                    assert( true);
                }
            }
            else
            {
                ++i;
            }
        }
    }

    void leave( Context & a_context)
    {
        internal::lock::enter( internal::context::m_lock);
        {
            --a_context.m_lockCount;
            if (0U == a_context.m_lockCount)
            {
                auto event_id = internal::handle::getId< internal::event::Id>( a_context.m_event);
                internal::event::set( internal::context::m_events, event_id);
            }
        }
        internal::lock::leave( internal::context::m_lock);
    }
}

namespace kernel::internal
{
    // Remove task from scheduler and internal::task.
    void terminateTask( task::Id a_id)
    {
        internal::lock::enter( internal::context::m_lock);
        {
            const auto current_task =
                internal::scheduler::getCurrentTaskId( internal::context::m_scheduler);

            internal::scheduler::removeTask(
                internal::context::m_scheduler,
                internal::context::m_tasks,
                a_id
            );

            internal::task::destroy( internal::context::m_tasks, a_id);

            // Reschedule in case task is killing itself.
            if ( current_task == a_id)
            {
                if ( true == internal::context::m_started)
                {
                    hardware::syscall( hardware::SyscallId::LoadNextTask);
                }
                else
                {
                    internal::lock::leave( internal::context::m_lock);
                }
            }
            else
            {
                internal::lock::leave( internal::context::m_lock);
            }
        }
    }

    // Task routine wrapper used by kernel.
    void task_routine()
    {
        internal::task::Id current_task;
        kernel::task::Routine routine;
        void * parameter;

        internal::lock::enter( internal::context::m_lock);
        {
            current_task = internal::scheduler::getCurrentTaskId( internal::context::m_scheduler);
            routine = internal::task::routine::get( internal::context::m_tasks, current_task);
            parameter = internal::task::parameter::get( internal::context::m_tasks, current_task);
        }
        internal::lock::leave( internal::context::m_lock);

        routine( parameter); // Call the actual task routine.

        terminateTask( current_task); // Cleanup task data.
    }

    // Default Idle routine. Idle task MUST always be available or UB will happen.
    // TODO: calculate CPU load
    // TODO: consider creating callback instead of 'weak' attribute, where kernel API
    //       functions won't work (Terminate on Idle task is a bad idea - UB).
    __attribute__(( weak)) void idle_routine( void * a_parameter)
    {
        while( true)
        {
            // TODO: consider using hardware::waitForInterrupt();
        }
    }
}

namespace kernel::internal
{
    // This function is bridge between kernel::hardware and kernel::internal implementations.
    // It stores hardware stack pointer in internal::task descriptor and provide memory location
    // where kernel::hardware will store current context.
    // NOTE: Must only be called from handler mode (MSP stack) since it is modifying psp.
    inline void storeContext(
        task::Context &   a_task_context,
        task::Id &        a_task
    )
    {
        const uint32_t sp = hardware::sp::get();
        internal::task::sp::set( a_task_context, a_task, sp);

        auto current_task_context = internal::task::context::get( a_task_context, a_task);
        hardware::context::current::set( current_task_context);
    }

    // This function is bridge between kernel::hardware and kernel::internal implementations.
    // It provide kernel::hardware layer with previously stored task context and stack pointer
    // from kernel::internal::task descriptor.
    // NOTE: Must only be called from handler mode (MSP stack) since it is modifying psp.
    inline void loadContext( 
        task::Context &     a_task_context,
        task::Id &          a_task
    )
    {
        auto next_task_context = task::context::get( a_task_context, a_task);
        hardware::context::next::set( next_task_context);

        const uint32_t next_sp = internal::task::sp::get( a_task_context, a_task);
        hardware::sp::set( next_sp);
    }

    // This is function used by kernel::hardware to load and get next task sp and context. 
    void loadNextTask()
    {
        internal::task::Id next_task;

        bool task_available = scheduler::getCurrentTask(
            context::m_scheduler,
            context::m_tasks,
            next_task
        );

        assert( true == task_available);

        loadContext( context::m_tasks, next_task);

        internal::lock::leave( context::m_lock);

        // TODO: there is corner case when:
        //       - task call syscall, which switch task to another
        //       - after switching to another task, sysTick is called and want to switch
        //       - next task in priority queue.
        //       Consider other corner cases and solve this. Probably round-robin time
        //       should be reset when syscall is used by user.
    }

    // This is function used by kernel::hardware to get information, where to store current
    // context and from where get next context.
    void switchContext()
    {
        task::Id current_task = scheduler::getCurrentTaskId( context::m_scheduler);
        task::Id next_task;

        bool task_available = scheduler::getCurrentTask(
            context::m_scheduler,
            context::m_tasks,
            next_task
        );

        assert( true == task_available);

        storeContext( context::m_tasks, current_task);
        loadContext( context::m_tasks, next_task);

        lock::leave( context::m_lock);

        // TODO: there is corner case when:
        //       - task call syscall, which switch task to another
        //       - after switching to another task, sysTick is called and want to switch
        //       - next task in priority queue.
        //       Consider other corner cases and solve this. Probably round-robin time
        //       should be reset when syscall is used by user.
    }

    bool tick() 
    {
        bool execute_context_switch = false;

        // If lock is enabled, increment time, but delay scheduler.
        if ( lock::isLocked( context::m_lock))
        {
            Time_ms current_time = system_timer::get( context::m_systemTimer);

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

            if ( interval_elapsed)
            {
                task::Id current_task =
                    scheduler::getCurrentTaskId( context::m_scheduler);

                // Find next task in priority group.
                internal::task::Id next_task;

                bool task_found = scheduler::getNextTask(
                    context::m_scheduler,
                    context::m_tasks,
                    next_task
                );

                if ( task_found)
                {
                    // TODO: move this check to scheduler
                    //       and integrate result to getNextTask
                    //       return value.
                    if ( current_task != next_task)
                    {
                        storeContext( context::m_tasks, current_task);
                        loadContext( context::m_tasks, next_task);

                        execute_context_switch = true;
                    }
                }
            }
        }

        system_timer::increment( context::m_systemTimer);

        return execute_context_switch;
    }
}