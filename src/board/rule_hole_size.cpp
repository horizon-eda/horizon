#include "rule_hole_size.hpp"
#include "util/util.hpp"
#include <sstream>
#include <nlohmann/json.hpp>

namespace horizon {
RuleHoleSize::RuleHoleSize(const UUID &uu) : Rule(uu)
{
}

RuleHoleSize::RuleHoleSize(const UUID &uu, const json &j, const RuleImportMap &import_map)
    : Rule(uu, j, import_map), diameter_min(j.at("diameter_min")), diameter_max(j.at("diameter_max")),
      match(j.at("match"), import_map)
{
}

json RuleHoleSize::serialize() const
{
    json j = Rule::serialize();
    j["diameter_min"] = diameter_min;
    j["diameter_max"] = diameter_max;
    j["match"] = match.serialize();
    return j;
}

std::string RuleHoleSize::get_brief(const class Block *block, class IPool *pool) const
{
    return "Match " + match.get_brief(block);
}

bool RuleHoleSize::can_export() const
{
    return match.can_export();
}

} // namespace horizon
