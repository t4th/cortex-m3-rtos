#include "catch.hpp"

#include "scheduler.hpp"

// stubs
namespace
{
    void task_routine(void * param)
    {
        // empty task routine
    }

    void kernel_task_routine()
    {
    }
}

namespace kernel
{
    Time_ms getTime()
    {
        return 0U;
    }
}

namespace kernel::internal::timer
{
    State getState( Context & a_context, Id a_id)
    {
        return State::Finished;
    }
}

namespace kernel::internal::event
{
    void reset( Context & a_context, Id & a_id)
    {

    }

    State getState( Context & a_context, Id & a_id)
    {
        return State::Reset;
    }

    bool isManualReset( Context & a_context, Id & a_id)
    {
        return false;
    }
}

using namespace kernel::internal;

namespace
{
    struct local_context
    {
        scheduler::Context      m_Scheduler;
        task::Context           m_Task;
        std::vector<task::Id>   m_TaskHandles;
        timer::Context          m_Timer;
        event::Context          m_Event;

        uint32_t m_current;
        local_context() : m_current(0U) {}
        // allocate enough task structures that will be used
        // to test scheduler for given test case context
        // - create vector of empty handlers
        // - allocate internal::task and return new handles to previous vector
        void allocate_tasks(kernel::task::Priority a_prio, uint32_t a_task_count)
        {
            uint32_t i;
            for (i = 0U; i < a_task_count; ++i)
            {
                m_TaskHandles.push_back({}); // create physical task handle

                bool result = kernel::internal::task::create(
                    m_Task,
                    kernel_task_routine,
                    task_routine,
                    a_prio,
                    &m_TaskHandles.at(m_current + i), // initialize physical task handle
                    nullptr,
                    false
                );

                if(!result)
                {
                    REQUIRE(false); // shouldnt happen in test
                }
            }

            m_current += i;
        }
    };
}

