#pragma once
#include "common/common.hpp"
#include "rules/rule.hpp"
#include "rules/rule_match.hpp"

namespace horizon {
class RuleDiffpair : public Rule {
public:
    static const auto id = RuleID::DIFFPAIR;
    RuleID get_id() const override
    {
        return id;
    }

    RuleDiffpair(const UUID &uu);
    RuleDiffpair(const UUID &uu, const json &j, const RuleImportMap &import_map);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr, class IPool *pool = nullptr) const override;
    bool can_export() const override
    {
        return true;
    }

    UUID net_class;
    int layer = 10000;

    uint64_t track_width = .2_mm;
    uint64_t track_gap = .2_mm;
    uint64_t via_gap = .2_mm;
};
} // namespace horizon
