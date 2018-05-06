#pragma once
#include "rule.hpp"
#include <string>

namespace horizon {
class RuleDescription {
public:
    RuleDescription(const std::string &n, bool m, bool c) : name(n), multi(m), can_check(c)
    {
    }

    std::string name;
    bool multi;
    bool can_check;
};

extern const std::map<RuleID, RuleDescription> rule_descriptions;
} // namespace horizon
