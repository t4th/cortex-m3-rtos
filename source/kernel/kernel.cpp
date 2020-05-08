#include <kernel.hpp>

#include <hardware.hpp>

#include <task.hpp>
#include <scheduler.hpp>

// This is workaround to use C++ variables symbols in inline assembly.
extern volatile kernel::hardware::task::Context * current_task_context;
extern volatile kernel::hardware::task::Context * next_task_context;

namespace kernel::internal
{
    constexpr uint32_t DEFAULT_TIMER_INTERVAL = 10;
    
    typedef uint32_t Time_ms;
    
    volatile struct Context
    {
        bool    first_run;
        bool    switch_requested;

        Time_ms old_time;
        Time_ms time;
        Time_ms interval = DEFAULT_TIMER_INTERVAL;
        
        
        kernel::task::Id m_current;
        kernel::task::Id m_next;
        
    } m_context;

    void idle_routine()
    {
        volatile int i = 0;
        while(1)
        {
            i++;
        }
    }

    void storeContext()
    {
        const uint32_t sp = hardware::sp::get();
        kernel::task::sp::set(m_context.m_current, sp);
        current_task_context = kernel::task::context::getRef(m_context.m_current);
    }

    void loadContext()
    {
        next_task_context = kernel::task::context::getRef(m_context.m_next);
        const uint32_t next_sp = kernel::task::sp::get(m_context.m_next);
        hardware::sp::set(next_sp );
    }
}

namespace kernel
{
    void init()
    {
        internal::m_context.old_time = 0;
        internal::m_context.first_run = true;

        hardware::init();
        
        task::Id idle_task_handle;
        kernel::task::create(internal::idle_routine, task::Priority::Idle, &idle_task_handle);
        
        scheduler::addTask(task::Priority::Idle, idle_task_handle);
        
        internal::m_context.m_current = idle_task_handle;
        internal::m_context.m_next = idle_task_handle;

        internal::m_context.switch_requested = true;
    }
    
    void start()
    {
        hardware::start();
        // system call - start first task
        hardware::syscall(hardware::SyscallId::StartFirstTask);
    }
}

namespace kernel::internal
{
    // TODO: all data must be in critical section
    bool tick()
    {
        bool execute_context_switch = false;
        
        if (m_context.time - m_context.old_time > m_context.interval)
        {
            if (false == m_context.switch_requested) // Do arbitration.
            {
                // Get current task priority.
                const task::Priority current  = kernel::task::priority::get(m_context.m_current);
                
                // Find next task in priority group.
                if(true == scheduler::findNextTask(current, m_context.m_next))
                {
                    if (m_context.m_current != m_context.m_next)
                    {
                        m_context.switch_requested = true;
                    }
                }
            }
            else  // Reset timestamp.
            {
                m_context.old_time = m_context.time;
            }
        }
        
        if (true == m_context.switch_requested)
        {
            // First_run used to load first task - skip Store Context. Workaround.
            if (true == m_context.first_run)
            {
                m_context.first_run = false;
            }
            else
            {
                storeContext();
            }

            loadContext();

            m_context.m_current = m_context.m_next;

            execute_context_switch = true;
            m_context.switch_requested = false;
        }
        
        m_context.time++;
        
        return execute_context_switch;
    }
}
