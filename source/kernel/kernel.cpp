#include <kernel.hpp>

#include <hardware.hpp>

#include <task.hpp>
#include <scheduler.hpp>

namespace
{
    constexpr uint32_t DEFAULT_TIMER_INTERVAL = 10;
    
    typedef uint32_t Time_ms;
    
    volatile struct
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
}

namespace kernel
{
    void init()
    {
        m_context.old_time = 0;
        m_context.first_run = true;

        hardware::init();
        
        task::Id idle_task_handle;
        kernel::task::create(idle_routine, task::Priority::Low, &idle_task_handle);
        
        scheduler::addTask(task::Priority::Low, idle_task_handle);
        
        m_context.m_current = idle_task_handle;
        m_context.m_next = idle_task_handle;

        m_context.switch_requested = true;
    }
    
    void start()
    {
        hardware::start();
    }
}

namespace kernel
{
    // TODO: all data must be in critical section
    bool tick(volatile task_context & current, volatile task_context & next)
    {
        bool execute_context_switch = false;
        
        if (m_context.time - m_context.old_time > m_context.interval)
        {
            if (false == m_context.switch_requested) // Do arbitration.
            {
                // Get current task priority.
                const task::Priority current  = kernel::task::priority::get(m_context.m_current);
                
                // Find next task in priority.
                scheduler::findNextTask(current, m_context.m_next);

                if (m_context.m_current != m_context.m_next)
                {
                    m_context.switch_requested = true;
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
                // Store context.
                const uint32_t current_sp = hardware::sp::get();
                kernel::task::sp::set(m_context.m_current, current_sp);
                current.context = kernel::task::context::getRef(m_context.m_current);
            }

            // Load next task context.
            next.context = kernel::task::context::getRef(m_context.m_next);
            const uint32_t next_sp = kernel::task::sp::get(m_context.m_next);
            hardware::sp::set(next_sp );

            m_context.m_current = m_context.m_next;

            execute_context_switch = true;
            m_context.switch_requested = false;
        }
        
        m_context.time++;
        
        return execute_context_switch;
    }
}
