#include "catch.hpp"

#include <scheduler.hpp>

#include <event.hpp>
#include <timer.hpp>

#include <array>

// This unit test validate scheduler module. It is implicit kernel::internal namespace
// feature, so all used context is exposed and need to be pre-allocated for propher test.

// stubs
namespace
{
    void task_routine(void * param)
    {
    }

    void kernel_task_routine()
    {
    }
}

namespace kernel::hardware
{
    namespace debug
    {
        void setBreakpoint()
        {
        }
    }

    namespace critical_section
    {
        void enter(
            volatile Context &                  a_context,
            interrupt::priority::Preemption     a_preemption_priority
        )
        {
        }

        void leave( volatile Context & a_context)
        {
        }
    }
}

using namespace kernel::internal;

namespace
{
    struct test_case_context
    {
        // kernel::internal context
        scheduler::Context      m_Scheduler;
        task::Context           m_Task;
        timer::Context          m_Timer;
        event::Context          m_Event;
        queue::Context          m_Queue;

        // members used by specyfic test case instance
        std::vector< task::Id>   m_TaskHandles;
        uint32_t                 m_current;

        test_case_context() : m_current( 0U) {}

        // allocate enough task structures that will be used
        // to test scheduler for given test case context
        // - create vector of empty handlers
        // - allocate internal::task and return new handles to previous vector
        void allocate_tasks( kernel::task::Priority a_prio, uint32_t a_task_count)
        {
            uint32_t i;

            if ( a_task_count > kernel::internal::task::max_number)
            {
                // Tests are allocating more memory than set in config.hpp.
                // Increase kernel::internal::task::max_number to fit tests.
                REQUIRE( false);
            }

            for ( i = 0U; i < a_task_count; ++i)
            {
                m_TaskHandles.push_back( {}); // create physical task handle

                bool result = kernel::internal::task::create(
                    m_Task,
                    kernel_task_routine,
                    task_routine,
                    a_prio,
                    &m_TaskHandles.at( m_current + i), // initialize physical task handle
                    nullptr,
                    false
                );

                if( !result)
                {
                    // shouldnt happen in test :P
                    REQUIRE( false); 
                }
            }

            m_current += i;
        }
    };
}

