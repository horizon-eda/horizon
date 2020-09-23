#pragma once
#include "rule.hpp"
#include <string>

namespace horizon {
class RuleDescription {
public:
    enum Flag {
        IS_MULTI = (1 << 0),
        CAN_CHECK = (1 << 1),
        CAN_APPLY = (1 << 2),
        NEEDS_MATCH_ALL = (1 << 3),
    };

    RuleDescription(const std::string &n, unsigned int flags)
        : name(n), multi(flags & IS_MULTI), can_check(flags & CAN_CHECK), can_apply(flags & CAN_APPLY),
          needs_match_all(flags & NEEDS_MATCH_ALL)
    {
    }

    std::string name;
    bool multi;
    bool can_check;
    bool can_apply;
    bool needs_match_all;
};

extern const std::map<RuleID, RuleDescription> rule_descriptions;
} // namespace horizon
