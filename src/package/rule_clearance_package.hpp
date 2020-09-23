#pragma once
#include "common/common.hpp"
#include "rules/rule.hpp"

namespace horizon {
class RuleClearancePackage : public Rule {
public:
    RuleClearancePackage();
    RuleClearancePackage(const json &j);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr) const override;

    uint64_t clearance_silkscreen_cu = 0.2_mm;
    uint64_t clearance_silkscreen_pkg = 0.2_mm;
};
} // namespace horizon
