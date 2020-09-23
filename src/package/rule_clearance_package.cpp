#include "rule_clearance_package.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
RuleClearancePackage::RuleClearancePackage() : Rule()
{
    id = RuleID::CLEARANCE_PACKAGE;
}

RuleClearancePackage::RuleClearancePackage(const json &j) : Rule(j)
{
    id = RuleID::CLEARANCE_PACKAGE;
    clearance_silkscreen_cu = j.value("clearance_silkscreen_cu", .2_mm);
    clearance_silkscreen_pkg = j.value("clearance_silkscreen_pkg", .2_mm);
}

json RuleClearancePackage::serialize() const
{
    json j = Rule::serialize();
    j["clearance_silkscreen_cu"] = clearance_silkscreen_cu;
    j["clearance_silkscreen_pkg"] = clearance_silkscreen_pkg;
    return j;
}

std::string RuleClearancePackage::get_brief(const class Block *block) const
{
    return "";
}
} // namespace horizon
