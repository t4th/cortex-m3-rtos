#include "catch.hpp"

#include "scheduler.hpp"

TEST_CASE("Scheduler")
{
    kernel::internal::scheduler::Context context;

    SECTION( "Add task 'a', 'b', 'c' and see if findNextTask is working")
    {
        kernel::internal::task::Id next_task_id;

        // add task 'a'
        next_task_id.m_id = 'a';
        REQUIRE(true == kernel::internal::scheduler::addTask(context, kernel::task::Priority::High, next_task_id));

        REQUIRE(false == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
        REQUIRE('a' == next_task_id.m_id);

        // add task 'b'
        next_task_id.m_id = 'b';
        REQUIRE(true == kernel::internal::scheduler::addTask(context, kernel::task::Priority::High, next_task_id));

        REQUIRE(true == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
        REQUIRE('b' == next_task_id.m_id);

        REQUIRE(true == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
        REQUIRE('a' == next_task_id.m_id);

        // add task 'c'
        next_task_id.m_id = 'c';
        REQUIRE(true == kernel::internal::scheduler::addTask(context, kernel::task::Priority::High, next_task_id));

        REQUIRE(true == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
        REQUIRE('b' == next_task_id.m_id);

        REQUIRE(true == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
        REQUIRE('c' == next_task_id.m_id);

        REQUIRE(true == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
        REQUIRE('a' == next_task_id.m_id);
        
        // remove task 'a'
        next_task_id.m_id = 'a';
        kernel::internal::scheduler::removeTask(context, kernel::task::Priority::High, next_task_id);

        REQUIRE(true == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
        REQUIRE('c' == next_task_id.m_id);

        REQUIRE(true == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
        REQUIRE('b' == next_task_id.m_id);

        REQUIRE(true == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
        REQUIRE('c' == next_task_id.m_id);

        // remove task 'b'
        next_task_id.m_id = 'b';
        kernel::internal::scheduler::removeTask(context, kernel::task::Priority::High, next_task_id);

        // only one task is remaining - task 'c'
        // if one task is left, expected return value is 'false'
        REQUIRE(false == kernel::internal::scheduler::findNextTask(context, kernel::task::Priority::High, next_task_id));
        // next_task_id remain unchanged
        REQUIRE('b' == next_task_id.m_id);
    }

    SECTION( "try to add too many task")
    {
        kernel::internal::task::Id next_task_id;

        for (uint32_t i = 0U; i < kernel::internal::task::MAX_NUMBER; i++)
        {
            next_task_id.m_id = i;
            REQUIRE(true == kernel::internal::scheduler::addTask(context, kernel::task::Priority::High, next_task_id));
        }

        // Adding MAX_TASK_NUMBER + 1 task should fail
        next_task_id.m_id = 0x123U;
        REQUIRE(false == kernel::internal::scheduler::addTask(context, kernel::task::Priority::High, next_task_id));
    }

    SECTION( "try to add dublicate with same id")
    {
        kernel::internal::task::Id next_task_id;

        next_task_id.m_id = 0x123U;
        REQUIRE(true == kernel::internal::scheduler::addTask(context, kernel::task::Priority::High, next_task_id));


        next_task_id.m_id = 0x124U;
        REQUIRE(true == kernel::internal::scheduler::addTask(context, kernel::task::Priority::High, next_task_id));


        next_task_id.m_id = 0x123U;

        // addTask should return false
        REQUIRE(false == kernel::internal::scheduler::addTask(context, kernel::task::Priority::High, next_task_id));
    }

    SECTION( "test finding highest priority task")
    {
        kernel::internal::task::Id next_task_id;
        // Add 2 task to each priority group

        // High: 
        next_task_id.m_id = 1;
        REQUIRE(true == kernel::internal::scheduler::addTask(context, kernel::task::Priority::High, next_task_id));
        next_task_id.m_id = 2;
        REQUIRE(true == kernel::internal::scheduler::addTask(context, kernel::task::Priority::High, next_task_id));
        
        // Medium:
        next_task_id.m_id = 3;
        REQUIRE(true == kernel::internal::scheduler::addTask(context, kernel::task::Priority::Medium, next_task_id));
        next_task_id.m_id = 4;
        REQUIRE(true == kernel::internal::scheduler::addTask(context, kernel::task::Priority::Medium, next_task_id));

        // Low:
        next_task_id.m_id = 5;
        REQUIRE(true == kernel::internal::scheduler::addTask(context, kernel::task::Priority::Low, next_task_id));
        next_task_id.m_id = 6;
        REQUIRE(true == kernel::internal::scheduler::addTask(context, kernel::task::Priority::Low, next_task_id));

        // Idle:
        next_task_id.m_id = 7;
        REQUIRE(true == kernel::internal::scheduler::addTask(context, kernel::task::Priority::Idle, next_task_id));
        next_task_id.m_id = 8;
        REQUIRE(true == kernel::internal::scheduler::addTask(context, kernel::task::Priority::Idle, next_task_id));

        // find highest priority task
        kernel::internal::scheduler::findHighestPrioTask(context, next_task_id);
        REQUIRE(1 == next_task_id.m_id);

        // remove 2 high priority tasks
        kernel::internal::scheduler::removeTask(context, kernel::task::Priority::High, {1});
        kernel::internal::scheduler::removeTask(context, kernel::task::Priority::High, {2});

        // find highest priority task
        kernel::internal::scheduler::findHighestPrioTask(context, next_task_id);
        REQUIRE(3 == next_task_id.m_id);
    }
}
