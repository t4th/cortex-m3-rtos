#pragma once

#include <cstdint>

namespace kernel::containers
{
    template< typename TData, size_t MaxSize>
    class StaticQueue
    {
    public:
        StaticQueue() = default;

        void reset()
        {
            m_current_size = 0U;
            m_head = 0U;
            m_tail = 0U;
        }

        bool isFull()
        {
            return ( m_current_size >= MaxSize);
        }

        bool isEmpty()
        {
            return ( 0U == m_current_size);
        }

        size_t getSize()
        {
            return m_current_size;
        }

        bool at( size_t a_index, TData & a_data)
        {
            if ( a_index > MaxSize)
            {
                return false;
            }

            size_t real_index = m_tail + a_index;
            
            if ( real_index >= MaxSize)
            {
                real_index = real_index - MaxSize;
            }

            a_data = m_data[ real_index];

            return true;
        }

        bool push( TData & a_new_item)
        {
            if ( true == isFull())
            {
                return false;
            }

            if ( false == isEmpty())
            {
                ++m_head;
            
                if ( m_head >= MaxSize)
                {
                    m_head = 0U;
                }
            }

            m_data[ m_head] = a_new_item;

            ++m_current_size;

            return true;
        }

        bool pop( TData & a_removed_item)
        {
            if ( true == isEmpty())
            {
                return false;
            }

            a_removed_item = m_data[ m_tail];

            if ( m_current_size > 1U)
            {
                ++m_tail;

                if ( m_tail >= MaxSize)
                {
                    m_tail = 0U;
                }
            }

            --m_current_size;
            
            return true;
        }

    private:
        TData       m_data[ MaxSize]{};
        size_t      m_current_size{ 0U};
        uint32_t    m_head{ 0U};
        uint32_t    m_tail{ 0U};
    };
}
