#pragma once
#include "common/common.hpp"
#include "rules/rule.hpp"

namespace horizon {
class RuleSymbolChecks : public Rule {
public:
    RuleSymbolChecks();
    RuleSymbolChecks(const json &j);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr) const override;
};
} // namespace horizon
