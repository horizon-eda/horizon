#include "rule_descr.hpp"

namespace horizon {
const std::map<RuleID, RuleDescription> rule_descriptions = {
        {RuleID::HOLE_SIZE, {"Hole size", true, true}},
        {RuleID::TRACK_WIDTH, {"Track width", true, true, true}},
        {RuleID::CLEARANCE_COPPER, {"Copper clearance", true, true, true}},
        {RuleID::CLEARANCE_SILKSCREEN_EXPOSED_COPPER, {"Clearance\nSilkscreen - Exposed copper", false, false}},
        {RuleID::PARAMETERS, {"Parameters", false, false}},
        {RuleID::SINGLE_PIN_NET, {"Single pin nets", false, true}},
        {RuleID::VIA, {"Vias", true, false, true}},
        {RuleID::CLEARANCE_COPPER_OTHER, {"Clearance Copper - Other", true, true, true}},
        {RuleID::PLANE, {"Planes", true, false, true}},
        {RuleID::DIFFPAIR, {"Diffpair", true, false}},
        {RuleID::PACKAGE_CHECKS, {"Package checks", false, true}},
        {RuleID::PREFLIGHT_CHECKS, {"Preflight checks", false, true}},
        {RuleID::CLEARANCE_COPPER_KEEPOUT, {"Clearance Copper - Keepout", true, true, true}},
        {RuleID::LAYER_PAIR, {"Layer pairs", true, false, false}},
        {RuleID::CLEARANCE_SAME_NET, {"Same net clearance", true, true, false}},
        {RuleID::SYMBOL_CHECKS, {"Symbol checks", false, true}},
};
}
