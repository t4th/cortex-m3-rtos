#include <task.hpp>

#include <memory_buffer.hpp>

namespace
{
    // Task system object representation.
    struct Task
    {
        uint32_t                        m_sp;
        kernel::hardware::task::Context m_context;
        kernel::hardware::task::Stack   m_stack;
        kernel::task::Priority          m_priority;
        kernel::task::State             m_state;
        kernel::task::Routine           m_routine;
    };
    
    struct Context
    {
        kernel::common::MemoryBuffer<Task, kernel::task::MAX_TASK_NUMBER>::Context m_context;
        kernel::common::MemoryBuffer<Task, kernel::task::MAX_TASK_NUMBER> m_data;
        
        Context() : m_data(m_context) {}
    } m_context;
}

namespace kernel::task
{
    bool create(
        Routine     a_routine,
        Priority    a_priority,
        Id *        a_handle,
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
        Task & task = m_context.m_data.at(item_id);
        
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
            *a_handle = item_id;
            // TODO: is it needed in kernel or user api?
            // *a_handle = kernel::handle::create(item_id, kernel::handle::ObjectType::Task);
        }
        
        return true;
    }
    

    namespace priority
    {
        Priority get(Id a_id)
        {
            return m_context.m_data.at(a_id).m_priority;
        }
    }

    namespace context
    {
        kernel::hardware::task::Context * getRef(Id a_id)
        {
            return &m_context.m_data.at(a_id).m_context;
        }
    }
    
    namespace sp
    {
        uint32_t get(Id a_id)
        {
            return m_context.m_data.at(a_id).m_sp;
        }
        void set(Id a_id, uint32_t a_new_sp)
        {
            m_context.m_data.at(a_id).m_sp = a_new_sp;
        }
    }
}
