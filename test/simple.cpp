#define CATCH_CONFIG_MAIN
#include <catch.hpp>

TEST_CASE("1 == 1", "[identity]")
{
    REQUIRE(1 == 1);
}
