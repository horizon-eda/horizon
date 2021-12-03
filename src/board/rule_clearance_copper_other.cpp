#include "rule_clearance_copper_other.hpp"
#include "common/lut.hpp"
#include "util/util.hpp"
#include <sstream>
#include "nlohmann/json.hpp"

namespace horizon {

RuleClearanceCopperOther::RuleClearanceCopperOther(const UUID &uu) : Rule(uu)
{
}

RuleClearanceCopperOther::RuleClearanceCopperOther(const UUID &uu, const json &j, const RuleImportMap &import_map)
    : Rule(uu, j, import_map), match(j.at("match"), import_map), layer(j.at("layer")),
      routing_offset(j.value("routing_offset", 0.05_mm))
{
    for (const auto &va : j.at("clearances")) {
        PatchType a = patch_type_lut.lookup(va.at("types").at(0));
        PatchType b = patch_type_lut.lookup(va.at("types").at(1));
        set_clearance(a, b, va.at("clearance").get<uint64_t>());
    }
}

uint64_t RuleClearanceCopperOther::get_clearance(PatchType pt_cu, PatchType pt_ncu) const
{
    if (pt_ncu == PatchType::TEXT) // text is same as other (lines, arcs, etc.)
        pt_ncu = PatchType::OTHER;

    std::pair<PatchType, PatchType> key(pt_cu, pt_ncu);
    if (clearances.count(key)) {
        return clearances.at(key);
    }
    return .1_mm;
}

uint64_t RuleClearanceCopperOther::get_max_clearance() const
{
    uint64_t max_clearance = 0;
    for (auto &it : clearances) {
        max_clearance = std::max(max_clearance, it.second);
    }
    return max_clearance;
}

void RuleClearanceCopperOther::set_clearance(PatchType pt_cu, PatchType pt_ncu, uint64_t c)
{
    std::pair<PatchType, PatchType> key(pt_cu, pt_ncu);
    clearances[key] = c;
}

json RuleClearanceCopperOther::serialize() const
{
    json j = Rule::serialize();
    j["match"] = match.serialize();
    j["layer"] = layer;
    j["routing_offset"] = routing_offset;
    j["clearances"] = json::array();
    for (const auto &it : clearances) {
        json k;
        k["types"] = {patch_type_lut.lookup_reverse(it.first.first), patch_type_lut.lookup_reverse(it.first.second)};
        k["clearance"] = it.second;
        j["clearances"].push_back(k);
    }
    return j;
}

std::string RuleClearanceCopperOther::get_brief(const class Block *block, class IPool *pool) const
{
    std::stringstream ss;
    ss << "Match " << match.get_brief(block) << "\n";
    ss << "Layer " << layer;
    return ss.str();
}

bool RuleClearanceCopperOther::is_match_all() const
{
    return match.mode == RuleMatch::Mode::ALL && layer == 10000;
}

bool RuleClearanceCopperOther::can_export() const
{
    return match.can_export();
}

} // namespace horizon
