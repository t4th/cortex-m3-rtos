#include "kernel.hpp"

#include "hardware/hardware.hpp"

#include "system_timer/system_timer.hpp"
#include "task/task.hpp"
#include "scheduler/scheduler.hpp"
#include "timer/timer.hpp"
#include "event/event.hpp"
#include "queue/queue.hpp"
#include "lock/lock.hpp"

// Print error in case of wrong kernel API usage.
// Note:  I considered error codes or GetLastError() like function,
//        but it would bloat the code.
// Note2: Must only be used to describe error behaviour.
namespace kernel::error
{
    inline void print( const char * a_string)
    {
        if constexpr ( debug_messages_enable)
        {
            hardware::debug::print( a_string);
        }
    }
}

// Context of the entire kernel.
namespace kernel::internal::context
{
    internal::system_timer::Context m_systemTimer;
    internal::task::Context         m_tasks;
    internal::scheduler::Context    m_scheduler;
    internal::timer::Context        m_timers;
    internal::event::Context        m_events;
    internal::queue::Context        m_queue;
    internal::lock::Context         m_lock;

    // Indicate if kernel has been started. It is used to detect if
    // system object was created before or after kernel::start and also
    // for some sanity checks.
    bool m_started = false;
}

// Declarations of internal kernel functions.
namespace kernel::internal
{
    void taskRoutine();
    void idleTaskRoutine( void * a_parameter);
    void terminateTask( task::Id a_id);
}

// User API implementations.
namespace kernel
{
    void init()
    {
        if ( true == internal::context::m_started)
        {
            error::print( "Kernel already started!\n");
            return;
        }

        internal::hardware::init();
        
        bool idle_task_created = task::create( internal::idleTaskRoutine, task::Priority::Idle);

        if ( false == idle_task_created)
        {
            error::print( "Critical Error! Failed to create Idle task!\n");
            hardware::debug::setBreakpoint();
            assert( true);
        }
    }
    
    void start()
    {
        if ( true == internal::context::m_started)
        {
            error::print( "Kernel already started!\n");
            return;
        }

        // This lock is cleared in context switch procedure after syscall.
        internal::lock::enter( internal::context::m_lock);
        {
            internal::context::m_started = true;

            internal::hardware::start();
        }
        internal::hardware::syscall( internal::hardware::SyscallId::LoadNextTask);
    }

    TimeMs getTime()
    {
        return internal::system_timer::get( internal::context::m_systemTimer);
    }
    
    uint32_t getCoreFrequencyHz()
    {
        return kernel::internal::hardware::core_clock_freq_hz;
    }
}

namespace kernel::task
{
    bool create(
        kernel::task::Routine   a_routine,
        kernel::task::Priority  a_priority,
        kernel::Handle * const  a_handle,
        void * const            a_parameter,
        bool                    a_create_suspended
    )
    {
        internal::lock::enter( internal::context::m_lock);
        {
            kernel::internal::task::Id created_task_id;

            bool task_created = internal::task::create(
                internal::context::m_tasks,
                internal::taskRoutine,
                a_routine, a_priority,
                &created_task_id,
                a_parameter,
                a_create_suspended
            );
        
            if ( false == task_created)
            {
                error::print( "Failed to internally create task!\n");
                internal::lock::leave( internal::context::m_lock);
                return false;
            }

            bool task_added;

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
                internal::task::destroy( internal::context::m_tasks, created_task_id);
                error::print( "Failed adding task to scheduler!\n");
                kernel::internal::lock::leave( internal::context::m_lock);
                return false;
            }

            auto current_task_id = internal::scheduler::getCurrentTaskId( internal::context::m_scheduler);

            const auto current_task_priority = internal::task::priority::get(
                internal::context::m_tasks,
                current_task_id
            );

            if ( a_handle)
            {
                *a_handle = internal::handle::create( internal::handle::ObjectType::Task, created_task_id);
            }

            // If kernel is started, priority of a task just created is higher than the current task and
            // created task is not suspended - issue a context switch.
            if ( (internal::context::m_started) && ( a_priority < current_task_priority) && ( false == a_create_suspended))
            {
                internal::hardware::syscall( internal::hardware::SyscallId::ExecuteContextSwitch);
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
            auto current_task_id = internal::scheduler::getCurrentTaskId( internal::context::m_scheduler);

            new_handle = internal::handle::create( internal::handle::ObjectType::Task, current_task_id);
        }
        internal::lock::leave( internal::context::m_lock);

        return new_handle;
    }

