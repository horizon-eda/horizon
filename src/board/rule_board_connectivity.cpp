#include "rule_board_connectivity.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
RuleBoardConnectivity::RuleBoardConnectivity() : Rule()
{
}

RuleBoardConnectivity::RuleBoardConnectivity(const json &j) : Rule(j)
{
}

json RuleBoardConnectivity::serialize() const
{
    json j = Rule::serialize();
    return j;
}

std::string RuleBoardConnectivity::get_brief(const class Block *block, class IPool *pool) const
{
    return "";
}
} // namespace horizon
