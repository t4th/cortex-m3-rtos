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
        
    } m_context;

    void idle_routine()
    {
        while(1)
        {
            // TODO: calculate CPU load
        }
    }

    void executeContextSwitch()
    {
        // TODO: implement
        // this sucker will be used for kernel api calls (create thread, event, etc)
        // m_context.old_time = m_context.time;
    }

    void storeContext()
    {
        const uint32_t sp = hardware::sp::get();
        kernel::task::sp::set(m_context.m_current, sp);

        kernel::hardware::task::Context * current_task = kernel::task::context::get(m_context.m_current);
        kernel::hardware::context::current::set(current_task);
    }

    void loadContext()
    {
        kernel::hardware::task::Context * next_task = kernel::task::context::get(m_context.m_next);
        kernel::hardware::context::next::set(next_task);

        const uint32_t next_sp = kernel::task::sp::get(m_context.m_next);
        kernel::hardware::sp::set(next_sp );
    }
}

namespace kernel
{
    void init()
    {
        internal::m_context.old_time = 0;

        hardware::init();
        
        task::Id idle_task_handle;
        kernel::task::create(internal::idle_routine, task::Priority::Idle, &idle_task_handle);
        
        scheduler::addTask(task::Priority::Idle, idle_task_handle);
        
        internal::m_context.m_current = idle_task_handle;
        internal::m_context.m_next = idle_task_handle;
        internal::loadContext();
    }
    
    void start()
    {
        hardware::start();
        // system call - start first task
        hardware::syscall(hardware::SyscallId::StartFirstTask); // TODO: can this be moved to hw?
    }
}

namespace kernel::internal
{
    // TODO: all data must be in critical section
    //  ...or dont. So far everything is designed to kernel m_context is
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
            const task::Priority current  = kernel::task::priority::get(m_context.m_current);
                
            // Find next task in priority group.
            if(true == scheduler::findNextTask(current, m_context.m_next))
            {
                storeContext();
                loadContext();

                m_context.m_current = m_context.m_next;

                execute_context_switch = true;
            }
        }
        
        m_context.time++;
        
        return execute_context_switch;
    }
}
