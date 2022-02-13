#pragma once
#include "common/common.hpp"
#include "rules/rule.hpp"

namespace horizon {
class RuleNetTies : public Rule {
public:
    static const auto id = RuleID::NET_TIES;
    RuleID get_id() const override
    {
        return id;
    }

    RuleNetTies();
    RuleNetTies(const json &j);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr, class IPool *pool = nullptr) const override;
};
} // namespace horizon
