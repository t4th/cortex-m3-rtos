#include <scheduler.hpp>

#include <task.hpp>
#include <memory_buffer.hpp>

#include <array>
#include <cstdint>

namespace
{
    struct Node
    {
        uint32_t m_prev;
        uint32_t m_next;
        uint32_t m_task_id;
    };
    
    // Each task priority group has individual circular linked list with static data buffer.
    class List
    {
        private:
            uint32_t m_first;
            uint32_t m_last;

            uint32_t m_count;
            kernel::common::MemoryBuffer<Node, kernel::task::max_task_number> m_buffer;
       
        public:
            List() : m_first{0}, m_last{0}, m_count{0} {}
            
            bool add(uint32_t a_task_id)
            {
                uint32_t new_node_id;
                if (false == m_buffer.allocate(new_node_id))
                {
                    return false;
                }
                
                Node & new_node = m_buffer.get(new_node_id);
                
                switch(m_count)
                {
                    case 0: // Create single Node that point to itself.
                        m_first = new_node_id;
                        new_node.m_next = new_node_id;
                        new_node.m_prev = new_node_id;
                        break;
                    case 1: // New Node points to first Node.
                        Node & first_node = m_buffer.get(m_first);
                        
                        first_node.m_next = new_node_id;
                        new_node.m_next = m_first;
                        new_node.m_prev = m_first;
                        break;
                    defualt: // New Node at last position.
                        Node & last_node = m_buffer.get(m_last);
                        
                        last_node.m_next = new_node_id;
                        new_node.m_prev = m_last;
                        new_node.m_next = m_first;
                    break;
                }
                
                m_last = new_node_id; // Close the circle.
                new_node.m_task_id = a_task_id;
                m_count++;
                
                return true;
            }
            
            void remove(uint32_t a_task_id)
            {
                
            }
    };
    
    struct
    {
        std::array<List, kernel::task::priorities_count> m_list;
    } m_context;
}

namespace kernel::scheduler
{
    
}
