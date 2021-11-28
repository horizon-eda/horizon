#include "rule_track_width.hpp"
#include "util/util.hpp"
#include <sstream>
#include "nlohmann/json.hpp"

namespace horizon {
RuleTrackWidth::Widths::Widths()
{
}
RuleTrackWidth::Widths::Widths(const json &j) : min(j.at("min")), max(j.at("max")), def(j.at("def"))
{
}
json RuleTrackWidth::Widths::serialize() const
{
    json j;
    j["min"] = min;
    j["max"] = max;
    j["def"] = def;
    return j;
}

RuleTrackWidth::RuleTrackWidth(const UUID &uu) : Rule(uu)
{
    id = RuleID::TRACK_WIDTH;
}

RuleTrackWidth::RuleTrackWidth(const UUID &uu, const json &j, const RuleImportMap &import_map)
    : Rule(uu, j, import_map), match(j.at("match"), import_map)
{
    id = RuleID::TRACK_WIDTH;
    {
        const json &o = j["widths"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            int layer = std::stoi(it.key());
            widths.emplace(std::piecewise_construct, std::forward_as_tuple(layer), std::forward_as_tuple(it.value()));
        }
    }
}

json RuleTrackWidth::serialize() const
{
    json j = Rule::serialize();
    j["match"] = match.serialize();
    j["widths"] = json::object();
    for (const auto &it : widths) {
        j["widths"][std::to_string(it.first)] = it.second.serialize();
    }
    // j["diameter_min"] = diameter_min;
    // j["diameter_max"] = diameter_max;
    return j;
}

std::string RuleTrackWidth::get_brief(const class Block *block, class IPool *pool) const
{
    return "Match " + match.get_brief(block);
}

bool RuleTrackWidth::is_match_all() const
{
    return match.mode == RuleMatch::Mode::ALL;
}

bool RuleTrackWidth::can_export() const
{
    return match.can_export();
}

} // namespace horizon
