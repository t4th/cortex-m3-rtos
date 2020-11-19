#include "catch.hpp"

#include "task.hpp"

namespace stubs
{
    void task_routine(void * param)
    {
        // empty task routine
    }

    void kernel_task_routine()
    {
    }
}

namespace kernel::hardware::task
{
    void Stack::init(uint32_t a_routine)
    {
        m_data[TASK_STACK_SIZE - 8] = 0xCD'CD'CD'CD; // R0
        m_data[TASK_STACK_SIZE - 7] = 0xCD'CD'CD'CD; // R1
        m_data[TASK_STACK_SIZE - 6] = 0xCD'CD'CD'CD; // R2
        m_data[TASK_STACK_SIZE - 5] = 0xCD'CD'CD'CD; // R3
        m_data[TASK_STACK_SIZE - 4] = 0; // R12
        m_data[TASK_STACK_SIZE - 3] = 0; // LR R14
        m_data[TASK_STACK_SIZE - 2] = a_routine;
        m_data[TASK_STACK_SIZE - 1] = 0x01000000; // xPSR
    }

    uint32_t Stack::getStackPointer()
    {
        return (uint32_t)&m_data[TASK_STACK_SIZE - 8];
    }
}

TEST_CASE("Task")
{
    SECTION ("create new task and verify context data")
    {
        kernel::internal::task::Context context{};
        kernel::internal::task::Id task_id{};
        uint32_t parameter;

        // create maximum number of tasks
        for (uint32_t i = 0U; i < kernel::internal::task::MAX_TASK_NUMBER; ++i)
        {
            bool result = kernel::internal::task::create(
                context,
                stubs::kernel_task_routine,
                stubs::task_routine,
                kernel::task::Priority::Medium,
                &task_id,
                &parameter,
                false
                );

            REQUIRE(true == result);
            REQUIRE(i == task_id.m_id); // task ID, is task index in memory buffer

            // priority
            REQUIRE(kernel::task::Priority::Medium == kernel::internal::task::priority::get(context, task_id));
            
            // state
            REQUIRE(kernel::task::State::Ready == kernel::internal::task::state::get(context, task_id));
        
            // context
            REQUIRE(&context.m_data.at(i).m_context == kernel::internal::task::context::get(context, task_id));
        
            // SP
            REQUIRE(context.m_data.at(i).m_sp == kernel::internal::task::sp::get(context, task_id));

            kernel::internal::task::sp::set(context, task_id, 0xdeadbeef);
            REQUIRE(0xdeadbeef == kernel::internal::task::sp::get(context, task_id));

            // routine
            REQUIRE(stubs::task_routine == kernel::internal::task::routine::get(context, task_id));

            // parameter
            REQUIRE(reinterpret_cast<void*>(&parameter) == kernel::internal::task::parameter::get(context, task_id));
        }

        // try overflow task buffer
        bool result = kernel::internal::task::create(
            context,
            stubs::kernel_task_routine,
            stubs::task_routine,
            kernel::task::Priority::Medium,
            &task_id,
            &parameter,
            false
        );

        REQUIRE(false == result);

        // remove 3rd task
        task_id.m_id = 3;
        kernel::internal::task::destroy(context, task_id);
        
        // allocate new task with id 3

        // try overflow task buffer
        result = kernel::internal::task::create(
            context,
            stubs::kernel_task_routine,
            stubs::task_routine,
            kernel::task::Priority::Medium,
            &task_id,
            &parameter,
            false
        );

        REQUIRE(true == result);
        REQUIRE(3 == task_id.m_id);

        // priority
        REQUIRE(kernel::task::Priority::Medium == kernel::internal::task::priority::get(context, task_id));

        // context
        REQUIRE(&context.m_data.at(3).m_context == kernel::internal::task::context::get(context, task_id));

        // SP
        REQUIRE(context.m_data.at(3).m_sp == kernel::internal::task::sp::get(context, task_id));

        kernel::internal::task::sp::set(context, task_id, 0x123);
        REQUIRE(0x123 == kernel::internal::task::sp::get(context, task_id));

        // routine
        REQUIRE(stubs::task_routine == kernel::internal::task::routine::get(context, task_id));

        // parameter
        REQUIRE(reinterpret_cast<void*>(&parameter) == kernel::internal::task::parameter::get(context, task_id));
    }

    SECTION ("create new task and modify context data with API")
    {
        kernel::internal::task::Context context{};
        kernel::internal::task::Id task_id{};
        uint32_t parameter;

        bool result = kernel::internal::task::create(
            context,
            stubs::kernel_task_routine,
            stubs::task_routine,
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