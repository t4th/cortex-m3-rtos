#pragma once

#include <memory_buffer.hpp>

// TODO: inline functions
namespace kernel::internal::common
{
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

            CircularList() : m_first{0U}, m_last{0U}, m_count{0U}, m_buffer{} {}
            
            bool add(TDataType a_new_data, uint32_t & a_new_node_index) volatile
            {
                uint32_t new_node_index;
                if (false == m_buffer.allocate(new_node_index))
                {
                    return false;
                }

                a_new_node_index = new_node_index;
                
                volatile Node & new_node = m_buffer.at(new_node_index);
                
                switch(m_count)
                {
                    case 0U: // Create single Node that point to itself.
                        {
                            m_first = new_node_index;

                            new_node.m_next = new_node_index;
                            new_node.m_prev = new_node_index;
                        }
                        break;
                    case 1U: // New Node points to first Node.
                        {
                            volatile Node & first_node = m_buffer.at(m_first);
                        
                            first_node.m_next = new_node_index;
                            first_node.m_prev = new_node_index;
                            new_node.m_next = m_first;
                            new_node.m_prev = m_first;
                        }
                        break;
                    default: // New Node at last position.
                        {
                            volatile Node & first_node = m_buffer.at(m_first);
                            volatile Node & last_node = m_buffer.at(m_last);
                        
                            last_node.m_next = new_node_index;
                            first_node.m_prev = new_node_index;
                            new_node.m_next = m_first;
                            new_node.m_prev = m_last;
                        }
                    break;
                }
                
                m_last = new_node_index; // Close the linked list.
                new_node.m_data = a_new_data;
                ++m_count;
                
                return true;
            }

            void remove(uint32_t a_node_index) volatile
            {
                if (m_count > 0U)
                {
                    if (m_count > 1U)
                    {
                        const uint32_t prev = m_buffer.at(a_node_index).m_prev;
                        const uint32_t next = m_buffer.at(a_node_index).m_next;

                        m_buffer.at(prev).m_next = next;
                        m_buffer.at(next).m_prev = prev;

                        if (a_node_index == m_first) { m_first = next; }
                        else if (a_node_index == m_last) { m_last = prev; }
                    }

                    m_buffer.free(a_node_index);
                    --m_count;
                }
            }

            // TODO: Check if function is used only by POD. If yes -> remove a_compare.
            bool find(
                TDataType & a_key,
                uint32_t &  a_found_index,
                bool        a_compare(TDataType &, volatile TDataType &)) volatile
            {
                uint32_t node_index = m_first;

                for (uint32_t i = 0U; i < m_count; ++i)
                {
                    if (a_compare(a_key, m_buffer.at(node_index).m_data))
                    {
                        a_found_index = node_index;
                        return true;
                    }
                    else
                    {
                        node_index = m_buffer.at(node_index).m_next;
                    }
                }

                return false;
            }

            volatile TDataType & at(volatile uint32_t a_node_index) volatile
            {
                return m_buffer.at(a_node_index).m_data;
            }

            uint32_t firstIndex() volatile
            {
                return m_first;
            }

            uint32_t nextIndex(uint32_t a_node_index) volatile
            {
                return m_buffer.at(a_node_index).m_next;
            }
            
            uint32_t count() volatile
            {
                return m_count;
            }

    private:
        MemoryBuffer<Node, MaxSize> m_buffer;
        uint32_t m_first;
        uint32_t m_last;
        uint32_t m_count;
    };
}
