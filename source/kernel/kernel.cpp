#include <kernel.hpp>

#include <hardware.hpp>

namespace kernel
{
    constexpr uint32_t default_timer_interval = 10;
    
    typedef uint32_t time_ms_t;
    
    struct
    {
        time_ms_t   time = 0;
        time_ms_t   interval = default_timer_interval;
    } context;
    

    
    void init()
    {
        hardware::init();
    }
    
    void start()
    {
        hardware::start();
    }
    
    bool tick(volatile task_context & current, volatile task_context & next)
    {
        static time_ms_t old_time = 0;
        
        if (kernel::context.time - old_time > kernel::context.interval)
        {
        }
        
        return false;
    }
}
