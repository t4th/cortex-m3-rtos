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
    kernel::internal::scheduler::Context context;

    SECTION( "Add new tasks to scheduler")
    {
        kernel::task::Id next_task_id;

        next_task_id.m_id = 'a';
        kernel::internal::scheduler::addTask(context, kernel::task::Priority::High, next_task_id);

        REQUIRE(false == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
        REQUIRE('a' == next_task_id.m_id);

        next_task_id.m_id = 'b';
        kernel::internal::scheduler::addTask(context, kernel::task::Priority::High, next_task_id);

        REQUIRE(true == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
        REQUIRE('b' == next_task_id.m_id);

        REQUIRE(true == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
        REQUIRE('a' == next_task_id.m_id);

        next_task_id.m_id = 'c';
        kernel::internal::scheduler::addTask(context, kernel::task::Priority::High, next_task_id);

        REQUIRE(true == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
        REQUIRE('b' == next_task_id.m_id);

        REQUIRE(true == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
        REQUIRE('c' == next_task_id.m_id);

        REQUIRE(true == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
        REQUIRE('a' == next_task_id.m_id);
        
        next_task_id.m_id = 'a';
        kernel::internal::scheduler::removeTask(context, kernel::task::Priority::High, next_task_id);

        REQUIRE(true == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
        REQUIRE('c' == next_task_id.m_id);

        REQUIRE(true == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
        REQUIRE('b' == next_task_id.m_id);

        REQUIRE(true == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
        REQUIRE('c' == next_task_id.m_id);

        next_task_id.m_id = 'b';
        kernel::internal::scheduler::removeTask(context, kernel::task::Priority::High, next_task_id);

        REQUIRE(false == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
    }
}
