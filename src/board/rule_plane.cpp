#include "rule_plane.hpp"
#include "util/util.hpp"
#include <sstream>
#include "nlohmann/json.hpp"

namespace horizon {
RulePlane::RulePlane(const UUID &uu) : Rule(uu)
{
    id = RuleID::PLANE;
}

RulePlane::RulePlane(const UUID &uu, const json &j, const RuleImportMap &import_map)
    : Rule(uu, j, import_map), match(j.at("match"), import_map), layer(j.at("layer")), settings(j.at("settings"))
{
    id = RuleID::PLANE;
}

json RulePlane::serialize() const
{
    json j = Rule::serialize();
    j["match"] = match.serialize();
    j["layer"] = layer;
    j["settings"] = settings.serialize();
    return j;
}

std::string RulePlane::get_brief(const class Block *block, class IPool *pool) const
{
    return "Match " + match.get_brief(block) + "\nLayer " + std::to_string(layer);
}

bool RulePlane::is_match_all() const
{
    return match.mode == RuleMatch::Mode::ALL && layer == 10000;
}

bool RulePlane::can_export() const
{
    return match.can_export();
}

} // namespace horizon
