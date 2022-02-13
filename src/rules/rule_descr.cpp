#include "rule_descr.hpp"

namespace horizon {

using RD = RuleDescription;

const std::map<RuleID, RD> rule_descriptions = {
        {RuleID::HOLE_SIZE, {"Hole size", RD::IS_MULTI | RD::CAN_CHECK}},

        {RuleID::TRACK_WIDTH, {"Track width", RD::IS_MULTI | RD::CAN_CHECK | RD::CAN_APPLY | RD::NEEDS_MATCH_ALL}},

        {RuleID::CLEARANCE_COPPER, {"Copper clearance", RD::IS_MULTI | RD::CAN_CHECK | RD::NEEDS_MATCH_ALL}},

        {RuleID::CLEARANCE_SILKSCREEN_EXPOSED_COPPER, {"Clearance Silkscreen - Exposed copper", RD::CAN_CHECK}},

        {RuleID::PARAMETERS, {"Parameters", RD::CAN_APPLY}},

        {RuleID::CONNECTIVITY, {"Connectivity", RD::CAN_CHECK}},

        {RuleID::VIA, {"Vias", RD::IS_MULTI | RD::CAN_APPLY | RD::NEEDS_MATCH_ALL}},

        {RuleID::CLEARANCE_COPPER_OTHER,
         {"Clearance Copper - Other", RD::IS_MULTI | RD::CAN_CHECK | RD::NEEDS_MATCH_ALL}},

        {RuleID::PLANE, {"Planes", RD::IS_MULTI | RD::CAN_APPLY | RD::NEEDS_MATCH_ALL | RD::CAN_CHECK}},

        {RuleID::DIFFPAIR, {"Diffpair", RD::IS_MULTI}},

        {RuleID::PACKAGE_CHECKS, {"Package checks", RD::CAN_CHECK}},

        {RuleID::SHORTED_PADS, {"Shorted Pads", RD::IS_MULTI | RD::CAN_APPLY}},

        {RuleID::THERMALS, {"Thermals", RD::IS_MULTI}},

        {RuleID::PREFLIGHT_CHECKS, {"Preflight checks", RD::CAN_CHECK}},

        {RuleID::CLEARANCE_COPPER_KEEPOUT,
         {"Clearance Copper - Keepout", RD::IS_MULTI | RD::CAN_CHECK | RD::NEEDS_MATCH_ALL}},

        {RuleID::LAYER_PAIR, {"Layer pairs", RD::IS_MULTI}},

        {RuleID::CLEARANCE_SAME_NET, {"Same net clearance", RD::IS_MULTI | RD::CAN_CHECK}},

        {RuleID::SYMBOL_CHECKS, {"Symbol checks", RD::CAN_CHECK}},

        {RuleID::CLEARANCE_PACKAGE, {"Package clearance", RD::CAN_CHECK}},

        {RuleID::NET_TIES, {"Net ties", RD::CAN_CHECK}},
};
} // namespace horizon
