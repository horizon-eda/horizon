#pragma once
#include "common/common.hpp"
#include "plane.hpp"
#include "rules/rule.hpp"
#include "rules/rule_match.hpp"

namespace horizon {
class RulePlane : public Rule {
public:
    RulePlane(const UUID &uu);
    RulePlane(const UUID &uu, const json &j);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr) const override;
    bool is_match_all() const override;

    RuleMatch match;
    int layer = 10000;

    PlaneSettings settings;
};
} // namespace horizon
