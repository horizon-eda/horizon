#pragma once
#include "common/common.hpp"
#include "rules/rule.hpp"

namespace horizon {
class RulePreflightChecks : public Rule {
public:
    RulePreflightChecks();
    RulePreflightChecks(const json &j);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr) const;
};
} // namespace horizon
