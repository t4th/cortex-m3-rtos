#include "catch.hpp"

#include "queue/queue.hpp"

TEST_CASE( "Queue")
{
    SECTION ("Create new queue, add and remove items.")
    {
        constexpr size_t Max_buffer_size{ 4U};
        kernel::static_queue::Buffer< int32_t, Max_buffer_size> buffer;

        kernel::internal::queue::Context queue_context;

        kernel::internal::queue::Id queue_id;

        // Create new queue.
        {
            size_t max_buffer_size{ Max_buffer_size};
            size_t max_type_size{ sizeof( int32_t)};

            bool queue_created = kernel::internal::queue::create(
                queue_context,
                queue_id,
                max_buffer_size,
                max_type_size,
                *reinterpret_cast< uint8_t*> ( buffer.m_data),
                nullptr
            );

            REQUIRE( true == queue_created);
            REQUIRE( true == kernel::internal::queue::isEmpty( queue_context, queue_id));
        }

        // Fill up queue with data
        for ( int i = 0; i < Max_buffer_size; ++i)
        {
            int32_t data_to_send = i;

            bool data_sent = kernel::internal::queue::send(
                queue_context,
                queue_id,
                *reinterpret_cast< uint8_t*> ( &data_to_send)
            );

            REQUIRE( true == data_sent);
            REQUIRE( data_to_send == buffer.m_data[ i]);
        }

        REQUIRE( true == kernel::internal::queue::isFull( queue_context, queue_id));

        // Try adding item to full queue.
        {
            int32_t data_to_send = 0x1234'567;

            bool data_sent = kernel::internal::queue::send(
                queue_context,
                queue_id,
                *reinterpret_cast< uint8_t*> ( &data_to_send)
            );

            REQUIRE( false == data_sent);
        }

        // Receive item from queue.
        {
            int32_t data_to_receive = 0xCDCD'CDCD;

            bool data_sent = kernel::internal::queue::receive(
                queue_context,
                queue_id,
                *reinterpret_cast< uint8_t*> ( &data_to_receive)
            );

            REQUIRE( true == data_sent);
            REQUIRE( 0 == data_to_receive);
        }

        // Try adding item to queue.
        {
            int32_t data_to_send = 0x1234'567;

            bool data_sent = kernel::internal::queue::send(
                queue_context,
                queue_id,
                *reinterpret_cast< uint8_t*> ( &data_to_send)
            );

            REQUIRE( true == data_sent);
            REQUIRE( data_to_send == buffer.m_data[ 0]);
        }

        // Receive all items from queue.
        {
            const int32_t expected_values[ Max_buffer_size]
                { 1, 2, 3, 0x1234'567};

            for ( int i = 0; i < Max_buffer_size; ++i)
            {
                int32_t data_to_receive = 0xCDCD'CDCD;

                bool data_sent = kernel::internal::queue::receive(
                    queue_context,
                    queue_id,
                    *reinterpret_cast< uint8_t*> ( &data_to_receive)
                );

                REQUIRE( true == data_sent);
                REQUIRE( expected_values[ i] == data_to_receive);
            }
        }

        REQUIRE( true == kernel::internal::queue::isEmpty( queue_context, queue_id));
        
        // Receive item from empty queue.
        {
            int32_t data_to_receive = 0xCDCD'CDCD;

            bool data_sent = kernel::internal::queue::receive(
                queue_context,
                queue_id,
                *reinterpret_cast< uint8_t*> ( &data_to_receive)
            );

            REQUIRE( false == data_sent);
            REQUIRE( 0xCDCD'CDCD == data_to_receive);
        }

        // Destroy queue
        kernel::internal::queue::destroy(
                queue_context,
                queue_id);
    }
}
