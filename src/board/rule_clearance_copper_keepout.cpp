#include "rule_clearance_copper_keepout.hpp"
#include "common/lut.hpp"
#include "util/util.hpp"
#include <sstream>
#include "nlohmann/json.hpp"

namespace horizon {

RuleClearanceCopperKeepout::RuleClearanceCopperKeepout(const UUID &uu) : Rule(uu)
{
    id = RuleID::CLEARANCE_COPPER_KEEPOUT;
}

RuleClearanceCopperKeepout::RuleClearanceCopperKeepout(const UUID &uu, const json &j)
    : Rule(uu, j), match(j.at("match")), match_keepout(j.at("match_keepout")),
      routing_offset(j.value("routing_offset", 0.05_mm))
{
    id = RuleID::CLEARANCE_COPPER_KEEPOUT;
    {
        const json &o = j["clearances"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            PatchType a = patch_type_lut.lookup(it.key());
            set_clearance(a, it.value());
        }
    }
}

uint64_t RuleClearanceCopperKeepout::get_clearance(PatchType pt) const
{
    if (clearances.count(pt)) {
        return clearances.at(pt);
    }
    return 0;
}

uint64_t RuleClearanceCopperKeepout::get_max_clearance() const
{
    uint64_t max_clearance = 0;
    for (auto &it : clearances) {
        max_clearance = std::max(max_clearance, it.second);
    }
    return max_clearance;
}

void RuleClearanceCopperKeepout::set_clearance(PatchType pt, uint64_t c)
{
    clearances[pt] = c;
}

json RuleClearanceCopperKeepout::serialize() const
{
    json j = Rule::serialize();
    j["match"] = match.serialize();
    j["match_keepout"] = match_keepout.serialize();
    j["routing_offset"] = routing_offset;
    json o;
    for (const auto &it : clearances) {
        o[patch_type_lut.lookup_reverse(it.first)] = it.second;
    }
    j["clearances"] = o;
    return j;
}

std::string RuleClearanceCopperKeepout::get_brief(const class Block *block) const
{
    std::stringstream ss;
    ss << "Match " << match.get_brief(block) << "\n";
    ss << match_keepout.get_brief(block);
    return ss.str();
}

bool RuleClearanceCopperKeepout::is_match_all() const
{
    return match.mode == RuleMatch::Mode::ALL && match_keepout.mode == RuleMatchKeepout::Mode::ALL;
}

} // namespace horizon
