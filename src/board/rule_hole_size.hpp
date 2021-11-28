#pragma once
#include "common/common.hpp"
#include "rules/rule.hpp"

namespace horizon {
class RuleHoleSize : public Rule {
public:
    RuleHoleSize(const UUID &uu);
    RuleHoleSize(const UUID &uu, const json &j, const RuleImportMap &import_map);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr, class IPool *pool = nullptr) const override;
    bool can_export() const override;

    uint64_t diameter_min = 0.1_mm;
    uint64_t diameter_max = 10_mm;
    RuleMatch match;
};
} // namespace horizon
