#include "rule_parameters.hpp"
#include "util/util.hpp"
#include <sstream>
#include "nlohmann/json.hpp"

namespace horizon {
RuleParameters::RuleParameters() : Rule()
{
    id = RuleID::PARAMETERS;
}

RuleParameters::RuleParameters(const json &j, const RuleImportMap &import_map) : Rule(j, import_map)
{
    id = RuleID::PARAMETERS;
    solder_mask_expansion = j.at("solder_mask_expansion").get<uint64_t>();
    paste_mask_contraction = j.at("paste_mask_contraction").get<uint64_t>();
    courtyard_expansion = j.at("courtyard_expansion").get<uint64_t>();
    via_solder_mask_expansion = j.value("via_solder_mask_expansion", .1_mm);
    hole_solder_mask_expansion = j.value("hole_solder_mask_expansion", .1_mm);
}

json RuleParameters::serialize() const
{
    json j = Rule::serialize();
    j["solder_mask_expansion"] = solder_mask_expansion;
    j["paste_mask_contraction"] = paste_mask_contraction;
    j["courtyard_expansion"] = courtyard_expansion;
    j["via_solder_mask_expansion"] = via_solder_mask_expansion;
    j["hole_solder_mask_expansion"] = hole_solder_mask_expansion;
    return j;
}

std::string RuleParameters::get_brief(const class Block *block) const
{
    return "";
}
} // namespace horizon
