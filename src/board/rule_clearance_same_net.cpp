#include "rule_clearance_same_net.hpp"
#include "common/lut.hpp"
#include "util/util.hpp"
#include <sstream>
#include <nlohmann/json.hpp>

namespace horizon {

RuleClearanceSameNet::RuleClearanceSameNet(const UUID &uu) : Rule(uu)
{
}

RuleClearanceSameNet::RuleClearanceSameNet(const UUID &uu, const json &j, const RuleImportMap &import_map)
    : Rule(uu, j, import_map), match(j.at("match"), import_map), layer(j.at("layer"))
{
    for (const auto &va : j.at("clearances")) {
        PatchType a = patch_type_lut.lookup(va.at("types").at(0));
        PatchType b = patch_type_lut.lookup(va.at("types").at(1));
        set_clearance(a, b, va.at("clearance").get<uint64_t>());
    }
}

int64_t RuleClearanceSameNet::get_clearance(PatchType a, PatchType b) const
{
    if (static_cast<int>(a) > static_cast<int>(b)) {
        std::swap(a, b);
    }
    std::pair<PatchType, PatchType> key(a, b);
    if (clearances.count(key)) {
        return clearances.at(key);
    }
    return -1;
}

uint64_t RuleClearanceSameNet::get_max_clearance() const
{
    int64_t max_clearance = 0;
    for (auto &it : clearances) {
        max_clearance = std::max(max_clearance, it.second);
    }
    return max_clearance;
}

void RuleClearanceSameNet::set_clearance(PatchType a, PatchType b, int64_t c)
{
    if (static_cast<int>(a) > static_cast<int>(b)) {
        std::swap(a, b);
    }
    std::pair<PatchType, PatchType> key(a, b);
    clearances[key] = c;
}

json RuleClearanceSameNet::serialize() const
{
    json j = Rule::serialize();
    j["match"] = match.serialize();
    j["layer"] = layer;
    j["clearances"] = json::array();
    for (const auto &it : clearances) {
        json k;
        k["types"] = {patch_type_lut.lookup_reverse(it.first.first), patch_type_lut.lookup_reverse(it.first.second)};
        k["clearance"] = it.second;
        j["clearances"].push_back(k);
    }
    return j;
}

std::string RuleClearanceSameNet::get_brief(const class Block *block, class IPool *pool) const
{
    std::stringstream ss;
    ss << "Match " << match.get_brief(block) << "\n";
    ss << layer_to_string(layer);
    return ss.str();
}

bool RuleClearanceSameNet::is_match_all() const
{
    return match.mode == RuleMatch::Mode::ALL && layer == 10000;
}

bool RuleClearanceSameNet::can_export() const
{
    return match.can_export();
}

} // namespace horizon
