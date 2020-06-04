#include <kernel.hpp>

#include <hardware.hpp>

#include <task.hpp>
#include <scheduler.hpp>

namespace kernel::internal
{
    constexpr uint32_t DEFAULT_TIMER_INTERVAL = 10;
    
    typedef uint32_t Time_ms;
    
    volatile struct Context
    {
        Time_ms old_time;
        Time_ms time;
        Time_ms interval = DEFAULT_TIMER_INTERVAL;
        
        
        kernel::task::Id m_current;
        kernel::task::Id m_next;

        bool started; // TODO: make status register

        // !0 'tick' will have no effect; 0 - 'tick' works normally
        // This is used as critical section
        volatile uint32_t schedule_lock;
    } m_context;

    // this can be only called from handler mode (MSP stack) since it is modifying psp
    void storeContext(kernel::task::Id a_task)
    {
        const uint32_t sp = hardware::sp::get();
        internal::task::sp::set(a_task, sp);

        kernel::hardware::task::Context * current_task = internal::task::context::get(a_task);
        kernel::hardware::context::current::set(current_task);
    }

    // this can be only called from handler mode (MSP stack) since it is modifying psp
    void loadContext(kernel::task::Id a_task)
    {
        kernel::hardware::task::Context * next_task = internal::task::context::get(a_task);
        kernel::hardware::context::next::set(next_task);

        const uint32_t next_sp = kernel::internal::task::sp::get(a_task);
        kernel::hardware::sp::set(next_sp);
    }

    struct CriticalSection
    {
        CriticalSection()  {kernel::hardware::interrupt::disableAll();}
        ~CriticalSection() {kernel::hardware::interrupt::enableAll();}
    };

    void lockScheduler()
    {
        ++internal::m_context.schedule_lock;
    }

    void unlockScheduler()
    {
        m_context.old_time = m_context.time; // Reset Round-Robin schedule time.
        --internal::m_context.schedule_lock;
    }

    void taskFinished(volatile kernel::task::Id a_id)
    {
        lockScheduler();
        {
            // TODO: cleanup system objects

            scheduler::removeTask(internal::task::priority::get(a_id), a_id);
            internal::task::destroy(a_id);
            
            // TODO: only reschedule if next task prio is higher - check if needed
            // Since task is finished, only load m_next context without storing m_current.
            kernel::scheduler::findHighestPrioTask(kernel::internal::m_context.m_next);
            
#if 1 // TODO: this need to be called from handler mode or MSP stack!
            loadContext(internal::m_context.m_next);
            internal::m_context.m_current = internal::m_context.m_next;
#endif
        }
        unlockScheduler();

        hardware::syscall(hardware::SyscallId::ExecuteContextSwitch);
    }

    void task_routine()
    {
        kernel::task::Routine routine = internal::task::routine::get(m_context.m_current);

        routine(); // Call the actual task routine.

        taskFinished(m_context.m_current);
    }

    void idle_routine()
    {
        while(1)
        {
            // TODO: calculate CPU load
        }
    }
}

namespace kernel
{
    void init()
    {
        hardware::init();
        
        internal::m_context.old_time = 0U;
        internal::m_context.time = 0U;

        task::Id idle_task_handle;
        kernel::internal::task::create(internal::task_routine, internal::idle_routine, task::Priority::Idle, &idle_task_handle);
        
        scheduler::addTask(task::Priority::Idle, idle_task_handle);
        
        internal::m_context.m_current = idle_task_handle;
        internal::m_context.m_next = idle_task_handle;
        internal::loadContext(internal::m_context.m_current);
    }
    
    void start()
    {
        kernel::internal::m_context.started = true;
        kernel::internal::m_context.schedule_lock = false;

        // Reschedule all tasks created before kernel::start
        kernel::scheduler::findHighestPrioTask(kernel::internal::m_context.m_next);
        
        hardware::start();
        
        // system call - start first task
        hardware::syscall(hardware::SyscallId::StartFirstTask); // TODO: can this be moved to hw start?
    }
}

namespace kernel::task
{
    bool create(
        kernel::task::Routine   a_routine,
        kernel::task::Priority  a_priority,
        kernel::task::Id *      a_handle,
        bool                    a_create_suspended
    )
    {
        internal::lockScheduler();
        {
            kernel::task::Id id;

            bool task_created = kernel::internal::task::create(internal::task_routine, a_routine, a_priority, &id, a_create_suspended);
        
            if (false == task_created)
            {
                internal::unlockScheduler();
                return false;
            }

            bool task_added = kernel::scheduler::addTask(a_priority, id);

            if (false == task_added)
            {
                kernel::internal::task::destroy(id);
                internal::unlockScheduler();
                return false;
            }

            if (a_handle)
            {
                *a_handle = id;
            }
        
            const kernel::task::Priority currentTaskPrio = internal::task::priority::get(internal::m_context.m_current);
            if (a_priority > currentTaskPrio)
            {
                kernel::scheduler::findHighestPrioTask(kernel::internal::m_context.m_next);
            }
        }
        internal::unlockScheduler();

        if (kernel::internal::m_context.started)
        {
            hardware::syscall(hardware::SyscallId::ExecuteContextSwitch);
        }

        return true;
    }
}

namespace kernel::internal
{
    // TODO: all data must be in critical section
    //  ...or dont. So far everything is designed assuming kernel m_context is
    // accessed from handler mode only without nesting interrupts (int disabled during handler)
    // - this must be confirmed. Also possible to use designed priority to handle critical section
    bool tick()
    {
        bool execute_context_switch = false;
       
        // If lock is enabled, increment time, but delay scheduler.
        if (0 == m_context.schedule_lock)
        {
            // Calculate Round-Robin time stamp
            // TODO: this looks like shit - make something nicer.
            if (m_context.time - m_context.old_time > m_context.interval)
            {
                m_context.old_time = m_context.time;

                // Get current task priority.
                const kernel::task::Priority current  = internal::task::priority::get(m_context.m_current);
            
                // Find next task in priority group.
                if(true == scheduler::findNextTask(current, m_context.m_next))
                {
                    storeContext(m_context.m_current);
                    loadContext(m_context.m_next);

                    m_context.m_current = m_context.m_next;

                    execute_context_switch = true;
                }
            }
        }

        m_context.time++;
        
        return execute_context_switch;
    }
}
