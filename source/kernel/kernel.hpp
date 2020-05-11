#pragma once

#include <cstdint>

namespace kernel
{    
    void init();
    void start();
}

namespace kernel::internal
{
    bool tick() __attribute__((always_inline));
}
