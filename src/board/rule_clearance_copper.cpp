#include "rule_clearance_copper.hpp"
#include "common/lut.hpp"
#include "util/util.hpp"
#include <sstream>
#include "nlohmann/json.hpp"

namespace horizon {

RuleClearanceCopper::RuleClearanceCopper(const UUID &uu) : Rule(uu)
{
    std::fill(clearances.begin(), clearances.end(), .1_mm);
}

RuleClearanceCopper::RuleClearanceCopper(const UUID &uu, const json &j, const RuleImportMap &import_map)
    : Rule(uu, j, import_map), match_1(j.at("match_1"), import_map), match_2(j.at("match_2"), import_map),
      layer(j.at("layer")), routing_offset(j.value("routing_offset", 0.05_mm))
{
    std::fill(clearances.begin(), clearances.end(), .1_mm);

    for (const auto &va : j.at("clearances")) {
        PatchType a = patch_type_lut.lookup(va.at("types").at(0));
        PatchType b = patch_type_lut.lookup(va.at("types").at(1));
        set_clearance(a, b, va.at("clearance").get<uint64_t>());
    }
}

static size_t get_index(PatchType a, PatchType b)
{
    if (a > b) {
        std::swap(a, b);
    }
    return static_cast<size_t>(a) * static_cast<size_t>(PatchType::N_TYPES) + static_cast<size_t>(b);
}

static std::optional<std::pair<PatchType, PatchType>> pt_from_index(size_t idx)
{
    constexpr auto nt = static_cast<size_t>(PatchType::N_TYPES);
    auto ai = (idx / nt);
    auto bi = (idx % nt);
    if (ai > bi)
        return std::nullopt;
    auto a = static_cast<PatchType>(ai);
    auto b = static_cast<PatchType>(bi);
    assert(idx == get_index(a, b));
    return std::make_pair(a, b);
}

json RuleClearanceCopper::serialize() const
{
    json j = Rule::serialize();
    j["match_1"] = match_1.serialize();
    j["match_2"] = match_2.serialize();
    j["layer"] = layer;
    j["routing_offset"] = routing_offset;
    j["clearances"] = json::array();
    size_t i = 0;
    for (const auto &it : clearances) {
        auto pts = pt_from_index(i);
        if (pts.has_value()) {
            json k;
            k["types"] = {patch_type_lut.lookup_reverse(pts->first), patch_type_lut.lookup_reverse(pts->second)};
            k["clearance"] = it;
            j["clearances"].push_back(k);
        }
        i++;
    }
    return j;
}


uint64_t RuleClearanceCopper::get_clearance(PatchType a, PatchType b) const
{
    return clearances.at(get_index(a, b));
}

uint64_t RuleClearanceCopper::get_max_clearance() const
{
    uint64_t max_clearance = 0;
    for (auto &it : clearances) {
        max_clearance = std::max(max_clearance, it);
    }
    return max_clearance;
}

void RuleClearanceCopper::set_clearance(PatchType a, PatchType b, uint64_t c)
{
    clearances.at(get_index(a, b)) = c;
}

std::string RuleClearanceCopper::get_brief(const class Block *block, class IPool *pool) const
{
    std::stringstream ss;
    ss << "1<sup>st</sup> Match " << match_1.get_brief(block) << "\n";
    ss << "2<sup>nd</sup> Match " << match_2.get_brief(block) << "\n";
    ss << "Layer " << layer;
    return ss.str();
}

bool RuleClearanceCopper::is_match_all() const
{
    return match_1.mode == RuleMatch::Mode::ALL && match_2.mode == RuleMatch::Mode::ALL && layer == 10000;
}

bool RuleClearanceCopper::can_export() const
{
    return match_1.can_export() && match_2.can_export();
}

} // namespace horizon