// tests
TEST_CASE("Scheduler")
{
    SECTION( " add tasks of same priority and test round-robin scheduling")
    {
        std::unique_ptr<local_context> context(new local_context);

        // init
        // create 10 task structures
        {
            context->allocate_tasks(kernel::task::Priority::Low, 10);
        }

        // add task and test Current and Next
        auto add_and_test_task = []
        (
            local_context &         context,
            uint32_t                current,
            uint32_t                expected_curr,
            uint32_t                expected_next
        )
        {
            // add new task to scheduler
            bool result = scheduler::addReadyTask(
                context.m_Scheduler,
                context.m_Task,
                kernel::task::Priority::Low,
                context.m_TaskHandles.at(current)
            );

            REQUIRE(true == result);

            // get current task
            {
                task::Id found_id;
                scheduler::getCurrentTaskId(
                    context.m_Scheduler,
                    found_id
                );
                REQUIRE(expected_curr == found_id.m_id);
            }

            // get next task
            {
                task::Id found_id;

                result = scheduler::getNextTask(
                    context.m_Scheduler,
                    context.m_Task,
                    found_id
                );

                REQUIRE(true == result);
                REQUIRE(expected_next == found_id.m_id);
            }
        };

        // test case
        // See if adding new task to scheduler will update Current and Next correctly.
        {
            constexpr uint32_t count = 5U;

            std::array<uint32_t, count> expected_curr = {0U, 0U, 1U, 2U, 3U};
            std::array<uint32_t, count> expected_next = {0U, 1U, 2U, 3U, 4U};

            for (uint32_t i = 0U; i < count; ++i)
            {
                add_and_test_task(*context, i, expected_curr[i], expected_next[i]);
            }
        }

        // Check cirurality
        {
            bool result = false;
            task::Id found_id;

            constexpr uint32_t count = 10U;
            std::array<uint32_t, count> expected_next = 
            {0U, 1U, 2U, 3U, 4U, 0U, 1U, 2U, 3U, 4U};

            for (uint32_t i = 0U; i < count; ++i)
            {
                result = scheduler::getNextTask(
                    context->m_Scheduler,
                    context->m_Task,
                    found_id
                );

                REQUIRE(true == result);
                REQUIRE(expected_next[i] == found_id.m_id);
            }
        }
    }


    SECTION( " add tasks of different priorites and find highest prio")
    {
        std::unique_ptr<local_context> context(new local_context);

        // add some tasks
        {
            // add 3 low priority tasks
            context->allocate_tasks(kernel::task::Priority::Low, 3);

            for (uint32_t i = 0U; i < 3U; ++i)
            {
                bool result = scheduler::addReadyTask(
                    context->m_Scheduler,
                    context->m_Task,
                    kernel::task::Priority::Low,
                    context->m_TaskHandles.at(i)
                );
                REQUIRE(true == result);
            }

            // add 3 medium priority tasks
            context->allocate_tasks(kernel::task::Priority::Medium, 3);
            for (uint32_t i = 0U; i < 3U; ++i)
            {
                bool result = scheduler::addReadyTask(
                    context->m_Scheduler,
                    context->m_Task,
                    kernel::task::Priority::Medium,
                    context->m_TaskHandles.at(3U + i)
                );
                REQUIRE(true == result);
            }
            
            // add 3 high priority tasks
            context->allocate_tasks(kernel::task::Priority::High, 3);
            for (uint32_t i = 0U; i < 3U; ++i)
            {
                bool result = scheduler::addReadyTask(
                    context->m_Scheduler,
                    context->m_Task,
                    kernel::task::Priority::High,
                    context->m_TaskHandles.at(6U + i)
                );
                REQUIRE(true == result);
            }
        }

        // get current task
        {
            task::Id found_id;
            bool result = scheduler::getCurrentTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );
            REQUIRE(true == result);
            // task no. 6 is first High priority task
            REQUIRE(6U == found_id.m_id);
        }

        // get next task
        {
            task::Id found_id;

            bool result = scheduler::getNextTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );

            REQUIRE(true == result);
            // task no. 7 is second High priority task
            REQUIRE(7U == found_id.m_id);
        }

        // remove all 3 high priority tasks
        {
            for (uint32_t i = 0U; i < 3U; ++i)
            {
                scheduler::removeTask(
                context->m_Scheduler,
                context->m_Task,
                context->m_TaskHandles.at(6U + i)
                );
            }
        }

        // get current task
        {
            task::Id found_id;
            bool result = scheduler::getCurrentTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );
            REQUIRE(true == result);
            // task no. 3 is first Medium priority task
            REQUIRE(3U == found_id.m_id);
        }

        // get next task
        {
            task::Id found_id;

            bool result = scheduler::getNextTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );

            REQUIRE(true == result);
            // task no. 4 is second Medium priority task
            REQUIRE(4U == found_id.m_id);
        }

        // get current task Id and remove it
        {
            task::Id found_id;

            scheduler::getCurrentTaskId(
                context->m_Scheduler,
                found_id
            );

            // should find previous getNextTask result, which is 4
            REQUIRE(4U == found_id.m_id);

            scheduler::removeTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );
        }

        // get current task after removing previous current (no. 4)
        {
            task::Id found_id;

            bool result = scheduler::getCurrentTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );

            REQUIRE(true == result);
            // task no. 5 is third (now second due to linked list)
            // Medium priority task
            REQUIRE(5U == found_id.m_id);
        }

        // get next task
        {
            task::Id found_id;

            bool result = scheduler::getNextTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );

            REQUIRE(true == result);
            // task no. 3 is fist Medium priority task
            REQUIRE(3U == found_id.m_id);
        }

        // now suspend last to medium tasks, so Low tasks have something to do
        {
            task::Id task_to_suspend;

            task_to_suspend.m_id = 3U;

            scheduler::setTaskToSuspended(
                context->m_Scheduler,
                context->m_Task,
                task_to_suspend
                );

            task_to_suspend.m_id = 5U;

            scheduler::setTaskToSuspended(
                context->m_Scheduler,
                context->m_Task,
                task_to_suspend
            );
        }

        // at this point all Hight and Medium tasks are either removed or
        // suspended, so Low priority task should be scheduled
        {
            task::Id found_id;

            bool result = scheduler::getCurrentTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );

            REQUIRE(true == result);
            REQUIRE(0U == found_id.m_id);
        }

        // get next task
        {
            task::Id found_id;

            bool result = scheduler::getNextTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );

            REQUIRE(true == result);
            REQUIRE(1U == found_id.m_id);
        }

        // resume one Medium task, so Low
        {
            // 4U is first Medium task that was suspended,
            // lets set it to Ready again
            task::Id task_to_resume;
            task_to_resume.m_id = 4U;

            scheduler::setTaskToReady(
                context->m_Scheduler,
                context->m_Task,
                task_to_resume
            );
        }

        // now that 1st Medium task is ready again, it should be scheduled
        {
            task::Id found_id;

            bool result = scheduler::getCurrentTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );

            REQUIRE(true == result);
            REQUIRE(4U == found_id.m_id);
        }

        // get next task
        {
            task::Id found_id;

            bool result = scheduler::getNextTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );

            REQUIRE(true == result);
            // since there is only one Medium task in Medium queue,
            // scheduler should return it (4U)
            REQUIRE(4U == found_id.m_id);
        }
    }

    // todo next
    // test setTaskToWait and waiting conditions
}
