#include "catch2/catch_amalgamated.hpp"
#include "util/util.hpp"

TEST_CASE("Natural comparison handles numeric runs longer than unsigned long")
{
    const std::string first = "part-18446744073709551616";
    const std::string second = "part-18446744073709551617";
    const std::string third = "part-18446744073709551618";

    CHECK(horizon::strcmp_natural(first, second) < 0);
    CHECK(horizon::strcmp_natural(second, third) < 0);
    CHECK(horizon::strcmp_natural(first, third) < 0);
}