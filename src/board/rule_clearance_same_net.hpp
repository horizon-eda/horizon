#pragma once
#include "common/common.hpp"
#include "rules/rule.hpp"
#include "rules/rule_match.hpp"

namespace horizon {
class RuleClearanceSameNet : public Rule {
public:
    static const auto id = RuleID::CLEARANCE_SAME_NET;
    RuleID get_id() const override
    {
        return id;
    }

    RuleClearanceSameNet(const UUID &uu);
    RuleClearanceSameNet(const UUID &uu, const json &j, const RuleImportMap &import_map);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr, class IPool *pool = nullptr) const override;
    bool is_match_all() const override;
    bool can_export() const override;

    int64_t get_clearance(PatchType a, PatchType b) const;
    void set_clearance(PatchType a, PatchType b, int64_t c);
    uint64_t get_max_clearance() const;

    RuleMatch match;
    int layer = 10000;

private:
    std::map<std::pair<PatchType, PatchType>, int64_t> clearances;
};
} // namespace horizon