    void terminate( kernel::Handle & a_handle)
    {
        const auto object_type = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Task != object_type)
        {
            error::print( "Invalid handle! Underlying object type is not supported by this function.\n");
            return;
        }

        auto terminated_task_id = internal::handle::getId< internal::task::Id>( a_handle);
        internal::terminateTask( terminated_task_id);
    }

    void suspend( kernel::Handle & a_handle)
    {
        const auto object_type = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Task != object_type)
        {
            error::print( "Invalid handle! Underlying object type is not supported by this function.\n");
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
                internal::hardware::syscall( internal::hardware::SyscallId::ExecuteContextSwitch);
            }
            else
            {
                internal::lock::leave( internal::context::m_lock);
            }
        }
    }

    void resume( kernel::Handle & a_handle)
    {
        const auto object_type = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Task != object_type)
        {
            error::print( "Invalid handle! Underlying object type is not supported by this function.\n");
            return;
        }

        if ( false == internal::context::m_started)
        {
            return;
        }

        kernel::internal::lock::enter( internal::context::m_lock);
        {
            auto resumed_task_id = internal::handle::getId< internal::task::Id>( a_handle);
            auto current_task_id = internal::scheduler::getCurrentTaskId( internal::context::m_scheduler);

            // If task is trying to resume itself - do nothing.
            if ( resumed_task_id == current_task_id)
            {
                internal::lock::leave( internal::context::m_lock);
                return;
            }

            bool task_resumed = internal::scheduler::resumeSuspendedTask(
                internal::context::m_scheduler,
                internal::context::m_tasks,
                resumed_task_id
            );

            // If task is not suspended - do nothing.
            if ( false == task_resumed)
            {
                internal::lock::leave( internal::context::m_lock);
                return;
            }

            // If resumed task is higher priority than current, issue a context switch.
            const auto current_task_priority = internal::task::priority::get(
                internal::context::m_tasks,
                current_task_id
            );

            const auto resumed_task_priority = internal::task::priority::get(
                internal::context::m_tasks,
                resumed_task_id
            );

            if ( resumed_task_priority < current_task_priority)
            {
                internal::hardware::syscall( internal::hardware::SyscallId::ExecuteContextSwitch);
            }
            else
            {
                internal::lock::leave( internal::context::m_lock);
            }
        }
    }

    void sleep( TimeMs a_time)
    {
        // Note: Spin lock in case if provided time is smaller or equal to a single context switch interval.
        if ( a_time <= internal::system_timer::context_switch_interval_ms)
        {
            for ( volatile TimeMs delay = 0U; delay < a_time; ++delay);
            return;
        }

        internal::lock::enter( internal::context::m_lock);
        {
            auto current_task_id = internal::scheduler::getCurrentTaskId( internal::context::m_scheduler);

            TimeMs current_time = internal::system_timer::get( internal::context::m_systemTimer);

            internal::scheduler::setTaskToSleep(
                internal::context::m_scheduler,
                internal::context::m_tasks,
                current_task_id,
                a_time,
                current_time
            );
        }
        internal::hardware::syscall( internal::hardware::SyscallId::ExecuteContextSwitch);
    }
}

namespace kernel::timer
{
    bool create( kernel::Handle & a_handle, TimeMs a_interval)
    {
        internal::lock::enter( internal::context::m_lock);
        {
            TimeMs current_time = internal::system_timer::get( internal::context::m_systemTimer);

            internal::timer::Id new_timer_id;

            bool timer_created = internal::timer::create(
                internal::context::m_timers,
                new_timer_id,
                current_time,
                a_interval
            );

            if ( false == timer_created)
            {
                error::print( "Failed to internally create timer!\n");
                internal::lock::leave( internal::context::m_lock);
                return false;
            }

            a_handle = internal::handle::create( internal::handle::ObjectType::Timer, new_timer_id);
        }
        internal::lock::leave( internal::context::m_lock);

        return true;
    }

