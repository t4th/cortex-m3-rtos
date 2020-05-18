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
    } m_context;

    struct CriticalSection
    {
        CriticalSection()  {kernel::hardware::interrupt::disableAll();}
        ~CriticalSection() {kernel::hardware::interrupt::enableAll();}
    };

    void taskFinished(volatile kernel::task::Id a_id)
    {
        // TODO: cleanup
        // TerminateTask?
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

    void storeContext(kernel::task::Id a_task)
    {
        const uint32_t sp = hardware::sp::get();
        internal::task::sp::set(a_task, sp);

        kernel::hardware::task::Context * current_task = internal::task::context::get(a_task);
        kernel::hardware::context::current::set(current_task);
    }

    void loadContext(kernel::task::Id a_task)
    {
        kernel::hardware::task::Context * next_task = internal::task::context::get(a_task);
        kernel::hardware::context::next::set(next_task);

        const uint32_t next_sp = kernel::internal::task::sp::get(a_task);
        kernel::hardware::sp::set(next_sp );
    }
}

namespace kernel
{
    void init()
    {
        hardware::init();
        
        task::Id idle_task_handle;
        kernel::internal::task::create(internal::idle_routine, task::Priority::Idle, &idle_task_handle);
        
        scheduler::addTask(task::Priority::Idle, idle_task_handle);
        
        internal::m_context.m_current = idle_task_handle;
        internal::m_context.m_next = idle_task_handle;
        internal::loadContext(internal::m_context.m_current);
    }
    
    void start()
    {
        hardware::start();
        // system call - start first task
        kernel::internal::m_context.started = true;
        hardware::syscall(hardware::SyscallId::StartFirstTask); // TODO: can this be moved to hw?
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
        kernel::internal::CriticalSection cs;

        kernel::task::Id id;

        bool task_created = kernel::internal::task::create(a_routine, a_priority, &id, a_create_suspended);
        
        if (false == task_created)
        {
            return false;
        }

        bool task_added = kernel::scheduler::addTask(a_priority, id);

        if (false == task_added)
        {
            kernel::internal::task::destroy(id);
            return false;
        }

        if (a_handle)
        {
            *a_handle = id;
        }

        bool task_found = kernel::scheduler::findHighestPrioTask(kernel::internal::m_context.m_next);

        if (task_found) // Ignore 'false', because Idle task is always available.
        {
            if (kernel::internal::m_context.started)
            {
                hardware::syscall(hardware::SyscallId::ExecuteContextSwitch);
            }
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
        
        // Calculate Round-Robin time stamp
        // TODO: this looks like shit - make something nicer.
        if (m_context.time - m_context.old_time > m_context.interval)
        {
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
        
        m_context.time++;
        
        return execute_context_switch;
    }
}
