#pragma once

#include <memory_buffer.hpp>

namespace kernel::common
{
    // Each task priority group has individual circular linked list with static data buffer.
    // TDataType must be of primitive type.
    template <typename TDataType, std::size_t MaxSize>
    class CircularList
    {
        private:
            struct Node
            {
                uint32_t  m_next;
                uint32_t  m_prev;   // although prev is not used for scheduling having it makes removing an item simple
                TDataType m_data;   // NOTE: only primitive data
            };
       
        public:
            struct Context
            {
                uint32_t m_first;
                uint32_t m_last;

                uint32_t m_count;
                typename MemoryBuffer<Node, MaxSize>::Context m_bufferContext;
                MemoryBuffer<Node, MaxSize> m_buffer;

                Context() : m_first{0}, m_last{0}, m_count{0}, m_buffer(m_bufferContext) {}
            };

            CircularList(Context & a_context)
                : m_context(a_context) {}
            
            bool add(TDataType a_new_data, uint32_t & a_new_node_index)
            {
                uint32_t new_node_index;
                if (false == m_context.m_buffer.allocate(new_node_index))
                {
                    return false;
                }

                a_new_node_index = new_node_index;
                
                Node & new_node = m_context.m_buffer.at(new_node_index);
                
                switch(m_context.m_count)
                {
                    case 0: // Create single Node that point to itself.
                        {
                            m_context.m_first = new_node_index;

                            new_node.m_next = new_node_index;
                            new_node.m_prev = new_node_index;
                        }
                        break;
                    case 1: // New Node points to first Node.
                        {
                            Node & first_node = m_context.m_buffer.at(m_context.m_first);
                        
                            first_node.m_next = new_node_index;
                            first_node.m_prev = new_node_index;
                            new_node.m_next = m_context.m_first;
                            new_node.m_prev = m_context.m_first;
                        }
                        break;
                    default: // New Node at last position.
                        {
                            Node & first_node = m_context.m_buffer.at(m_context.m_first);
                            Node & last_node = m_context.m_buffer.at(m_context.m_last);
                        
                            last_node.m_next = new_node_index;
                            first_node.m_prev = new_node_index;
                            new_node.m_next = m_context.m_first;
                            new_node.m_prev = m_context.m_last;
                        }
                    break;
                }
                
                m_context.m_last = new_node_index; // Close the circle.
                new_node.m_data = a_new_data;
                m_context.m_count++;
                
                return true;
            }

            void remove(uint32_t a_node_index)
            {
                if (m_context.m_count > 0)
                {
                    if (m_context.m_count > 1)
                    {
                        const uint32_t prev = m_context.m_buffer.at(a_node_index).m_prev;
                        const uint32_t next = m_context.m_buffer.at(a_node_index).m_next;

                        m_context.m_buffer.at(prev).m_next = next;
                        m_context.m_buffer.at(next).m_prev = prev;

                        if (a_node_index == m_context.m_first) { m_context.m_first = next; }
                        else if (a_node_index == m_context.m_last) { m_context.m_last = prev; }
                    }

                    m_context.m_buffer.free(a_node_index);
                    m_context.m_count--;
                }
            }

    private:
        Context & m_context;
    };
}
