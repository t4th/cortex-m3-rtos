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
        timer::Context          m_Timer;
        event::Context          m_Event;

        std::vector<task::Id> tasks;
    };
}

// tests
TEST_CASE("Scheduler")
{
    SECTION( " add tasks of same priority and test round-robin scheduling")
    {
        std::unique_ptr<local_context> context(new local_context);

        // add task and test Current and Next
        auto add_and_test_task = []
        (
            local_context &         context,
            uint32_t                current,
            uint32_t                expected_curr,
            uint32_t                expected_next
        )
        {
            bool result = false;
            
            // new task Id
            context.tasks.push_back({});

            // create task structure
            kernel::internal::task::create(
                context.m_Task,
                kernel_task_routine,
                task_routine,
                kernel::task::Priority::Low,
                &context.tasks.at(current),
                nullptr,
                false
            );

            // add new task to scheduler
            result = scheduler::addReadyTask(
                context.m_Scheduler,
                context.m_Task,
                kernel::task::Priority::Low,
                context.tasks.at(current)
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
        // todo
        // remove highest prio task
        // remove current task
    }
}
