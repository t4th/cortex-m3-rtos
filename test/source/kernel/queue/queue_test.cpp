#include "catch.hpp"

#include "queue/queue.hpp"

TEST_CASE( "Queue")
{
    SECTION ("Create new queue and add some data.")
    {
        constexpr size_t Max_buffer_size{ 4U};
        kernel::static_queue::Buffer< int32_t, Max_buffer_size> buffer;

        kernel::internal::event::Context event_context;
        kernel::internal::queue::Context queue_context;

        kernel::internal::queue::Id queue_id;

        size_t max_buffer_size{ Max_buffer_size};
        size_t max_type_size{ sizeof( int32_t)};

        bool queue_created = kernel::internal::queue::create(
            queue_context,
            event_context,
            queue_id,
            max_buffer_size,
            max_type_size,
            *reinterpret_cast< uint8_t*> ( buffer.m_data)
        );

        REQUIRE( true == queue_created);

        // Put some data on the queue.
        {
            int32_t data_to_send = 0x1234'5678;

            bool data_sent = kernel::internal::queue::send(
                queue_context,
                event_context,
                queue_id,
                *reinterpret_cast< uint8_t*> ( &data_to_send)
            );

            REQUIRE( true == data_sent);
            REQUIRE( data_to_send == buffer.m_data[ 0]);
        }
        
        // Put some data on the queue.
        {
            int32_t data_to_send = 0xABAB'DEDE;

            bool data_sent = kernel::internal::queue::send(
                queue_context,
                event_context,
                queue_id,
                *reinterpret_cast< uint8_t*> ( &data_to_send)
            );

            REQUIRE( true == data_sent);
            REQUIRE( data_to_send == buffer.m_data[ 1]);
        }
    }
}
