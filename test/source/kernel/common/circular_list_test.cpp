#include "catch.hpp"

#include "circular_list.hpp"

TEST_CASE( "CircularList")
{
    kernel::internal::common::CircularList< uint32_t, 3U> list;
    typedef kernel::internal::common::CircularList< uint32_t, 3U>::Id ListId;

    REQUIRE( 0U == list.count());

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
        ListId new_node_index = static_cast< ListId>( 0U);

        REQUIRE( true == list.add( 0xabcU, new_node_index));
        {
            REQUIRE( static_cast< ListId>( 0U) == list.firstIndex());
            REQUIRE( 1U == list.count());
            REQUIRE( 0xabcU == list.at( new_node_index));

            REQUIRE( static_cast< ListId>( 0U) == list.nextIndex( new_node_index));
        }

        REQUIRE( true == list.add( 0x123U, new_node_index));
        {
            REQUIRE( static_cast< ListId>( 0U) == list.firstIndex());
            REQUIRE( 2U == list.count());
            REQUIRE( 0x123U == list.at( new_node_index));

            REQUIRE( static_cast< ListId>( 1U) == list.nextIndex(  static_cast< ListId>( 0U)));

            REQUIRE( static_cast< ListId>( 0U) == list.nextIndex( new_node_index));
        }

        REQUIRE( true == list.add( 0xbabaU, new_node_index));
        {
            REQUIRE(  static_cast< ListId>( 0U) == list.firstIndex());
            REQUIRE( 3U == list.count());
            REQUIRE( 0xbabaU == list.at(new_node_index));

            REQUIRE( static_cast< ListId>( 1U) == list.nextIndex( static_cast< ListId>( 0U)));

            REQUIRE( static_cast< ListId>( 2U) == list.nextIndex( static_cast< ListId>( 1U)));

            REQUIRE( static_cast< ListId>( 0U) == list.nextIndex( static_cast< ListId>( 2U)));
        }

        SECTION( "Try to overflow.")
        {
            REQUIRE( false == list.add( 0xFF34U, new_node_index));
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
            list.remove( static_cast< ListId>( 1U));
            {
                REQUIRE( static_cast< ListId>( 0U) == list.firstIndex());
                REQUIRE( 2U == list.count());

                REQUIRE( static_cast< ListId>( 2U) == list.nextIndex( static_cast< ListId>( 0U)));
                
                REQUIRE( static_cast< ListId>( 0U) == list.nextIndex( static_cast< ListId>( 2U)));
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
                REQUIRE( true == list.add( 0xb33fU, new_node_index));
                {
                    REQUIRE( static_cast< ListId>( 0U) == list.firstIndex());
                    REQUIRE( 3U == list.count());
                    REQUIRE( 0xb33fU == list.at( new_node_index));

                    REQUIRE( static_cast< ListId>( 2U) == list.nextIndex( static_cast< ListId>( 0U)));

                    REQUIRE( static_cast< ListId>( 1U) == list.nextIndex( static_cast< ListId>( 2U)));

                    REQUIRE( static_cast< ListId>( 0U) == list.nextIndex( static_cast< ListId>( 1U)));
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
        SECTION( "Remove first item.")
        {
            list.remove( static_cast< ListId>( 0U));
            {
                REQUIRE( static_cast< ListId>( 1U) == list.firstIndex());
                REQUIRE( 2U == list.count());

                REQUIRE( static_cast< ListId>( 2U) == list.nextIndex( static_cast< ListId>( 1U)));

                REQUIRE( static_cast< ListId>( 1U) == list.nextIndex( static_cast< ListId>( 2U)));
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
            SECTION( "Add item after removing.")
            {
                REQUIRE( true == list.add( 0xb33f1U, new_node_index));
                {
                    REQUIRE( static_cast< ListId>( 1U) == list.firstIndex());
                    REQUIRE( 3U == list.count());
                    REQUIRE( 0xb33f1U == list.at( new_node_index));

                    REQUIRE( static_cast< ListId>( 2U) == list.nextIndex( static_cast< ListId>( 1U)));

                    REQUIRE( static_cast< ListId>( 0U) == list.nextIndex( static_cast< ListId>( 2U)));

                    REQUIRE( static_cast< ListId>( 1U) == list.nextIndex( static_cast< ListId>( 0U)));
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
            list.remove( static_cast< ListId>( 2U));
            {
                REQUIRE( static_cast< ListId>( 0U) == list.firstIndex());
                REQUIRE( 2U == list.count());

                REQUIRE( static_cast< ListId>( 1U) == list.nextIndex( static_cast< ListId>( 0U)));

                REQUIRE( static_cast< ListId>( 0U) == list.nextIndex( static_cast< ListId>( 1U)));
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
                REQUIRE( true == list.add( 0xb33f1U, new_node_index));
                {
                    REQUIRE( static_cast< ListId>( 0U) == list.firstIndex());
                    REQUIRE( 3U == list.count());
                    REQUIRE( 0xb33f1U == list.at( new_node_index));

                    REQUIRE( static_cast< ListId>( 1U) == list.nextIndex( static_cast< ListId>( 0U)));

                    REQUIRE( static_cast< ListId>( 2U) == list.nextIndex( static_cast< ListId>( 1U)));

                    REQUIRE( static_cast< ListId>( 0U) == list.nextIndex( static_cast< ListId>( 2U)));
                }
            }
        }
    }
}
