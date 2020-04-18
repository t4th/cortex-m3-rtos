#include <scheduler.hpp>

#include <task.hpp>
#include <circular_list.hpp>

#include <array>
#include <cstdint>

namespace
{

    
    struct
    {
        std::array<
            kernel::common::CircularList<uint32_t, kernel::task::max_task_number>,
            kernel::task::priorities_count>
        m_list;
        
    } m_context;
}

namespace kernel::scheduler
{
    void test()
    {
        m_context.m_list.at(0).remove(23);
    }
}
