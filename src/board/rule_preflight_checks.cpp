#include "rule_preflight_checks.hpp"
#include "util/util.hpp"
#include <sstream>
#include "nlohmann/json.hpp"

namespace horizon {
RulePreflightChecks::RulePreflightChecks() : Rule()
{
    id = RuleID::PREFLIGHT_CHECKS;
}

RulePreflightChecks::RulePreflightChecks(const json &j) : Rule(j)
{
    id = RuleID::PREFLIGHT_CHECKS;
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
