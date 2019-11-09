#pragma once
#include "common/common.hpp"
#include "rules/rule.hpp"
#include "rules/rule_match.hpp"

namespace horizon {
class RuleTrackWidth : public Rule {
public:
    class Widths {
    public:
        Widths();
        Widths(const json &j);
        json serialize() const;

        uint64_t min = .1_mm;
        uint64_t max = 10_mm;
        uint64_t def = .2_mm;
    };

    RuleTrackWidth(const UUID &uu);
    RuleTrackWidth(const UUID &uu, const json &j);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr) const override;
    bool is_match_all() const override;

    RuleMatch match;
    std::map<int, Widths> widths;
};
} // namespace horizon