    void destroy( kernel::Handle & a_handle)
    {
        const auto object_type = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Timer != object_type)
        {
            error::print( "Invalid handle! Underlying object type is not supported by this function.\n");
            return;
        }

        internal::lock::enter( internal::context::m_lock);
        {
            auto timer_id = internal::handle::getId< internal::timer::Id>( a_handle);
            internal::timer::destroy( internal::context::m_timers, timer_id);
        }
        internal::lock::leave( internal::context::m_lock);
    }

    void start( kernel::Handle & a_handle)
    {
        const auto object_type = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Timer != object_type)
        {
            error::print( "Invalid handle! Underlying object type is not supported by this function.\n");
            return;
        }

        internal::lock::enter( internal::context::m_lock);
        {
            auto timer_id = internal::handle::getId< internal::timer::Id>( a_handle);
            internal::timer::start( internal::context::m_timers, timer_id);
        }
        internal::lock::leave( internal::context::m_lock);
    }

    void restart( kernel::Handle & a_handle)
    {
        const auto object_type = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Timer != object_type)
        {
            error::print( "Invalid handle! Underlying object type is not supported by this function.\n");
            return;
        }

        internal::lock::enter( internal::context::m_lock);
        {
            auto timer_id = internal::handle::getId< internal::timer::Id>( a_handle);
            internal::timer::restart( internal::context::m_timers, timer_id);
        }
        internal::lock::leave( internal::context::m_lock);
    }

    void stop( kernel::Handle & a_handle)
    {
        const auto object_type = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Timer != object_type)
        {
            error::print( "Invalid handle! Underlying object type is not supported by this function.\n");
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
    // Note: No lock is required since internal::event API is already protected.
    bool create( kernel::Handle & a_handle, bool a_manual_reset, const char * const a_name)
    {
        internal::event::Id new_event_id;

        bool event_created = internal::event::create(
            internal::context::m_events,
            new_event_id,
            a_manual_reset,
            a_name
        );

        if ( false == event_created)
        {
            error::print( "Failed to internally create event!\n");
            return false;
        }

        a_handle = internal::handle::create( internal::handle::ObjectType::Event, new_event_id);

        return true;
    }

    bool open( kernel::Handle & a_handle, const char * const ap_name)
    {
        if ( nullptr == ap_name)
        {
            error::print( "Invalid argument! Empty pointer to event name!\n");
            return false;
        }

        internal::event::Id event_id;

        bool event_opened = internal::event::open( internal::context::m_events, event_id, ap_name);
        
        if ( true == event_opened)
        {
            a_handle = internal::handle::create( internal::handle::ObjectType::Event, event_id);
        }

        return event_opened;
    }

    void destroy( kernel::Handle & a_handle)
    {
        const auto object_type = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Event != object_type)
        {
            error::print( "Invalid handle! Underlying object type is not supported by this function.\n");
            return;
        }

        auto event_id = internal::handle::getId< internal::event::Id>( a_handle);
        internal::event::destroy( internal::context::m_events, event_id);
    }

    void set( kernel::Handle & a_handle)
    {
        const auto object_type = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Event != object_type)
        {
            error::print( "Invalid handle! Underlying object type is not supported by this function.\n");
            return;
        }

        auto event_id = internal::handle::getId< internal::event::Id>( a_handle);
        internal::event::set( internal::context::m_events, event_id);
    }

    void reset( kernel::Handle & a_handle)
    {
        const auto object_type = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Event != object_type)
        {
            error::print( "Invalid handle! Underlying object type is not supported by this function.\n");
            return;
        }

        auto event_id = internal::handle::getId< internal::event::Id>( a_handle);
        internal::event::reset( internal::context::m_events, event_id);
    }
}

