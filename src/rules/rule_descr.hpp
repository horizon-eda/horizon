#pragma once
#include "rule.hpp"
#include <string>

namespace horizon {
class RuleDescription {
public:
    RuleDescription(const std::string &n, bool m, bool c, bool a = false)
        : name(n), multi(m), can_check(c), needs_match_all(a)
    {
    }

    std::string name;
    bool multi;
    bool can_check;
    bool needs_match_all;
};

extern const std::map<RuleID, RuleDescription> rule_descriptions;
} // namespace horizon
