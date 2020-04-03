#include <kernel.hpp>
#include <task.hpp>

void routine()
{
    
}

int main()
{
    kernel::init();
    
    kernel::start();
    
    kernel::task::create(routine);
    
    for(;;);
}