namespace kernel::critical_section
{
    bool init( Context & a_context, uint32_t a_spinLock)
    {
        internal::lock::enter( internal::context::m_lock);
        {
            internal::event::Id new_event_id;

            // Create event used to wake up tasks waiting for a critical section.
            bool event_created = internal::event::create(
                internal::context::m_events,
                new_event_id,
                false,
                nullptr
            );

            if ( false == event_created)
            {
                error::print( "Failed to internally create event!\n");
                internal::lock::leave( internal::context::m_lock);
                return false;
            }

            a_context.m_event = internal::handle::create(
                internal::handle::ObjectType::Event,
                new_event_id
            );

            internal::event::set( internal::context::m_events, new_event_id);

            internal::task::Id current_task_id =
                internal::scheduler::getCurrentTaskId( internal::context::m_scheduler);

            a_context.m_ownerTask = internal::handle::create(
                internal::handle::ObjectType::Task,
                current_task_id
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
        volatile uint32_t i = 0U;

        // Note: Function is constructed this way, because time between kernel lock/unlock and
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
            
            // Test critical state condition m_spinLock times, before creating any system object
            // and context switching.
            if ( i >= a_context.m_spinLock)
            {
                i = 0U;
                sync::WaitResult result = sync::waitForSingleObject( a_context.m_event);

                if ( sync::WaitResult::ObjectSet != result)
                {
                    // This is critical error since this function doesn't return a value.
                    // It would most likely happen when critical_section::init was not called.
                    error::print( "Critical Error!\n");
                    hardware::debug::setBreakpoint();
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
            if ( 0U == a_context.m_lockCount)
            {
                auto event_id = internal::handle::getId< internal::event::Id>( a_context.m_event);
                internal::event::set( internal::context::m_events, event_id);
            }
        }
        internal::lock::leave( internal::context::m_lock);
    }
}

namespace kernel::sync
{
    WaitResult waitForSingleObject(
        kernel::Handle &    a_handle,
        bool                a_wait_forver,
        TimeMs              a_timeout
    )
    {
        constexpr uint32_t number_of_elements{ 1U};
        constexpr bool dont_wait_for_all_elements{ false};

        WaitResult result = waitForMultipleObjects(
            &a_handle,
            number_of_elements,
            dont_wait_for_all_elements,
            a_wait_forver,
            a_timeout,
            nullptr
        );

        return result;
    }

    WaitResult waitForMultipleObjects(
        kernel::Handle * const  a_array_of_handles,
        uint32_t                a_number_of_elements,
        bool                    a_wait_for_all,
        bool                    a_wait_forver,
        TimeMs                  a_timeout,
        uint32_t * const        a_signaled_item_index
    )
    {
        if ( nullptr == a_array_of_handles)
        {
            error::print( "Invalid argument! Empty pointer to array of handles!\n");
            return kernel::sync::WaitResult::WaitFailed;
        }

        if ( a_number_of_elements < 1)
        {
            error::print( "Invalid argument! Number of elements should be bigger than 0.\n");
            return kernel::sync::WaitResult::WaitFailed;
        }

        if ( a_number_of_elements >= internal::scheduler::wait::max_input_signals)
        {
            // Can be fixed by increasing number of max_input_signals in config.hpp
            error::print( "Invalid argument! Number of elements is bigger than allocated memory!\n");
            return kernel::sync::WaitResult::WaitFailed;
        }

        // Note: Before creating system object, this function used to check all wait conditions
        //       SpinLock times, but testing it with test project didn't show any performance
        //       boost, so it was removed.

        // TODO: add spin lock checks

        internal::lock::enter( internal::context::m_lock);
        {
            // Set task to Wait state for object pointed by a_handle
            TimeMs current_time = internal::system_timer::get( internal::context::m_systemTimer);

            auto current_task_id = internal::scheduler::getCurrentTaskId( internal::context::m_scheduler);

            bool operation_result = internal::scheduler::setTaskToWaitForObj(
                internal::context::m_scheduler,
                internal::context::m_tasks,
                current_task_id,
                a_array_of_handles,
                a_number_of_elements,
                a_wait_for_all,
                a_wait_forver,
                a_timeout,
                current_time
            );

            if ( false == operation_result)
            {
                // Can be fixed by increasing number of max_input_signals in config.hpp
                error::print( "Critical Error! Failed to internally create Wait Object.\n");
                assert( true);
                internal::lock::leave( internal::context::m_lock);
                return WaitResult::WaitFailed;
            }
        }

        internal::hardware::syscall( internal::hardware::SyscallId::ExecuteContextSwitch);
        
        // After returning from Waiting State, update wait::result and a_signaled_item_index.
        WaitResult result = WaitResult::WaitFailed;

        internal::lock::enter( internal::context::m_lock);
        {
            auto current_task_id = internal::scheduler::getCurrentTaskId( internal::context::m_scheduler);
            result = internal::task::wait::result::get( internal::context::m_tasks, current_task_id);

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

namespace kernel::static_queue
{
    // Note: No lock is required since internal::queue API is already protected.
    bool create(
        kernel::Handle &        a_handle,
        size_t                  a_data_max_size,
        size_t                  a_data_type_size,
        volatile void * const   ap_static_buffer,
        const char * const      ap_name
    )
    {
        if ( 0U == a_data_max_size)
        {
            error::print( "Invalid argument! Buffer size must be bigger than 0.\n");
            return false;
        }

        if ( 0U == a_data_type_size)
        {
            error::print( "Invalid argument! Type size must be bigger than 0.\n");
            return false;
        }

        if ( nullptr == ap_static_buffer)
        {
            error::print( "Invalid argument! Empty pointer to static buffer!\n");
            return false;
        }
        
        kernel::internal::queue::Id created_queue_id;

        bool queue_created = kernel::internal::queue::create(
            kernel::internal::context::m_queue,
            created_queue_id,
            a_data_max_size,
            a_data_type_size,
            ap_static_buffer,
            ap_name
        );

        if ( false == queue_created)
        {
            error::print( "Failed to internally create static queue!\n");
            return false;
        }

        return true;
    }

    bool open( kernel::Handle & a_handle, const char * const ap_name)
    {
        if ( nullptr == ap_name)
        {
            error::print( "Invalid argument! Empty pointer to queue name!\n");
            return false;
        }

        internal::queue::Id queue_id;

        bool queue_opened = internal::queue::open( internal::context::m_queue, queue_id, ap_name);
        
        if ( true == queue_opened)
        {
            a_handle = internal::handle::create( internal::handle::ObjectType::Queue, queue_id);
        }

        return queue_opened;
    }

    void destroy( kernel::Handle & a_handle)
    {
        const auto object_type = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Queue != object_type)
        {
            error::print( "Invalid handle! Underlying object type is not supported by this function.\n");
            return;
        }

        auto queue_id = internal::handle::getId< internal::queue::Id>( a_handle);

        internal::queue::destroy( internal::context::m_queue, queue_id);
    }

    bool send( kernel::Handle & a_handle, volatile void * const ap_data)
    {
        const auto object_type = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Queue != object_type)
        {
            error::print( "Invalid handle! Underlying object type is not supported by this function.\n");
            return false;
        }

        if ( nullptr == ap_data)
        {
            error::print( "Invalid argument! Empty pointer to data!\n");
            return false;
        }

        auto queue_id = internal::handle::getId< internal::queue::Id>( a_handle);

        bool send_result = internal::queue::send(
            internal::context::m_queue,
            queue_id,
            ap_data
        );

        return send_result;
    }

    bool receive( kernel::Handle & a_handle, volatile void * const ap_data)
    {
        const auto object_type = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Queue != object_type)
        {
            error::print( "Invalid handle! Underlying object type is not supported by this function.\n");
            return false;
        }

        if ( nullptr == ap_data)
        {
            error::print( "Invalid argument! Empty pointer to data!\n");
            return false;
        }

        auto queue_id = internal::handle::getId< internal::queue::Id>( a_handle);

        bool receive_result = internal::queue::receive(
            internal::context::m_queue,
            queue_id,
            ap_data
        );

        return receive_result;
    }

    bool size( kernel::Handle & a_handle, size_t & a_size)
    {
        const auto object_type = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Queue != object_type)
        {
            error::print( "Invalid handle! Underlying object type is not supported by this function.\n");
            return false;
        }

        auto queue_id = internal::handle::getId< internal::queue::Id>( a_handle);
            
        a_size = internal::queue::getSize( internal::context::m_queue, queue_id);

        return true;
    }

    bool isFull( kernel::Handle & a_handle, bool & a_is_full)
    {
        const auto object_type = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Queue != object_type)
        {
            error::print( "Invalid handle! Underlying object type is not supported by this function.\n");
            return false;
        }

        auto queue_id = internal::handle::getId< internal::queue::Id>( a_handle);
            
        a_is_full = internal::queue::isFull( internal::context::m_queue, queue_id);

        return true;
    }

    bool isEmpty( kernel::Handle & a_handle, bool & a_is_empty)
    {
        const auto object_type = internal::handle::getObjectType( a_handle);

        if ( internal::handle::ObjectType::Queue != object_type)
        {
            error::print( "Invalid handle! Underlying object type is not supported by this function.\n");
            return false;
        }

        auto queue_id = internal::handle::getId< internal::queue::Id>( a_handle);
            
        a_is_empty = internal::queue::isEmpty( internal::context::m_queue, queue_id);

        return true;
    }
}

namespace kernel::internal
{
    // Remove task from scheduler and internal::task.
    void terminateTask( task::Id a_id)
    {
        internal::lock::enter( context::m_lock);
        {
            const auto current_task = scheduler::getCurrentTaskId( context::m_scheduler);

            scheduler::removeTask( context::m_scheduler, context::m_tasks, a_id);

            internal::task::destroy( context::m_tasks, a_id);

            // Reschedule in case task is killing itself.
            if ( current_task == a_id)
            {
                if ( true == context::m_started)
                {
                    hardware::syscall( hardware::SyscallId::LoadNextTask);
                }
                else
                {
                    internal::lock::leave( context::m_lock);
                }
            }
            else
            {
                internal::lock::leave( context::m_lock);
            }
        }
    }

