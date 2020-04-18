#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "circular_list.hpp"

// Helper class used to unit test kernel::common::CircularLlist by accessing protected members.
class TestedClass : public kernel::common::CircularList<uint32_t, 4>
{
private:
    void testCirculity()
    {

    }
public:
    void addItems()
    {
        // Check if data is initialized correctly.
        REQUIRE(0 == m_first);
        REQUIRE(0 == m_last);
        REQUIRE(0 == m_count);

        REQUIRE(true == add(0x10)); //  1st element
        REQUIRE(0 == m_first);
        REQUIRE(0 == m_last);
        REQUIRE(1 == m_count);

        REQUIRE(true == add(0x20)); //  2nd element
        REQUIRE(0 == m_first);
        REQUIRE(1 == m_last);
        REQUIRE(2 == m_count);

        REQUIRE(true == add(0x30)); //  3rd element
        REQUIRE(0 == m_first);
        REQUIRE(2 == m_last);
        REQUIRE(3 == m_count);

        REQUIRE(true == add(0x40)); //  4th element
        REQUIRE(0 == m_first);
        REQUIRE(3 == m_last);
        REQUIRE(4 == m_count);

        REQUIRE(false == add(0x50)); //  5th element
        REQUIRE(0 == m_first);
        REQUIRE(3 == m_last);
        REQUIRE(4 == m_count);
    }

    void remoteItems()
    {
        //remove(0x40);
        remove(0x10);
        REQUIRE(0 == m_first);
        REQUIRE(3 == m_last);
        REQUIRE(4 == m_count);
        remove(0x30);
        remove(0x20);
    }
};

TEST_CASE("Test adding items")
{
    TestedClass testedClass;

    testedClass.addItems();
    testedClass.remoteItems();
}
