#include <kernel.hpp>


int main()
{
    kernel::init();
    
    kernel::start();
    
    for(;;);
}
