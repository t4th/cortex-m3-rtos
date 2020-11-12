#include <task.hpp>

#include <memory_buffer.hpp>

namespace kernel::internal::task
{
    bool create(
        Context &               a_context,
        TaskRoutine             a_task_routine,
        kernel::task::Routine   a_routine,
        kernel::task::Priority  a_priority,
        kernel::task::Id *      a_handle,
        bool                    a_create_suspended
        )
    {
        // Verify arguments.
        if (!a_routine)
        {
            return false;
        }
        
        // Create new Task object.
        uint32_t item_id;
        
        if (false == a_context.m_data.allocate(item_id))
        {
            return false;
        }
        
        // Initialize new Task object.
        Task & task = a_context.m_data.at(item_id);
        
        task.m_priority = a_priority;
        task.m_routine = a_routine;
        task.m_stack.init((uint32_t)a_task_routine);
        task.m_sp = task.m_stack.getStackPointer();
        
        if (a_create_suspended)
        {
            task.m_state = kernel::task::State::Suspended;
        }
        else
        {
            task.m_state = kernel::task::State::Ready;
        }
        
        if (a_handle)
        {
            a_handle->m_id = item_id;
            // TODO: is it needed in kernel or user api?
            // *a_handle = kernel::handle::create(item_id, kernel::handle::ObjectType::Task);
        }
        
        return true;
    }
    
    void destroy(
        Context &               a_context,
        kernel::task::Id        a_id
    )
    {
        a_context.m_data.free(a_id.m_id);
    }

    namespace priority
    {
        kernel::task::Priority get(
            Context &           a_context,
            kernel::task::Id    a_id
        )
        {
            return a_context.m_data.at(a_id.m_id).m_priority;
        }
    }

    namespace context
    {
        kernel::hardware::task::Context * get(
            Context &           a_context,
            kernel::task::Id    a_id
        )
        {
            // NOTE: now that context moved out of anonymous namespace this
            //       construct is just a bad practice...
            return &a_context.m_data.at(a_id.m_id).m_context;
        }
    }
    
    namespace sp
    {
        uint32_t get(
            Context &           a_context,
            kernel::task::Id    a_id
        )
        {
            return a_context.m_data.at(a_id.m_id).m_sp;
        }

        void set(
            Context &           a_context,
            kernel::task::Id    a_id,
            uint32_t            a_new_sp
        )
        {
            a_context.m_data.at(a_id.m_id).m_sp = a_new_sp;
        }
    }

    namespace routine
    {
        kernel::task::Routine get(
            Context &           a_context,
            kernel::task::Id    a_id
        )
        {
            return a_context.m_data.at(a_id.m_id).m_routine;
        }
    }
}
