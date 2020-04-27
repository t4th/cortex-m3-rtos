#include <kernel.hpp>

void routine()
{
    
}

int main()
{
    kernel::init();
    
    kernel::start();
    
    for(;;);
}
