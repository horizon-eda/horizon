#include "rule.hpp"
#include "nlohmann/json.hpp"
#include "board/board_layers.hpp"

namespace horizon {
Rule::Rule()
{
}
Rule::~Rule()
{
}

Rule::Rule(const UUID &uu) : uuid(uu)
{
}

Rule::Rule(const UUID &uu, const json &j) : uuid(uu), enabled(j.at("enabled")), order(j.value("order", 0))
{
}

Rule::Rule(const UUID &uu, const json &j, const RuleImportMap &import_map) : Rule(uu, j)
{
    order = import_map.get_order(order);
    imported = import_map.is_imported();
}

Rule::Rule(const json &j) : enabled(j.at("enabled"))
{
}

Rule::Rule(const json &j, const RuleImportMap &import_map) : Rule(j)
{
    imported = import_map.is_imported();
}

json Rule::serialize() const
{
    json j;
    j["enabled"] = enabled;
    j["order"] = order;
    return j;
}

const LutEnumStr<RuleID> rule_id_lut = {
        {"none", RuleID::NONE},
        {"hole_size", RuleID::HOLE_SIZE},
        {"clearance_silkscreen_exposed_copper", RuleID::CLEARANCE_SILKSCREEN_EXPOSED_COPPER},
        {"track_width", RuleID::TRACK_WIDTH},
        {"clearance_copper", RuleID::CLEARANCE_COPPER},
        {"single_pin_net", RuleID::CONNECTIVITY},
        {"parameters", RuleID::PARAMETERS},
        {"via", RuleID::VIA},
        {"clearance_copper_other", RuleID::CLEARANCE_COPPER_OTHER},
        {"plane", RuleID::PLANE},
        {"diffpair", RuleID::DIFFPAIR},
        {"package_checks", RuleID::PACKAGE_CHECKS},
        {"preflight_checks", RuleID::PREFLIGHT_CHECKS},
        {"clearance_copper_keepout", RuleID::CLEARANCE_COPPER_KEEPOUT},
        {"board_connectivity", RuleID::BOARD_CONNECTIVITY},
};

std::string Rule::layer_to_string(int layer)
{
    if (layer == 10000)
        return "Any Layer";
    else
        return BoardLayers::get_layer_name(layer);
}

} // namespace horizon
