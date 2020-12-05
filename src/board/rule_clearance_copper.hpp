#pragma once
#include "common/common.hpp"
#include "rules/rule.hpp"
#include "rules/rule_match.hpp"

namespace horizon {
class RuleClearanceCopper : public Rule {
public:
    RuleClearanceCopper(const UUID &uu);
    RuleClearanceCopper(const UUID &uu, const json &j, const RuleImportMap &import_map);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr) const override;
    bool is_match_all() const override;
    bool can_export() const override;

    RuleMatch match_1;
    RuleMatch match_2;
    int layer = 10000;
    uint64_t routing_offset = 0.05_mm;

    uint64_t get_clearance(PatchType a, PatchType b) const;
    void set_clearance(PatchType a, PatchType b, uint64_t c);
    uint64_t get_max_clearance() const;

private:
    std::array<uint64_t, static_cast<size_t>(PatchType::N_TYPES) * static_cast<size_t>(PatchType::N_TYPES)> clearances;
};
} // namespace horizon
