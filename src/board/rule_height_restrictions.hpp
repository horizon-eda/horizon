#pragma once
#include "rules/rule.hpp"

namespace horizon {
class RuleHeightRestrictions : public Rule {
public:
    static const auto id = RuleID::HEIGHT_RESTRICTIONS;
    RuleID get_id() const override
    {
        return id;
    }

    RuleHeightRestrictions();
    RuleHeightRestrictions(const json &j);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr, class IPool *pool = nullptr) const override;
};
} // namespace horizon
