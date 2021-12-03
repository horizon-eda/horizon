#include "rule_preflight_checks.hpp"
#include "util/util.hpp"
#include <sstream>
#include "nlohmann/json.hpp"

namespace horizon {
RulePreflightChecks::RulePreflightChecks() : Rule()
{
}

RulePreflightChecks::RulePreflightChecks(const json &j) : Rule(j)
{
}

json RulePreflightChecks::serialize() const
{
    json j = Rule::serialize();
    return j;
}

std::string RulePreflightChecks::get_brief(const class Block *block, class IPool *pool) const
{
    return "";
}
} // namespace horizon