// tests
TEST_CASE("Scheduler")
{
    SECTION( "Add tasks of same priority and test round-robin scheduling.")
    {
        std::unique_ptr<test_case_context> context( new test_case_context);

        // Create 10 internal::task structures.
        {
            context->allocate_tasks( kernel::task::Priority::Low, 10U);
        }

        // Add task and test Current and Next.
        auto add_and_test_task = []
        (
            test_case_context &         context,
            uint32_t                    current,
            task::Id                    expected_curr,
            task::Id                    expected_next
        )
        {
            // Add new task ID to scheduler.
            bool result = scheduler::addReadyTask(
                context.m_Scheduler,
                context.m_Task,
                context.m_TaskHandles.at( current)
            );

            REQUIRE( true == result);

            // Get current task ID.
            {
                task::Id found_id =
                    scheduler::getCurrentTaskId( context.m_Scheduler);

                REQUIRE( expected_curr == found_id);
            }

            // Get next task ID.
            {
                task::Id found_id;

                result = scheduler::getNextTask(
                    context.m_Scheduler,
                    context.m_Task,
                    found_id
                );

                REQUIRE( true == result);
                REQUIRE( expected_next == found_id);
            }
        };

        // Main test code:
        // Check if adding new task to scheduler will update Current and Next correctly.
        {
            constexpr uint32_t count = 5U;

            std::array< uint32_t, count> expected_curr = { 0U, 0U, 1U, 2U, 3U};
            std::array< uint32_t, count> expected_next = { 0U, 1U, 2U, 3U, 4U};

            for ( uint32_t i = 0U; i < count; ++i)
            {
                add_and_test_task(
                    *context,
                    i,
                    static_cast< task::Id>( expected_curr[ i]),
                    static_cast< task::Id>( expected_next[ i])
                );
            }
        }

        // Check circularity.
        {
            bool result = false;
            task::Id found_id;

            constexpr uint32_t count = 10U;
            std::array< uint32_t, count> expected_next = 
            { 0U, 1U, 2U, 3U, 4U, 0U, 1U, 2U, 3U, 4U};

            for (uint32_t i = 0U; i < count; ++i)
            {
                result = scheduler::getNextTask(
                    context->m_Scheduler,
                    context->m_Task,
                    found_id
                );

                REQUIRE( true == result);
                REQUIRE( static_cast< task::Id>( expected_next[ i]) == found_id);
            }
        }
    }


    SECTION( "Add tasks of different priorites and find highest priority.")
    {
        std::unique_ptr< test_case_context> context( new test_case_context);

        // Add some tasks and keep their ID in vector m_TaskHandles.
        {
            // Add 3 LOW priority tasks.
            context->allocate_tasks( kernel::task::Priority::Low, 3U);

            for ( uint32_t i = 0U; i < 3U; ++i)
            {
                bool result = scheduler::addReadyTask(
                    context->m_Scheduler,
                    context->m_Task,
                    context->m_TaskHandles.at( i)
                );

                REQUIRE( true == result);
            }

            // Add 3 MEDIUMA priority tasks.
            context->allocate_tasks( kernel::task::Priority::Medium, 3U);

            for ( uint32_t i = 0U; i < 3U; ++i)
            {
                bool result = scheduler::addReadyTask(
                    context->m_Scheduler,
                    context->m_Task,
                    context->m_TaskHandles.at( 3U + i)
                );

                REQUIRE( true == result);
            }
            
            // Add 3 HIGH priority tasks.
            context->allocate_tasks( kernel::task::Priority::High, 3U);

            for ( uint32_t i = 0U; i < 3U; ++i)
            {
                bool result = scheduler::addReadyTask(
                    context->m_Scheduler,
                    context->m_Task,
                    context->m_TaskHandles.at( 6U + i)
                );

                REQUIRE( true == result);
            }
        }

        // Check current task ID.
        // Expected: task no. 6 is first High priority task.
        {
            task::Id found_id;

            bool result = scheduler::getCurrentTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );

            REQUIRE( true == result);
            REQUIRE( static_cast< task::Id>( 6U) == found_id);
        }

        // Check next task ID.
        // Expected: task no. 7 is second High priority task.
        {
            task::Id found_id;

            bool result = scheduler::getNextTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );

            REQUIRE( true == result);
            REQUIRE( static_cast< task::Id>( 7U) == found_id);
        }

        // Remove all HIGH priority tasks.
        {
            for ( uint32_t i = 0U; i < 3U; ++i)
            {
                scheduler::removeTask(
                    context->m_Scheduler,
                    context->m_Task,
                    context->m_TaskHandles.at( 6U + i)
                );
            }
        }

        // Check current task ID.
        // Expected: task no. 3 is first Medium priority task.
        {
            task::Id found_id;

            bool result = scheduler::getCurrentTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );
            REQUIRE( true == result);
            REQUIRE( static_cast< task::Id>( 3U) == found_id);
        }

        // Check next task ID.
        // Expected: task no. 4 is second Medium priority task.
        {
            task::Id found_id;

            bool result = scheduler::getNextTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );

            REQUIRE( true == result);
            REQUIRE( static_cast< task::Id>( 4U) == found_id);
        }

        // Get current task Id and remove it.
        {
            task::Id found_id = scheduler::getCurrentTaskId( context->m_Scheduler);

            // Expected: Should find previous getNextTask result, which is 4.
            REQUIRE( static_cast< task::Id>( 4U) == found_id);

            scheduler::removeTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );
        }

        // Get current task after removing previous current (no. 4).
        {
            task::Id found_id;

            bool result = scheduler::getCurrentTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );

            REQUIRE( true == result);
            // Expected: task no. 5 is third (now second due to linked list)
            //           Medium priority task
            REQUIRE( static_cast< task::Id>( 5U) == found_id);
        }

        // Check next task ID.
        // Expected: task no. 3 is fist Medium priority task.
        {
            task::Id found_id;

            bool result = scheduler::getNextTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );

            REQUIRE( true == result);
            REQUIRE( static_cast< task::Id>( 3U) == found_id);
        }

        // Suspend last two Medium tasks, so LOW priority tasks are ready.
        {
            task::Id task_to_suspend;

            task_to_suspend = static_cast< task::Id>( 3U);

            scheduler::setTaskToSuspended(
                context->m_Scheduler,
                context->m_Task,
                task_to_suspend
                );

            task_to_suspend = static_cast< task::Id>( 5U);

            scheduler::setTaskToSuspended(
                context->m_Scheduler,
                context->m_Task,
                task_to_suspend
            );
        }

        // Expected: At this point all Hight and Medium tasks are either removed or
        // suspended, so Low priority task should be scheduled.
        {
            task::Id found_id;

            bool result = scheduler::getCurrentTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );

            REQUIRE( true == result);
            REQUIRE( static_cast< task::Id>( 0U) == found_id);
        }

        // Check next task ID.
        {
            task::Id found_id;

            bool result = scheduler::getNextTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );

            REQUIRE( true == result);
            REQUIRE( static_cast< task::Id>( 1U) == found_id);
        }

        // Resume one Medium task, so Low priority is skipped.
        {
            // 3U is first Medium task that was suspended,
            // lets set it to Ready again
            task::Id task_to_resume = static_cast< task::Id>( 3U);

            scheduler::resumeSuspendedTask(
                context->m_Scheduler,
                context->m_Task,
                task_to_resume
            );
        }

        // Expected: Now that 1st Medium task is ready again, it should be scheduled.
        {
            task::Id found_id;

            bool result = scheduler::getCurrentTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );

            REQUIRE( true == result);
            REQUIRE( static_cast< task::Id>( 3U) == found_id);
        }

        // Check next task ID.
        // Expected: since there is only one Medium task in Medium queue,
        //           scheduler should return it (4U).
        {
            task::Id found_id;

            bool result = scheduler::getNextTask(
                context->m_Scheduler,
                context->m_Task,
                found_id
            );

            REQUIRE( true == result);
            REQUIRE( static_cast< task::Id>( 3U) == found_id);
        }
    }

    SECTION( "Create 3 tasks and set one to sleep.")
    {
        std::unique_ptr<test_case_context> context(new test_case_context);

        kernel::TimeMs timeout = 100U;
        kernel::TimeMs current = 100U;

        // Pre-condition
        {
            // Create 3 LOW priority tasks.
            context->allocate_tasks( kernel::task::Priority::Low, 3);

            // Add newly created task IDs to scheduler.
            for ( uint32_t i = 0U; i < 3U; ++i)
            {
                bool result = scheduler::addReadyTask(
                    context->m_Scheduler,
                    context->m_Task,
                    context->m_TaskHandles.at(i)
                );

                REQUIRE( true == result);
            }
        }

        // Check tasks states.
        {
            using namespace kernel::internal;
            REQUIRE( kernel::task::State::Ready == task::state::get( context->m_Task, context->m_TaskHandles.at( 0U)));
            REQUIRE( kernel::task::State::Ready == task::state::get( context->m_Task, context->m_TaskHandles.at( 1U)));
            REQUIRE( kernel::task::State::Ready == task::state::get( context->m_Task, context->m_TaskHandles.at( 2U)));
        }

        // Do some normal scheduling:
        // Tasks queue: 0 (current) - 1 - 2
        {
            // Get current task ID.
            // Expected: Current task should be Task 0.
            {
                task::Id found_id;

                bool result = scheduler::getCurrentTask(
                    context->m_Scheduler,
                    context->m_Task,
                    found_id
                );

                REQUIRE( true == result);
                REQUIRE( static_cast< task::Id>( 0U) == found_id);
            }

            // Check tasks states.
            {
                using namespace kernel::internal;
                REQUIRE( kernel::task::State::Running == task::state::get( context->m_Task, context->m_TaskHandles.at( 0U)));
                REQUIRE( kernel::task::State::Ready ==   task::state::get( context->m_Task, context->m_TaskHandles.at( 1U)));
                REQUIRE( kernel::task::State::Ready ==   task::state::get( context->m_Task, context->m_TaskHandles.at( 2U)));
            }

            // Check next task ID.
            // Expected: Next task should be Task 0.
            {
                task::Id found_id;

                bool result = scheduler::getNextTask(
                    context->m_Scheduler,
                    context->m_Task,
                    found_id
                );

                REQUIRE( true == result);
                REQUIRE( static_cast< task::Id>( 1U) == found_id);
            }

            // Check tasks states.
            {
                using namespace kernel::internal;
                REQUIRE( kernel::task::State::Ready ==   task::state::get( context->m_Task, context->m_TaskHandles.at( 0U)));
                REQUIRE( kernel::task::State::Running == task::state::get( context->m_Task, context->m_TaskHandles.at( 1U)));
                REQUIRE( kernel::task::State::Ready ==   task::state::get( context->m_Task, context->m_TaskHandles.at( 2U)));
            }
        }

        // Set task 0 to sleep.
        // Tasks queue:
        // wait: 0 (sleep)
        // ready: 1(current) - 2
        {
            task::Id task_to_sleep = context->m_TaskHandles.at( 0U);
            kernel::TimeMs current_time = 0U;

            bool result = scheduler::setTaskToSleep(
                context->m_Scheduler,
                context->m_Task,
                task_to_sleep,
                timeout,
                current_time
            );

            REQUIRE( true == result);
        }

        // After Task 0 is set to sleep, scheduler should go to 
        // task 2 and then to Task 1 (skiping sleeping Task 0).
        {
            // Check next task ID.
            // Expected: Next task should be Task 2.
            // Tasks queue:
            // wait: 0 (sleep)
            // ready: 1 - 2 (current)
            {
                task::Id found_id;

                bool result = scheduler::getNextTask(
                    context->m_Scheduler,
                    context->m_Task,
                    found_id
                );

                REQUIRE( true == result);
                REQUIRE( static_cast< task::Id>( 2U) == found_id);
            }

            // Check tasks states
            {
                using namespace kernel::internal;
                REQUIRE( kernel::task::State::Waiting == task::state::get( context->m_Task, context->m_TaskHandles.at( 0U)));
                REQUIRE( kernel::task::State::Ready ==   task::state::get( context->m_Task, context->m_TaskHandles.at( 1U)));
                REQUIRE( kernel::task::State::Running == task::state::get( context->m_Task, context->m_TaskHandles.at( 2U)));
            }

            // Check next task ID.
            // Expected: Next task should be Task 1.
            // Tasks queue:
            // wait: 0 (sleep)
            // ready: 1 (current) - 2

            {
                task::Id found_id;

                bool result = scheduler::getNextTask(
                    context->m_Scheduler,
                    context->m_Task,
                    found_id
                );

                REQUIRE( true == result);
                REQUIRE( static_cast< task::Id>( 1U) == found_id);
            }

            // Check tasks states
            {
                using namespace kernel::internal;
                REQUIRE( kernel::task::State::Waiting == task::state::get( context->m_Task, context->m_TaskHandles.at( 0U)));
                REQUIRE( kernel::task::State::Running == task::state::get( context->m_Task, context->m_TaskHandles.at( 1U)));
                REQUIRE( kernel::task::State::Ready ==   task::state::get( context->m_Task, context->m_TaskHandles.at( 2U)));
            }

            // Check wait condition to wake up Task 0.
            {
                kernel::TimeMs new_time = current + timeout;

                checkWaitConditions(
                    context->m_Scheduler,
                    context->m_Task,
                    context->m_Timer,
                    context->m_Event,
                    context->m_Queue,
                    new_time
                );
            }
            
            // At this point Task 0 should be Ready again, but
            // scheduler continues with next task in queue.
            // Expected: Next task should be Task 2
            // Tasks queue:
            // ready: 0 - 1 - 2 (current)
            {
                task::Id found_id;

                bool result = scheduler::getNextTask(
                    context->m_Scheduler,
                    context->m_Task,
                    found_id
                );

                REQUIRE( true == result);
                REQUIRE( static_cast< task::Id>( 2U) == found_id);
            }

            // Check tasks states
            {
                using namespace kernel::internal;
                REQUIRE( kernel::task::State::Ready ==   task::state::get( context->m_Task, context->m_TaskHandles.at( 0U)));
                REQUIRE( kernel::task::State::Ready ==   task::state::get( context->m_Task, context->m_TaskHandles.at( 1U)));
                REQUIRE( kernel::task::State::Running == task::state::get( context->m_Task, context->m_TaskHandles.at( 2U)));
            }

            // Expected: Task 0 should be scheduled.
            // Tasks queue:
            // ready: 0 (current) - 1 - 2
            {
                task::Id found_id;

                bool result = scheduler::getNextTask(
                    context->m_Scheduler,
                    context->m_Task,
                    found_id
                );

                REQUIRE( true == result);
                REQUIRE(static_cast< task::Id>( 0U) == found_id);
            }

            // Check tasks states.
            {
                using namespace kernel::internal;
                REQUIRE( kernel::task::State::Running == task::state::get( context->m_Task, context->m_TaskHandles.at( 0U)));
                REQUIRE( kernel::task::State::Ready ==   task::state::get( context->m_Task, context->m_TaskHandles.at( 1U)));
                REQUIRE( kernel::task::State::Ready ==   task::state::get( context->m_Task, context->m_TaskHandles.at( 2U)));
            }
        }
    }

    SECTION( "Create 3 tasks and set two to wait for synchronization objects: event and timer.")
    {
        std::unique_ptr<test_case_context> context(new test_case_context);

        kernel::Handle event;
        kernel::Handle timer;

        const kernel::TimeMs timeout = 100U;
        const kernel::TimeMs current = 100U;

        // Pre-condition
        {
            // Create 3 LOW priority tasks.
            context->allocate_tasks( kernel::task::Priority::Low, 3U);

            for (uint32_t i = 0U; i < 3U; ++i)
            {
                // Add newly created task IDs to scheduler.
                bool result = scheduler::addReadyTask(
                    context->m_Scheduler,
                    context->m_Task,
                    context->m_TaskHandles.at( i)
                );
                REQUIRE( true == result);
            }
        }

        // Check tasks states.
        {
            using namespace kernel::internal;
            REQUIRE( kernel::task::State::Ready == task::state::get( context->m_Task, context->m_TaskHandles.at( 0U)));
            REQUIRE( kernel::task::State::Ready == task::state::get( context->m_Task, context->m_TaskHandles.at( 1U)));
            REQUIRE( kernel::task::State::Ready == task::state::get( context->m_Task, context->m_TaskHandles.at( 2U)));
        }

        // Set Task 1 to wait for event object.
        {
            // Create event.
            {
                kernel::internal::event::Id new_event_id;
                bool result = kernel::internal::event::create( context->m_Event, new_event_id, true, nullptr);
                
                REQUIRE( true == result);

                event = kernel::internal::handle::create(
                    kernel::internal::handle::ObjectType::Event,
                    new_event_id
                );
            }

            // Set task to wait for new event.
            {
                kernel::TimeMs unused_ref = 0U;
                const bool wait_for_all_signals = true;
                bool wait_forever = true;

                task::Id task_to_wait = context->m_TaskHandles.at( 1U);

                setTaskToWaitForObj(
                    context->m_Scheduler,
                    context->m_Task,
                    task_to_wait,
                    &event,
                    1U,
                    wait_for_all_signals,
                    wait_forever,
                    unused_ref,
                    unused_ref
                );
            }
        }

        // Check tasks states.
        {
            using namespace kernel::internal;
            REQUIRE( kernel::task::State::Ready ==   task::state::get( context->m_Task, context->m_TaskHandles.at( 0U)));
            REQUIRE( kernel::task::State::Waiting == task::state::get( context->m_Task, context->m_TaskHandles.at( 1U)));
            REQUIRE( kernel::task::State::Ready ==   task::state::get( context->m_Task, context->m_TaskHandles.at( 2U)));
        }

        // Set Task 2 to wait for timer.
        {
            // Create and start timer object.
            {
                kernel::internal::timer::Id new_timer_id;
                kernel::TimeMs start = 0U;
                kernel::TimeMs interval = 10000U;

                bool result = kernel::internal::timer::create(
                    context->m_Timer,
                    new_timer_id,
                    start,
                    interval
                );

                REQUIRE( true == result);

                kernel::internal::timer::start(
                    context->m_Timer,
                    new_timer_id);

                timer = kernel::internal::handle::create(
                    kernel::internal::handle::ObjectType::Timer,
                    new_timer_id
                );
            }

            // Set task to wait for new timer.
            {
                kernel::TimeMs unused_ref = 0U;
                const bool wait_for_all_signals = true;
                bool wait_forever = true;

                task::Id task_to_wait = context->m_TaskHandles.at( 2U);

                setTaskToWaitForObj(
                    context->m_Scheduler,
                    context->m_Task,
                    task_to_wait,
                    &timer,
                    1U,
                    wait_for_all_signals,
                    wait_forever,
                    unused_ref,
                    unused_ref
                );
            }
        }

        // Check tasks states.
        {
            using namespace kernel::internal;
            REQUIRE( kernel::task::State::Ready ==   task::state::get( context->m_Task, context->m_TaskHandles.at( 0U)));
            REQUIRE( kernel::task::State::Waiting == task::state::get( context->m_Task, context->m_TaskHandles.at( 1U)));
            REQUIRE( kernel::task::State::Waiting == task::state::get( context->m_Task, context->m_TaskHandles.at( 2U)));
        }

        // Check synchronization objects conditions when not conditions are not met.
        {
            kernel::TimeMs currentTime = 10U;

            checkWaitConditions(
                context->m_Scheduler,
                context->m_Task,
                context->m_Timer,
                context->m_Event,
                context->m_Queue,
                currentTime
                );
        }

        // Check tasks states.
        // Expected: Object are still in Waiting state.
        {
            using namespace kernel::internal;
            REQUIRE( kernel::task::State::Ready ==   task::state::get( context->m_Task, context->m_TaskHandles.at( 0U)));
            REQUIRE( kernel::task::State::Waiting == task::state::get( context->m_Task, context->m_TaskHandles.at( 1U)));
            REQUIRE( kernel::task::State::Waiting == task::state::get( context->m_Task, context->m_TaskHandles.at( 2U)));
        }

        // Set synchronizatin object to meet condition.
        // Set event.
        {
            auto id = kernel::internal::handle::getId<kernel::internal::event::Id>( event);
            kernel::internal::event::set( context->m_Event, id);
        }

        // Check conditions.
        {
            kernel::TimeMs currentTime = 11U;

            checkWaitConditions(
                context->m_Scheduler,
                context->m_Task,
                context->m_Timer,
                context->m_Event,
                context->m_Queue,
                currentTime
            );
        }

        // Check tasks states.
        // Expected: Task 1 should wake up and set state to Ready.
        {
            using namespace kernel::internal;
            REQUIRE( kernel::task::State::Ready ==   task::state::get( context->m_Task, context->m_TaskHandles.at( 0U)));
            REQUIRE( kernel::task::State::Ready ==   task::state::get( context->m_Task, context->m_TaskHandles.at( 1U)));
            REQUIRE( kernel::task::State::Waiting == task::state::get( context->m_Task, context->m_TaskHandles.at( 2U)));
        }

        // Set synchronizatin object to meet condition.
        // Timer should be set to Finished state.
        {
            // Timer start value was 0, interval 10000U,
            // set new value higher than expected.
            kernel::TimeMs currentTime = 10001U;

            kernel::internal::timer::tick( context->m_Timer, currentTime);
        }

        // Check conditions.
        {
            kernel::TimeMs currentTime = 11U;

            checkWaitConditions(
                context->m_Scheduler,
                context->m_Task,
                context->m_Timer,
                context->m_Event,
                context->m_Queue,
                currentTime
            );
        }

        // Check tasks states.
        // Expected: Task 2 should wake up and set state to Ready.
        {
            using namespace kernel::internal;
            REQUIRE( kernel::task::State::Ready == task::state::get( context->m_Task, context->m_TaskHandles.at( 0U)));
            REQUIRE( kernel::task::State::Ready == task::state::get( context->m_Task, context->m_TaskHandles.at( 1U)));
            REQUIRE( kernel::task::State::Ready == task::state::get( context->m_Task, context->m_TaskHandles.at( 2U)));
        }
    }
}
