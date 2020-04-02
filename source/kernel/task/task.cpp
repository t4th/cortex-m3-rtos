#include <task.hpp>

#include <memory_buffer.hpp>
#include <handle.hpp>
#include <hardware.hpp>

namespace
{
    constexpr uint32_t max_task_number = 16;
    constexpr uint32_t task_stack_size = 32;
    
    struct Stack
    {
        uint32_t m_data[task_stack_size];
    };
    
    struct Task
    {
        uint32_t                        sp;
        kernel::hardware::task_context  context;
        Stack                           stack;
        kernel::task::Priority          priority;
        kernel::task::State             state;
        kernel::task::Routine           routine;
    };
    
    struct
    {
        kernel::common::MemoryBuffer<Task, max_task_number> m_data;
    } m_context;
}

namespace kernel::task
{
    bool create(
        Routine     a_routube,
        Priority    a_priority,
        uint32_t *  a_handle,
        bool        a_create_suspended
        )
    {
        if (!a_routube)
        {
            return false;
        }
        
        uint32_t item_id;
        
        if (false == m_context.m_data.allocate(item_id))
        {
            return false;
        }
        
        Task & task = m_context.m_data.get(item_id);
        
        if (a_create_suspended)
        {
            task.state = kernel::task::State::Suspended;
        }
        
        if (a_handle)
        {
            *a_handle = kernel::handle::create(item_id, kernel::handle::ObjectType::task);
        }
        
        return true;
    }
}
