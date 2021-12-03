#pragma once
#include "common/common.hpp"
#include "rules/rule.hpp"
#include "rules/rule_match.hpp"
#include "rules/rule_match_component.hpp"
#include <set>

namespace horizon {
class RuleShortedPads : public Rule {
public:
    static const auto id = RuleID::SHORTED_PADS;
    RuleID get_id() const override
    {
        return id;
    }

    RuleShortedPads(const UUID &uu);
    RuleShortedPads(const UUID &uu, const json &j);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr, class IPool *pool = nullptr) const override;
    bool can_export() const override;

    bool matches(const class Component *component, const class Net *net) const;

    RuleMatch match;
    RuleMatchComponent match_component;
};
} // namespace horizon
