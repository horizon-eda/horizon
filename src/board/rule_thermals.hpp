#pragma once
#include "common/common.hpp"
#include "rules/rule.hpp"
#include "rules/rule_match.hpp"
#include "rules/rule_match_component.hpp"
#include "board/plane.hpp"
#include <set>

namespace horizon {
class RuleThermals : public Rule {
public:
    static const auto id = RuleID::THERMALS;
    RuleID get_id() const override
    {
        return id;
    }

    RuleThermals(const UUID &uu);
    RuleThermals(const UUID &uu, const json &j);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr, class IPool *pool = nullptr) const override;
    bool can_export() const override;

    bool matches(const class BoardPackage &pkg, const class Pad &pad, int layer) const;

    RuleMatch match;
    RuleMatchComponent match_component;
    int layer = 10000;

    enum class PadMode { ALL, PADS };
    PadMode pad_mode = PadMode::ALL;
    std::set<UUID> pads;

    ThermalSettings thermal_settings;
};
} // namespace horizon
