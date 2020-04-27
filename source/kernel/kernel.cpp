#include <kernel.hpp>

#include <hardware.hpp>

#include <task.hpp>
#include <scheduler.hpp>

namespace
{
    constexpr uint32_t default_timer_interval = 10;
    
    typedef uint32_t time_ms_t;
    
    volatile struct
    {
        time_ms_t   time = 0;
        time_ms_t   interval = default_timer_interval;
        
        
        uint32_t m_current;
        uint32_t m_next;
        
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
        hardware::init();
        
        task::id idle_task_handle;
        kernel::task::create(idle_routine, task::Priority::Low, &idle_task_handle);
        
        scheduler::addTask(task::Priority::Low, idle_task_handle);
        
        m_context.m_current = idle_task_handle;
        m_context.m_next = idle_task_handle;
    }
    
    void start()
    {
        hardware::start();
    }
}

namespace kernel::hardware
{
    
    bool tick(volatile task_context & current, volatile task_context & next)
    {
        static bool first_run =  true;
        static time_ms_t old_time = 0;
        
        bool ret_val = false;
        
        if (m_context.time - old_time > m_context.interval)
        {
            // get current task prio
            // find next task in prio
            
            if (first_run) // load first task - no storing of current task
            {
                next.context = kernel::task::getContext(m_context.m_next);
                next.sp = kernel::task::getSp(m_context.m_next);
                
                first_run = !first_run;
                ret_val = true;
            }
        }
        
        m_context.time++;
        
        return ret_val;
    }
}
