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

namespace kernel::hardware::task
{
    void Stack::init(uint32_t a_routine) volatile
    {
    }

    uint32_t Stack::getStackPointer() volatile
    {
        return 0U;
    }
}

TEST_CASE("Task")
{
    SECTION ("Create new task and verify context data.")
    {
        std::unique_ptr<kernel::internal::task::Context> m_heap_context(new kernel::internal::task::Context);
        kernel::internal::task::Context & context = *m_heap_context;

        kernel::internal::task::Id task_id{};
        uint32_t parameter;

        // Create maximum number of tasks.
        for (uint32_t i = 0U; i < kernel::internal::task::MAX_NUMBER; ++i)
        {
            bool result = kernel::internal::task::create(
                context,
                kernel_task_routine,
                task_routine,
                kernel::task::Priority::Medium,
                &task_id,
                &parameter,
                false
                );

            REQUIRE(true == result);
            REQUIRE(i == task_id); // task ID, is task index in memory buffer

            // priority
            REQUIRE(kernel::task::Priority::Medium == kernel::internal::task::priority::get(context, task_id));
            
            // state
            REQUIRE(kernel::task::State::Ready == kernel::internal::task::state::get(context, task_id));
        
            // context
            REQUIRE(&context.m_data.at(i).m_context == kernel::internal::task::context::get(context, task_id));
        
            // SP
            REQUIRE(context.m_data.at(i).m_sp == kernel::internal::task::sp::get(context, task_id));

            kernel::internal::task::sp::set(context, task_id, 0xdeadbeefU);
            REQUIRE(0xdeadbeefU == kernel::internal::task::sp::get(context, task_id));

            // routine
            REQUIRE(&task_routine == kernel::internal::task::routine::get(context, task_id));

            // parameter
            REQUIRE(reinterpret_cast<void*>(&parameter) == kernel::internal::task::parameter::get(context, task_id));
        }

        // Try overflow task buffer.
        bool result = kernel::internal::task::create(
            context,
            kernel_task_routine,
            task_routine,
            kernel::task::Priority::Medium,
            &task_id,
            &parameter,
            false
        );

        REQUIRE(false == result);

        // Remove task with ID 3.
        task_id = 3U;
        kernel::internal::task::destroy(context, task_id);
        
        // Allocate new task with ID 3.
        // Expected: new task will get ID 3.
        result = kernel::internal::task::create(
            context,
            kernel_task_routine,
            task_routine,
            kernel::task::Priority::Medium,
            &task_id,
            &parameter,
            false
        );

        REQUIRE(true == result);
        REQUIRE(3U == task_id);

        // priority
        REQUIRE(kernel::task::Priority::Medium == kernel::internal::task::priority::get(context, task_id));

        // context
        REQUIRE(&context.m_data.at(3U).m_context == kernel::internal::task::context::get(context, task_id));

        // SP
        REQUIRE(context.m_data.at(3U).m_sp == kernel::internal::task::sp::get(context, task_id));

        kernel::internal::task::sp::set(context, task_id, 0x123U);
        REQUIRE(0x123U == kernel::internal::task::sp::get(context, task_id));

        // routine
        REQUIRE(&task_routine == kernel::internal::task::routine::get(context, task_id));

        // parameter
        REQUIRE(reinterpret_cast<void*>(&parameter) == kernel::internal::task::parameter::get(context, task_id));
    }

    SECTION ("Create new task and modify context data with API.")
    {
        std::unique_ptr<kernel::internal::task::Context> m_heap_context(new kernel::internal::task::Context);
        kernel::internal::task::Context & context = *m_heap_context;
        kernel::internal::task::Id task_id{};
        uint32_t parameter;

        bool result = kernel::internal::task::create(
            context,
            kernel_task_routine,
            task_routine,
            kernel::task::Priority::Medium,
            &task_id,
            &parameter,
            false
            );

            REQUIRE(true == result);

            // check task state after creating
            REQUIRE(kernel::task::State::Ready == kernel::internal::task::state::get(context, task_id));

            // change task state
            kernel::internal::task::state::set(context, task_id, kernel::task::State::Running);

            // verify if state has changed
            REQUIRE(kernel::task::State::Running == kernel::internal::task::state::get(context, task_id));
    }
}