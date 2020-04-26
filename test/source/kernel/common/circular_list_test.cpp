#include "catch.hpp"

#include "circular_list.hpp"

TEST_CASE("CircularList")
{
    kernel::common::CircularList<uint32_t, 3>::Context context;
    kernel::common::CircularList<uint32_t, 3> list(context);

    REQUIRE(0 == context.m_count);

    /*
     * Expected result
     * 
     *       0                  1                  2
     * +-----+-----+      +-----+-----+      +-----+-----+
     * |prev | next|      |prev | next|      |prev | next|
     * +-----------+      +-----------+      +-----------+
     * |  2  |  1  |      |  0  |  2  |      |  1  |  0  |
     * |     |     |      |     |     |      |     |     |
     * +-----+-----+      +-----+-----+      +-----+-----+
     *
     */
    SECTION( "Add new items and see if m_next field is updated correctly")
    {
        uint32_t new_node_index = 0;

        REQUIRE(true == list.add(0xabc, new_node_index));
        {
            REQUIRE(0 == context.m_first);
            REQUIRE(0 == context.m_last);
            REQUIRE(1 == context.m_count);
            REQUIRE(0xabc == context.m_buffer.at(new_node_index).m_data);

            REQUIRE(0 == context.m_buffer.at(new_node_index).m_prev);
            REQUIRE(0 == context.m_buffer.at(new_node_index).m_next);
        }

        REQUIRE(true == list.add(0x123, new_node_index));
        {
            REQUIRE(0 == context.m_first);
            REQUIRE(1 == context.m_last);
            REQUIRE(2 == context.m_count);
            REQUIRE(0x123 == context.m_buffer.at(new_node_index).m_data);

            REQUIRE(1 == context.m_buffer.at(0).m_prev);
            REQUIRE(1 == context.m_buffer.at(0).m_next);

            REQUIRE(0 == context.m_buffer.at(new_node_index).m_prev);
            REQUIRE(0 == context.m_buffer.at(new_node_index).m_next);
        }

        REQUIRE(true == list.add(0xbaba, new_node_index));
        {
            REQUIRE(0 == context.m_first);
            REQUIRE(2 == context.m_last);
            REQUIRE(3 == context.m_count);
            REQUIRE(0xbaba == context.m_buffer.at(new_node_index).m_data);

            REQUIRE(2 == context.m_buffer.at(0).m_prev);
            REQUIRE(1 == context.m_buffer.at(0).m_next);

            REQUIRE(0 == context.m_buffer.at(1).m_prev);
            REQUIRE(2 == context.m_buffer.at(1).m_next);

            REQUIRE(1 == context.m_buffer.at(2).m_prev);
            REQUIRE(0 == context.m_buffer.at(2).m_next);
        }

        SECTION( "Try to overflow")
        {
            REQUIRE(false == list.add(0xFF34, new_node_index));
        }

        /*
         * Expected result
         * 
         *       0              (removed)              2
         * +-----+-----+      +-----+-----+      +-----+-----+
         * |prev | next|      |prev | next|      |prev | next|
         * +-----------+      +-----------+      +-----------+
         * |  2  |  2  |      |     |     |      |  0  |  0  |
         * |     |     |      |     |     |      |     |     |
         * +-----+-----+      +-----+-----+      +-----+-----+
         *
         */
        SECTION( "Remove item in the middle")
        {
            list.remove(1);
            {
                REQUIRE(0 == context.m_first);
                REQUIRE(2 == context.m_last);
                REQUIRE(2 == context.m_count);

                REQUIRE(2 == context.m_buffer.at(0).m_prev);
                REQUIRE(2 == context.m_buffer.at(0).m_next);
                
                REQUIRE(0 == context.m_buffer.at(2).m_prev);
                REQUIRE(0 == context.m_buffer.at(2).m_next);
            }

            /*
             * Expected result
             * 
             *       0                  2                  1
             * +-----+-----+      +-----+-----+      +-----+-----+
             * |prev | next|      |prev | next|      |prev | next|
             * +-----------+      +-----------+      +-----------+
             * |  1  |  2  |      |  0  |  1  |      |  2  |  0  |
             * |     |     |      |     |     |      |     |     |
             * +-----+-----+      +-----+-----+      +-----+-----+
             *
             */
            SECTION( "Add item after removing")
            {
                REQUIRE(true == list.add(0xb33f, new_node_index));
                {
                    REQUIRE(0 == context.m_first);
                    REQUIRE(1 == context.m_last);
                    REQUIRE(3 == context.m_count);
                    REQUIRE(0xb33f == context.m_buffer.at(new_node_index).m_data);

                    REQUIRE(1 == context.m_buffer.at(0).m_prev);
                    REQUIRE(2 == context.m_buffer.at(0).m_next);

                    REQUIRE(0 == context.m_buffer.at(2).m_prev);
                    REQUIRE(1 == context.m_buffer.at(2).m_next);

                    REQUIRE(2 == context.m_buffer.at(1).m_prev);
                    REQUIRE(0 == context.m_buffer.at(1).m_next);
                }
            }
        }

        /*
         * Expected result
         * 
         *   (removed)              1                  2
         * +-----+-----+      +-----+-----+      +-----+-----+
         * |prev | next|      |prev | next|      |prev | next|
         * +-----------+      +-----------+      +-----------+
         * |     |     |      |  2  |  2  |      |  1  |  1  |
         * |     |     |      |     |     |      |     |     |
         * +-----+-----+      +-----+-----+      +-----+-----+
         *
         */
        SECTION( "Remove first item")
        {
            list.remove(0);
            {
                REQUIRE(1 == context.m_first);
                REQUIRE(2 == context.m_last);
                REQUIRE(2 == context.m_count);

                REQUIRE(2 == context.m_buffer.at(1).m_prev);
                REQUIRE(2 == context.m_buffer.at(1).m_next);

                REQUIRE(1 == context.m_buffer.at(2).m_prev);
                REQUIRE(1 == context.m_buffer.at(2).m_next);
            }

            /*
             * Expected result
             * 
             *       1                  2                  0
             * +-----+-----+      +-----+-----+      +-----+-----+
             * |prev | next|      |prev | next|      |prev | next|
             * +-----------+      +-----------+      +-----------+
             * |  0  |  2  |      |  1  |  0  |      |  2  |  1  |
             * |     |     |      |     |     |      |     |     |
             * +-----+-----+      +-----+-----+      +-----+-----+
             *
             */
            SECTION( "Add item after removing")
            {
                REQUIRE(true == list.add(0xb33f1, new_node_index));
                {
                    REQUIRE(1 == context.m_first);
                    REQUIRE(0 == context.m_last);
                    REQUIRE(3 == context.m_count);
                    REQUIRE(0xb33f1 == context.m_buffer.at(new_node_index).m_data);

                    REQUIRE(0 == context.m_buffer.at(1).m_prev);
                    REQUIRE(2 == context.m_buffer.at(1).m_next);

                    REQUIRE(1 == context.m_buffer.at(2).m_prev);
                    REQUIRE(0 == context.m_buffer.at(2).m_next);

                    REQUIRE(2 == context.m_buffer.at(0).m_prev);
                    REQUIRE(1 == context.m_buffer.at(0).m_next);
                }
            }
        }

        /*
         * Expected result
         * 
         *       0                  1              (removed)
         * +-----+-----+      +-----+-----+      +-----+-----+
         * |prev | next|      |prev | next|      |prev | next|
         * +-----------+      +-----------+      +-----------+
         * |  1  |  1  |      |  0  |  1  |      |     |     |
         * |     |     |      |     |     |      |     |     |
         * +-----+-----+      +-----+-----+      +-----+-----+
         *
         */
        SECTION( "Remove last item")
        {
            list.remove(2);
            {
                REQUIRE(0 == context.m_first);
                REQUIRE(1 == context.m_last);
                REQUIRE(2 == context.m_count);

                REQUIRE(1 == context.m_buffer.at(0).m_prev);
                REQUIRE(1 == context.m_buffer.at(0).m_next);

                REQUIRE(0 == context.m_buffer.at(1).m_prev);
                REQUIRE(0 == context.m_buffer.at(1).m_next);
            }

            /*
             * Expected result
             * 
             *       0                  1                  2
             * +-----+-----+      +-----+-----+      +-----+-----+
             * |prev | next|      |prev | next|      |prev | next|
             * +-----------+      +-----------+      +-----------+
             * |  2  |  1  |      |  0  |  2  |      |  1  |  0  |
             * |     |     |      |     |     |      |     |     |
             * +-----+-----+      +-----+-----+      +-----+-----+
             *
             */
            SECTION( "Add item after removing")
            {
                REQUIRE(true == list.add(0xb33f1, new_node_index));
                {
                    REQUIRE(0 == context.m_first);
                    REQUIRE(2 == context.m_last);
                    REQUIRE(3 == context.m_count);
                    REQUIRE(0xb33f1 == context.m_buffer.at(new_node_index).m_data);

                    REQUIRE(2 == context.m_buffer.at(0).m_prev);
                    REQUIRE(1 == context.m_buffer.at(0).m_next);

                    REQUIRE(0 == context.m_buffer.at(1).m_prev);
                    REQUIRE(2 == context.m_buffer.at(1).m_next);

                    REQUIRE(1 == context.m_buffer.at(2).m_prev);
                    REQUIRE(0 == context.m_buffer.at(2).m_next);
                }
            }
        }
    }
}
