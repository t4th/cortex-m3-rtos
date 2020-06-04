#pragma once

#include <task.hpp>

namespace kernel::scheduler
{
    bool addTask(kernel::task::Priority a_priority, kernel::task::Id a_id);
    
    void removeTask(kernel::task::Priority a_priority, kernel::task::Id a_id);
    
    bool findNextTask(kernel::task::Priority a_priority, volatile kernel::task::Id & a_id);

    // return id of highest priority task.
    // idle task is always available as lowest possible priority thus function always success.
    void findHighestPrioTask(volatile kernel::task::Id & a_id);
}