    // Task routine wrapper used by kernel.
    void taskRoutine()
    {
        internal::task::Id current_task;
        kernel::task::Routine routine;
        void * parameter;

        internal::lock::enter( context::m_lock);
        {
            current_task = internal::scheduler::getCurrentTaskId( context::m_scheduler);
            routine = internal::task::routine::get( context::m_tasks, current_task);
            parameter = internal::task::parameter::get( context::m_tasks, current_task);
        }
        internal::lock::leave( context::m_lock);

        routine( parameter); // Call the actual task routine.

        terminateTask( current_task); // Cleanup task data.
    }

    // Default Idle routine. Idle task MUST always be available or UB will happen.
    // TODO: calculate CPU load
    // TODO: consider creating callback instead of 'weak' attribute, where kernel API
    //       functions won't work (Terminate on Idle task is a bad idea - UB).
    __attribute__(( weak)) void idleTaskRoutine( void * a_parameter)
    {
        while (true)
        {
            // If there is nothing to do, wait for external event (or RTOS tick).
            kernel::hardware::interrupt::wait();
        }
    }
}

namespace kernel::internal
{
    // This function is bridge between kernel::internal::hardware and kernel::internal implementations.
    // It stores hardware stack pointer in internal::task descriptor and provide memory location
    // where kernel::internal::hardware will store current context.
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

