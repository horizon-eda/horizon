#pragma once
#include "common/common.hpp"
#include "rules/rule.hpp"
#include "rules/rule_match.hpp"
#include "rules/rule_match_keepout.hpp"

namespace horizon {
class RuleClearanceCopperKeepout : public Rule {
public:
    RuleClearanceCopperKeepout(const UUID &uu);
    RuleClearanceCopperKeepout(const UUID &uu, const json &j);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr) const override;
    bool is_match_all() const override;

    uint64_t get_clearance(PatchType pt_copper) const;
    void set_clearance(PatchType pt_copper, uint64_t c);
    uint64_t get_max_clearance() const;

    RuleMatch match;
    RuleMatchKeepout match_keepout;
    uint64_t routing_offset = 0.05_mm;

private:
    std::map<PatchType, uint64_t> clearances;
};
} // namespace horizon
