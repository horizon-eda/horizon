#include "rule_package_checks.hpp"
#include "util/util.hpp"
#include <sstream>
#include "nlohmann/json.hpp"

namespace horizon {
RulePackageChecks::RulePackageChecks() : Rule()
{
    id = RuleID::PACKAGE_CHECKS;
}

RulePackageChecks::RulePackageChecks(const json &j) : Rule(j)
{
    id = RuleID::PACKAGE_CHECKS;
}

json RulePackageChecks::serialize() const
{
    json j = Rule::serialize();
    return j;
}

std::string RulePackageChecks::get_brief(const class Block *block, class IPool *pool) const
{
    return "";
}
} // namespace horizon
