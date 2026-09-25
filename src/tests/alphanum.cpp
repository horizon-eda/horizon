#include "catch2/catch_amalgamated.hpp"
#include "util/util.hpp"
TEST_CASE("Natural comparison handles numeric runs longer than uint64_t")
{
    const std::string first = "part-18446744073709551616";
    const std::string second = "part-18446744073709551617";
    const std::string third = "part-18446744073709551618";
    CHECK(horizon::strcmp_natural(first, second) < 0);
    CHECK(horizon::strcmp_natural(second, third) < 0);
    CHECK(horizon::strcmp_natural(first, third) < 0);
    CHECK(horizon::strcmp_natural("", "") == 0);
    CHECK(horizon::strcmp_natural("", "a") < 0);
    CHECK(horizon::strcmp_natural("a", "") > 0);
    CHECK(horizon::strcmp_natural("a", "a") == 0);
    CHECK(horizon::strcmp_natural("", "9") < 0);
    CHECK(horizon::strcmp_natural("9", "") > 0);
    CHECK(horizon::strcmp_natural("1", "1") == 0);
    CHECK(horizon::strcmp_natural("1", "2") < 0);
    CHECK(horizon::strcmp_natural("3", "2") > 0);
    CHECK(horizon::strcmp_natural("a12", "a1") > 0);
    CHECK(horizon::strcmp_natural("a12", "a01") > 0);
    CHECK(horizon::strcmp_natural("a12", "a11") > 0);
    CHECK(horizon::strcmp_natural("a2", "a02") == 0);
    CHECK(horizon::strcmp_natural("a0", "a0000") == 0);
    CHECK(horizon::strcmp_natural("a0", "a0001") < 0);
    CHECK(horizon::strcmp_natural("a01", "a000") > 0);
    CHECK(horizon::strcmp_natural("a1", "a1") == 0);
    CHECK(horizon::strcmp_natural("a1", "a2") < 0);
    CHECK(horizon::strcmp_natural("a2", "a1") > 0);
    CHECK(horizon::strcmp_natural("a1a2", "a1a3") < 0);
    CHECK(horizon::strcmp_natural("a1a2", "a1a0") > 0);
    CHECK(horizon::strcmp_natural("134", "122") > 0);
    CHECK(horizon::strcmp_natural("12a3", "12a3") == 0);
    CHECK(horizon::strcmp_natural("12a1", "12a0") > 0);
    CHECK(horizon::strcmp_natural("12a1", "12a2") < 0);
    CHECK(horizon::strcmp_natural("a18446744073709551616", "a184467440737095516165") < 0);
    CHECK(horizon::strcmp_natural("a99918446744073709551616", "a99918446744073709551617") < 0);
    CHECK(horizon::strcmp_natural("a19918446744073709551616", "a99918446744073709551617") < 0);
    CHECK(horizon::strcmp_natural("a89918446744073709551616", "a79918446744073709551617") > 0);
    CHECK(horizon::strcmp_natural("a99918446744073709551618", "a99918446744073709551617") > 0);
    CHECK(horizon::strcmp_natural("a", "aa") < 0);
    CHECK(horizon::strcmp_natural("aaa", "aa") > 0);
    CHECK(horizon::strcmp_natural("Alpha 2", "Alpha 2") == 0);
    CHECK(horizon::strcmp_natural("Alpha 2", "Alpha 2A") < 0);
    CHECK(horizon::strcmp_natural("Alpha 2 B", "Alpha 2") > 0);
}
