#include "rule_net_ties.hpp"
#include "util/util.hpp"
#include <nlohmann/json.hpp>

namespace horizon {
RuleNetTies::RuleNetTies() : Rule()
{
}

RuleNetTies::RuleNetTies(const json &j) : Rule(j)
{
}

json RuleNetTies::serialize() const
{
    json j = Rule::serialize();
    return j;
}

std::string RuleNetTies::get_brief(const class Block *block, class IPool *pool) const
{
    return "";
}
} // namespace horizon
