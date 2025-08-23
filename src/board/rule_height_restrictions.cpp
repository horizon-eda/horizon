#include "rule_height_restrictions.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
RuleHeightRestrictions::RuleHeightRestrictions() : Rule()
{
}

RuleHeightRestrictions::RuleHeightRestrictions(const json &j) : Rule(j)
{
}

json RuleHeightRestrictions::serialize() const
{
    json j = Rule::serialize();
    return j;
}

std::string RuleHeightRestrictions::get_brief(const class Block *block, class IPool *pool) const
{
    return "";
}
} // namespace horizon
