#pragma once

#include "common/memory_buffer.hpp"

// Circular linked list with statically allocated fixed number of nodes.
// In context of kernel, it is only used internally by scheduler.
namespace kernel::internal::common
{
    // TDataType must be of primitive type.
    template < typename TDataType, std::size_t MaxSize>
    class CircularList
    {
        public:
            // Type strong index of Node.
            enum class Id : uint32_t{};

        private:
            struct Node
            {
                uint32_t  m_next{ 0U};
                uint32_t  m_prev{ 0U};  // although prev is not used for scheduling having it makes removing an item simple
                TDataType m_data{};     // NOTE: only primitive data
            };
       
            // Type strong memory index for allocated Task type.
            typedef typename common::MemoryBuffer< Node, MaxSize>::Id MemoryBufferIndex;

        public:
            bool add( TDataType a_new_data, Id & a_new_node_index) volatile
            {
                MemoryBufferIndex new_node_index;

                if ( false == m_buffer.allocate( new_node_index))
                {
                    return false;
                }

                a_new_node_index = static_cast< Id>( new_node_index);
                
                volatile Node & new_node = m_buffer.at( new_node_index);
                
                switch( m_count)
                {
                    case 0U: // Create single Node that point to itself.
                        {
                            m_first = static_cast< uint32_t>( new_node_index);

                            new_node.m_next = static_cast< uint32_t>( new_node_index);
                            new_node.m_prev = static_cast< uint32_t>( new_node_index);
                        }
                        break;
                    case 1U: // New Node points to first Node.
                        {
                            volatile Node & first_node = m_buffer.at( static_cast< MemoryBufferIndex>( m_first));
                        
                            first_node.m_next = static_cast< uint32_t>( new_node_index);
                            first_node.m_prev = static_cast< uint32_t>( new_node_index);
                            new_node.m_next = m_first;
                            new_node.m_prev = m_first;
                        }
                        break;
                    default: // New Node at last position.
                        {
                            volatile Node & first_node = m_buffer.at( static_cast< MemoryBufferIndex>( m_first));
                            volatile Node & last_node = m_buffer.at( static_cast< MemoryBufferIndex>( m_last));
                        
                            last_node.m_next = static_cast< uint32_t>( new_node_index);
                            first_node.m_prev = static_cast< uint32_t>( new_node_index);
                            new_node.m_next = m_first;
                            new_node.m_prev = m_last;
                        }
                    break;
                }
                
                m_last = static_cast< uint32_t>( new_node_index); // Close the linked list.
                new_node.m_data = a_new_data;
                ++m_count;
                
                return true;
            }

            void remove( Id a_node_index) volatile
            {
                if ( m_count > 0U)
                {
                    if ( m_count > 1U)
                    {
                        const uint32_t prev = m_buffer.at( static_cast< MemoryBufferIndex>( a_node_index)).m_prev;
                        const uint32_t next = m_buffer.at( static_cast< MemoryBufferIndex>( a_node_index)).m_next;

                        m_buffer.at( static_cast< MemoryBufferIndex>( prev)).m_next = next;
                        m_buffer.at( static_cast< MemoryBufferIndex>( next)).m_prev = prev;

                        if ( static_cast< uint32_t>( a_node_index) == m_first)
                        {
                            m_first = next;
                        }
                        else if ( static_cast< uint32_t>( a_node_index) == m_last)
                        {
                            m_last = prev;
                        }
                    }

                    m_buffer.free( static_cast< MemoryBufferIndex>( a_node_index));
                    --m_count;
                }
            }

            bool find( TDataType & a_key, Id & a_found_index) volatile
            {
                uint32_t node_index = m_first;

                for ( uint32_t i = 0U; i < m_count; ++i)
                {
                    if ( a_key == m_buffer.at( static_cast< MemoryBufferIndex>( node_index)).m_data)
                    {
                        a_found_index = static_cast< Id>( node_index);
                        return true;
                    }
                    else
                    {
                        node_index = m_buffer.at( static_cast< MemoryBufferIndex>( node_index)).m_next;
                    }
                }

                return false;
            }

            volatile TDataType & at( volatile Id a_node_index) volatile
            {
                return m_buffer.at( static_cast< MemoryBufferIndex>( a_node_index)).m_data;
            }

            Id firstIndex() volatile
            {
                return static_cast< Id>( m_first);
            }

            Id nextIndex( Id a_node_index) volatile
            {
                return static_cast< Id>( m_buffer.at( static_cast< MemoryBufferIndex>( a_node_index)).m_next);
            }
            
            uint32_t count() volatile
            {
                return m_count;
            }

    private:
        MemoryBuffer< Node, MaxSize> m_buffer{};
        uint32_t m_first{ 0U};
        uint32_t m_last{ 0U};
        uint32_t m_count{ 0U};
    };
}
