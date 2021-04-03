#include "catch.hpp"

#include "task.hpp"

namespace
{
    void task_routine(void * param)
    {
    }

    void kernel_task_routine()
    {
    }
}

namespace kernel::internal::hardware::task
{
    void Stack::init( uint32_t a_routine_address) volatile
    {
    }

    uint32_t Stack::getStackPointer() volatile
    {
        return 0U;
    }
}

TEST_CASE( "Task")
{
    SECTION ( "Create new task and verify context data.")
    {
        using namespace kernel::internal;

        std::unique_ptr< task::Context> m_heap_context( new task::Context);
        task::Context & context = *m_heap_context;

        task::Id task_id{};
        uint32_t parameter;

        // Create maximum number of tasks.
        for ( uint32_t i = 0U; i < task::max_number; ++i)
        {
            bool result = task::create(
                context,
                kernel_task_routine,
                task_routine,
                kernel::task::Priority::Medium,
                &task_id,
                &parameter,
                false
                );

            REQUIRE( true == result);
            REQUIRE( static_cast< task::Id>( i) == task_id); // task ID, is task index in memory buffer

            // priority
            REQUIRE( kernel::task::Priority::Medium == task::priority::get( context, task_id));
            
            // state
            REQUIRE( kernel::task::State::Ready == task::state::get( context, task_id));
        
            // context
            REQUIRE( &context.m_data.at( static_cast< task::MemoryBufferIndex>( i)).m_context == task::context::get( context, task_id));
        
            // SP
            REQUIRE( context.m_data.at( static_cast< task::MemoryBufferIndex>( i)).m_sp == task::sp::get( context, task_id));

            task::sp::set( context, task_id, 0xdeadbeefU);
            REQUIRE( 0xdeadbeefU == task::sp::get( context, task_id));

            // routine
            REQUIRE( &task_routine == task::routine::get( context, task_id));

            // parameter
            REQUIRE( reinterpret_cast< void*>( &parameter) == task::parameter::get( context, task_id));
        }

        // Try overflow task buffer.
        bool result = task::create(
            context,
            kernel_task_routine,
            task_routine,
            kernel::task::Priority::Medium,
            &task_id,
            &parameter,
            false
        );

        REQUIRE( false == result);

        // Remove task with ID 3.
        task_id = static_cast< task::Id>( 3U);
        task::destroy(context, task_id);
        
        // Allocate new task with ID 3.
        // Expected: new task will get ID 3.
        result = task::create(
            context,
            kernel_task_routine,
            task_routine,
            kernel::task::Priority::Medium,
            &task_id,
            &parameter,
            false
        );

        REQUIRE( true == result);
        REQUIRE( static_cast< task::Id>( 3U) == task_id);

        // priority
        REQUIRE( kernel::task::Priority::Medium == task::priority::get( context, task_id));

        // context
        REQUIRE( &context.m_data.at( static_cast< task::MemoryBufferIndex>( 3U)).m_context == task::context::get( context, task_id));

        // SP
        REQUIRE( context.m_data.at( static_cast< task::MemoryBufferIndex>( 3U)).m_sp == task::sp::get( context, task_id));

        task::sp::set( context, task_id, 0x123U);

        REQUIRE( 0x123U == task::sp::get(context, task_id));

        // routine
        REQUIRE( &task_routine == task::routine::get(context, task_id));

        // parameter
        REQUIRE( reinterpret_cast< void*>( &parameter) == task::parameter::get( context, task_id));
    }

    SECTION ( "Create new task and modify context data with API.")
    {
        using namespace kernel::internal;

        std::unique_ptr< task::Context> m_heap_context( new task::Context);
        task::Context & context = *m_heap_context;
        task::Id task_id{};
        uint32_t parameter;

        bool result = task::create(
            context,
            kernel_task_routine,
            task_routine,
            kernel::task::Priority::Medium,
            &task_id,
            &parameter,
            false
            );

            REQUIRE( true == result);

            // check task state after creating
            REQUIRE( kernel::task::State::Ready == task::state::get( context, task_id));

            // change task state
            task::state::set( context, task_id, kernel::task::State::Running);

            // verify if state has changed
            REQUIRE( kernel::task::State::Running == task::state::get( context, task_id));
    }
}