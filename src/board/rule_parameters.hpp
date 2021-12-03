#pragma once
#include "common/common.hpp"
#include "rules/rule.hpp"

namespace horizon {
class RuleParameters : public Rule {
public:
    static const auto id = RuleID::PARAMETERS;
    RuleID get_id() const override
    {
        return id;
    }

    RuleParameters();
    RuleParameters(const json &j, const RuleImportMap &import_map);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr, class IPool *pool = nullptr) const override;
    bool can_export() const override
    {
        return true;
    }

    uint64_t solder_mask_expansion = 0.1_mm;
    uint64_t paste_mask_contraction = 0;
    uint64_t courtyard_expansion = 0.25_mm;
    uint64_t via_solder_mask_expansion = 0.1_mm;
    uint64_t hole_solder_mask_expansion = 0.1_mm;
};
} // namespace horizon
