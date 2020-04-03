#include <task.hpp>

#include <memory_buffer.hpp>
#include <handle.hpp>
#include <hardware.hpp>

namespace
{
    constexpr uint32_t max_task_number = 16;
    
    struct Task
    {
        uint32_t                        m_sp;
        kernel::hardware::task::Context m_context;
        kernel::hardware::task::Stack   m_stack;
        kernel::task::Priority          m_priority;
        kernel::task::State             m_state;
        kernel::task::Routine           m_routine;
    };
    
    struct
    {
        kernel::common::MemoryBuffer<Task, max_task_number> m_data;
    } m_context;
}

namespace kernel::task
{
    bool create(
        Routine     a_routine,
        Priority    a_priority,
        uint32_t *  a_handle,
        bool        a_create_suspended
        )
    {
        // Verify arguments.
        if (!a_routine)
        {
            return false;
        }
        
        // Create new Task object.
        uint32_t item_id;
        
        if (false == m_context.m_data.allocate(item_id))
        {
            return false;
        }
        
        // Initialize new Task object.
        Task & task = m_context.m_data.get(item_id);
        
        task.m_priority = a_priority;
        task.m_routine = a_routine;
        task.m_stack.init((uint32_t)task.m_routine);
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
            *a_handle = kernel::handle::create(item_id, kernel::handle::ObjectType::Task);
        }
        
        return true;
    }
}
