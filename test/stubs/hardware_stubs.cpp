#include <kernel.hpp>

// Stubs for hardware interface.
namespace kernel::hardware::critical_section
{
    void enter(
        Context & a_context,
        interrupt::priority::Preemption a_preemption_priority)
    {
    }

    void leave( Context & a_context)
    {
    }
}
