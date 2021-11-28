#pragma once
#include "common/common.hpp"
#include "rules/rule.hpp"

namespace horizon {
class RuleClearanceSilkscreenExposedCopper : public Rule {
public:
    RuleClearanceSilkscreenExposedCopper();
    RuleClearanceSilkscreenExposedCopper(const json &j, const RuleImportMap &import_map);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr, class IPool *pool = nullptr) const override;

    uint64_t clearance_top = 0.1_mm;
    uint64_t clearance_bottom = 0.1_mm;
};
} // namespace horizon
