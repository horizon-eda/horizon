#include "rule_thermals.hpp"
#include "common/lut.hpp"
#include "util/util.hpp"
#include <sstream>
#include <nlohmann/json.hpp>
#include "board/board_package.hpp"

namespace horizon {

RuleThermals::RuleThermals(const UUID &uu) : Rule(uu)
{
}

static const LutEnumStr<RuleThermals::PadMode> mode_lut = {
        {"all", RuleThermals::PadMode::ALL},
        {"pads", RuleThermals::PadMode::PADS},
};

RuleThermals::RuleThermals(const UUID &uu, const json &j)
    : Rule(uu, j), match(j.at("match")), match_component(j.at("match_component")), layer(j.value("layer", 10000)),
      thermal_settings(j)
{
    if (j.count("pad_mode"))
        pad_mode = mode_lut.lookup(j.at("pad_mode"));
    if (j.count("pads")) {
        for (const auto &it : j.at("pads")) {
            pads.insert(it.get<std::string>());
        }
    }
}

json RuleThermals::serialize() const
{
    json j = Rule::serialize();
    j["match"] = match.serialize();
    j["match_component"] = match_component.serialize();
    thermal_settings.serialize(j);
    j["layer"] = layer;
    j["pad_mode"] = mode_lut.lookup_reverse(pad_mode);
    {
        auto o = json::array();
        for (const auto &it : pads) {
            o.push_back(static_cast<std::string>(it));
        }
        j["pads"] = o;
    }
    return j;
}

bool RuleThermals::matches(const class BoardPackage &pkg, const class Pad &pad, int la) const
{
    return enabled && match_component.matches(pkg.component) && match.match(pad.net)
           && ((pad_mode == PadMode::ALL) || pads.count(pad.uuid)) && ((layer == 10000) || (layer == la));
}


std::string RuleThermals::get_brief(const class Block *block, class IPool *pool) const
{
    std::stringstream ss;
    ss << "Match " << match_component.get_brief(block, pool) << "\n";
    ss << match.get_brief(block) << "\n";
    ss << layer_to_string(layer);
    return ss.str();
}

bool RuleThermals::can_export() const
{
    return match.can_export() && match_component.can_export();
}

} // namespace horizon
