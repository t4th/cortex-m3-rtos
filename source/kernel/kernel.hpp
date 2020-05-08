#pragma once

#include <cstdint>
#include <hardware.hpp> //TODO: create kerneli::api fascade

namespace kernel
{    
    void init();
    void start();
}

namespace kernel::internal
{
    bool tick() __attribute__((always_inline));
}
