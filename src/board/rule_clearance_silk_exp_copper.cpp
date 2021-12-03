#include "rule_clearance_silk_exp_copper.hpp"
#include "util/util.hpp"
#include <sstream>
#include "nlohmann/json.hpp"

namespace horizon {
RuleClearanceSilkscreenExposedCopper::RuleClearanceSilkscreenExposedCopper() : Rule()
{
}

RuleClearanceSilkscreenExposedCopper::RuleClearanceSilkscreenExposedCopper(const json &j,
                                                                           const RuleImportMap &import_map)
    : Rule(j, import_map)
{
    clearance_top = j.at("clearance_top").get<uint64_t>();
    clearance_bottom = j.at("clearance_bottom").get<uint64_t>();
}

json RuleClearanceSilkscreenExposedCopper::serialize() const
{
    json j = Rule::serialize();
    j["clearance_top"] = clearance_top;
    j["clearance_bottom"] = clearance_bottom;
    return j;
}

std::string RuleClearanceSilkscreenExposedCopper::get_brief(const class Block *block, class IPool *pool) const
{
    return "";
}
} // namespace horizon
