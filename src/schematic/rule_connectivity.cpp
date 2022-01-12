#include "rule_connectivity.hpp"
#include "util/util.hpp"
#include <sstream>
#include "nlohmann/json.hpp"

namespace horizon {
RuleConnectivity::RuleConnectivity() : Rule()
{
}

RuleConnectivity::RuleConnectivity(const json &j) : Rule(j)
{
    include_unnamed = j.at("include_unnamed").get<bool>();
}

json RuleConnectivity::serialize() const
{
    json j = Rule::serialize();
    j["include_unnamed"] = include_unnamed;
    return j;
}

std::string RuleConnectivity::get_brief(const class Block *block, class IPool *pool) const
{
    return "";
}
} // namespace horizon
