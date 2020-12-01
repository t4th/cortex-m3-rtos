#include <hardware.hpp>


extern "C"
{
    void __aeabi_assert(
        const char * expr,
        const char * file,
        int line
    )
    {
        kernel::hardware::debug::setBreakpoint();
    }
}
