#pragma once

#include <memory_buffer.hpp>

namespace kernel::common
{
    // Each task priority group has individual circular linked list with static data buffer.
    // TDataType must be of primitive type.
    template <typename TDataType, std::size_t MaxSize>
    class CircularList
    {
        protected:
            struct Node
            {
                uint32_t  m_next;
                TDataType m_data;
            };
    
            uint32_t m_first;
            uint32_t m_last;

            uint32_t m_count;
            kernel::common::MemoryBuffer<Node, MaxSize> m_buffer;
       
        public:
            CircularList() : m_first{0}, m_last{0}, m_count{0} {}
            
            bool add(TDataType a_new_data)
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
                        {
                            m_first = new_node_id;
                            new_node.m_next = new_node_id;
                        }
                        break;
                    case 1: // New Node points to first Node.
                        {
                            Node & first_node = m_buffer.get(m_first);
                        
                            first_node.m_next = new_node_id;
                            new_node.m_next = m_first;
                        }
                        break;
                    default: // New Node at last position.
                        {
                            Node & last_node = m_buffer.get(m_last);
                        
                            last_node.m_next = new_node_id;
                            new_node.m_next = m_first;
                        }
                    break;
                }
                
                m_last = new_node_id; // Close the circle.
                new_node.m_data = a_new_data;
                m_count++;
                
                return true;
            }
            
            void remove(uint32_t a_data)
            {
                if (0 == m_count)
                {
                    return;
                }
                else
                {
                    // find data in linked list
                    uint32_t current_index = m_first;
                    uint32_t previous_index = m_first;

                    for (uint32_t i = 0; i < m_count; ++i)
                    {
                        Node * current_node = & (m_buffer.get(current_index));
                        Node * previous_node = & (m_buffer.get(previous_index));
                    
                        if (current_node->m_data == a_data)
                        {
                            //m_first is the one
                            if (m_first == previous_index)
                            {
                                m_first = current_index;
                            }

                            if (m_last == current_index)
                            {
                                m_last = previous_index;
                            }

                            previous_node->m_next = current_node->m_next;

                            m_buffer.free(current_index);
                            m_count--;
                            break;
                        }

                        previous_index = current_index;
                        current_index = current_node->m_next;
                    }
                }
            }
    };
}
