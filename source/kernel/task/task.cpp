#include <task.hpp>

namespace kernel::internal::task
{
    bool create(
        Context &               a_context,
        TaskRoutine             a_task_routine,
        kernel::task::Routine   a_routine,
        kernel::task::Priority  a_priority,
        Id *                    a_id,
        void *                  a_parameter,
        bool                    a_create_suspended
        )
    {
        // Verify arguments.
        if (!a_routine)
        {
            return false;
        }
        
        // Create new Task object.
        uint32_t new_item_id;
        
        if (false == a_context.m_data.allocate(new_item_id))
        {
            return false;
        }
        
        // Initialize new Task object.
        volatile Task & new_task = a_context.m_data.at(new_item_id);
        
        new_task.m_priority = a_priority;
        new_task.m_routine = a_routine;
        new_task.m_stack.init(reinterpret_cast<uint32_t>(a_task_routine));
        new_task.m_sp = new_task.m_stack.getStackPointer();
        new_task.m_parameter = a_parameter;
        
        if (a_create_suspended)
        {
            new_task.m_state = kernel::task::State::Suspended;
        }
        else
        {
            new_task.m_state = kernel::task::State::Ready;
        }
        
        if (a_id)
        {
            // Task ID, is task index in memory buffer by design.
            *a_id = new_item_id;
        }
        
        return true;
    }
}
