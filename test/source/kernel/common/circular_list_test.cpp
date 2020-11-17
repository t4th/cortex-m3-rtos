#include "catch.hpp"

#include "circular_list.hpp"

TEST_CASE("CircularList")
{
    kernel::internal::common::CircularList<uint32_t, 3> list;

    REQUIRE(0 == list.count());

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
            REQUIRE(0 == list.firstIndex());
            REQUIRE(1 == list.count());
            REQUIRE(0xabc == list.at(new_node_index));

            REQUIRE(0 == list.nextIndex(new_node_index));
        }

        REQUIRE(true == list.add(0x123, new_node_index));
        {
            REQUIRE(0 == list.firstIndex());
            REQUIRE(2 == list.count());
            REQUIRE(0x123 == list.at(new_node_index));

            REQUIRE(1 == list.nextIndex(0));

            REQUIRE(0 == list.nextIndex(new_node_index));
        }

        REQUIRE(true == list.add(0xbaba, new_node_index));
        {
            REQUIRE(0 == list.firstIndex());
            REQUIRE(3 == list.count());
            REQUIRE(0xbaba == list.at(new_node_index));

            REQUIRE(1 == list.nextIndex(0));

            REQUIRE(2 == list.nextIndex(1));

            REQUIRE(0 == list.nextIndex(2));
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
                REQUIRE(0 == list.firstIndex());
                REQUIRE(2 == list.count());

                REQUIRE(2 == list.nextIndex(0));
                
                REQUIRE(0 == list.nextIndex(2));
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
                    REQUIRE(0 == list.firstIndex());
                    REQUIRE(3 == list.count());
                    REQUIRE(0xb33f == list.at(new_node_index));

                    REQUIRE(2 == list.nextIndex(0));

                    REQUIRE(1 == list.nextIndex(2));

                    REQUIRE(0 == list.nextIndex(1));
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
                REQUIRE(1 == list.firstIndex());
                REQUIRE(2 == list.count());

                REQUIRE(2 == list.nextIndex(1));

                REQUIRE(1 == list.nextIndex(2));
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
                    REQUIRE(1 == list.firstIndex());
                    REQUIRE(3 == list.count());
                    REQUIRE(0xb33f1 == list.at(new_node_index));

                    REQUIRE(2 == list.nextIndex(1));

                    REQUIRE(0 == list.nextIndex(2));

                    REQUIRE(1 == list.nextIndex(0));
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
                REQUIRE(0 == list.firstIndex());
                REQUIRE(2 == list.count());

                REQUIRE(1 == list.nextIndex(0));

                REQUIRE(0 == list.nextIndex(1));
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
                    REQUIRE(0 == list.firstIndex());
                    REQUIRE(3 == list.count());
                    REQUIRE(0xb33f1 == list.at(new_node_index));

                    REQUIRE(1 == list.nextIndex(0));

                    REQUIRE(2 == list.nextIndex(1));

                    REQUIRE(0 == list.nextIndex(2));
                }
            }
        }
    }
}
