#include <kernel.hpp>
#include <hardware.hpp>

namespace kernel
{
    void init()
    {
        hardware::init();
    }
    
    void tick()
    {
    }
    
    void start()
    {
        hardware::start();
    }
}