    // This function is bridge between kernel::internal::hardware and kernel::internal implementations.
    // It provide kernel::internal::hardware layer with previously stored task context and stack pointer
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

    // This is function used by kernel::internal::hardware to load and get next task sp and context. 
    void loadNextTask()
    {
        internal::task::Id next_task;

        bool task_available = scheduler::getCurrentTask(
            context::m_scheduler,
            context::m_tasks,
            next_task
        );

        if ( false == task_available)
        {
            // Should never happen. Most likely Idle task was not created.
            error::print( "Critical error! No task to switch context to!\n");
            kernel::hardware::debug::setBreakpoint();
        }

        loadContext( context::m_tasks, next_task);

        internal::lock::leave( context::m_lock);

        // TODO: there is corner case when:
        //       - task call syscall, which switch task to another
        //       - after switching to another task, sysTick is called and want to switch
        //       - next task in priority queue.
        //       Consider other corner cases and solve this. Probably round-robin time
        //       should be reset when syscall is used by user.
    }

    // This is function used by kernel::internal::hardware to get information, where to store current
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

        if ( false == task_available)
        {
            // Should never happen. Most likely Idle task was not created.
            error::print( "Critical error! No task to switch context to!\n");
            kernel::hardware::debug::setBreakpoint();
        }

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
            TimeMs current_time = system_timer::get( context::m_systemTimer);

            timer::tick( context::m_timers, current_time);

            // TODO: if task of priority higher than currently running
            //       has woken up - reschedule everything.
            scheduler::checkWaitConditions(
                context::m_scheduler,
                context::m_tasks,
                context::m_timers,
                context::m_events,
                context::m_queue,
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