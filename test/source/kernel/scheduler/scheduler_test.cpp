#include "catch.hpp"

#include "scheduler.hpp"

// Mock kernel::task
namespace kernel::task
{
    bool create(
        Routine     a_routine,
        Priority    a_priority,
        uint32_t *  a_handle,
        bool        a_create_suspended
    )
    {
        return false;
    }
}

TEST_CASE("Scheduler")
{
    SECTION( "Add new tasks to scheduler")
    {
        kernel::scheduler::addTask(kernel::task::Priority::High, 'a');

        kernel::task::id next_task_id = 0;
        REQUIRE(false == kernel::scheduler::findNextTask(kernel::task::Priority::High, next_task_id));
        REQUIRE(0 == next_task_id);

        kernel::scheduler::addTask(kernel::task::Priority::High, 'b');

        REQUIRE(true == kernel::scheduler::findNextTask(kernel::task::Priority::High, next_task_id));
        REQUIRE('b' == next_task_id);

        REQUIRE(true == kernel::scheduler::findNextTask(kernel::task::Priority::High, next_task_id));
        REQUIRE('a' == next_task_id);

        kernel::scheduler::addTask(kernel::task::Priority::High, 'c');

        REQUIRE(true == kernel::scheduler::findNextTask(kernel::task::Priority::High, next_task_id));
        REQUIRE('b' == next_task_id);

        REQUIRE(true == kernel::scheduler::findNextTask(kernel::task::Priority::High, next_task_id));
        REQUIRE('c' == next_task_id);

        REQUIRE(true == kernel::scheduler::findNextTask(kernel::task::Priority::High, next_task_id));
        REQUIRE('a' == next_task_id);

        kernel::scheduler::removeTask(kernel::task::Priority::High, 'a');

        REQUIRE(true == kernel::scheduler::findNextTask(kernel::task::Priority::High, next_task_id));
        REQUIRE('c' == next_task_id);

        REQUIRE(true == kernel::scheduler::findNextTask(kernel::task::Priority::High, next_task_id));
        REQUIRE('b' == next_task_id);

        REQUIRE(true == kernel::scheduler::findNextTask(kernel::task::Priority::High, next_task_id));
        REQUIRE('c' == next_task_id);

        kernel::scheduler::removeTask(kernel::task::Priority::High, 'b');

        REQUIRE(false == kernel::scheduler::findNextTask(kernel::task::Priority::High, next_task_id));
    }
}
