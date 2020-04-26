#pragma once

#include <task.hpp>

namespace kernel::scheduler
{
    bool addTask(kernel::task::Priority a_priority, kernel::task::id a_id);
    
    void removeTask(kernel::task::Priority a_priority, kernel::task::id a_id);
    
    bool findNextTask(kernel::task::Priority a_priority, kernel::task::id & a_id);
}
