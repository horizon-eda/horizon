#include "catch2/catch_amalgamated.hpp"
#include "imp/action_catalog.hpp"

TEST_CASE("Check that tool/action lut is complete")
{
    for (const auto &[act, it] : horizon::action_catalog) {
        SECTION(it.name)
        {
            CHECK(act.is_valid());
            if (act.is_action())
                CHECK_NOTHROW(horizon::action_lut.lookup_reverse(act.action));
            if (act.is_tool())
                CHECK_NOTHROW(horizon::tool_lut.lookup_reverse(act.tool));
        }
    }
}
