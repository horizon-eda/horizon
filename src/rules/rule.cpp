#include "rule.hpp"
#include "nlohmann/json.hpp"

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

Rule::Rule(const json &j) : enabled(j.at("enabled"))
{
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
        {"single_pin_net", RuleID::SINGLE_PIN_NET},
        {"parameters", RuleID::PARAMETERS},
        {"via", RuleID::VIA},
        {"clearance_copper_other", RuleID::CLEARANCE_COPPER_OTHER},
        {"plane", RuleID::PLANE},
        {"diffpair", RuleID::DIFFPAIR},
        {"package_checks", RuleID::PACKAGE_CHECKS},
        {"preflight_checks", RuleID::PREFLIGHT_CHECKS},
        {"clearance_copper_keepout", RuleID::CLEARANCE_COPPER_KEEPOUT},
};

} // namespace horizon
