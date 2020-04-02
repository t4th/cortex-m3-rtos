#include <task.hpp>

#include <memory_buffer.hpp>

namespace
{
    struct task_info
    {
        uint32_t a;
        uint32_t b;
        uint32_t c;
    };
    
    struct
    {
        kernel::common::MemoryBuffer<task_info, 64> m_data;
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
        uint32_t item_id;
        
        m_context.m_data.allocate(item_id);
        
        return false;
    }
}
