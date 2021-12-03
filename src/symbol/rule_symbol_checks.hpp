#pragma once
#include "common/common.hpp"
#include "rules/rule.hpp"

namespace horizon {
class RuleSymbolChecks : public Rule {
public:
    static const auto id = RuleID::SYMBOL_CHECKS;
    RuleID get_id() const override
    {
        return id;
    }

    RuleSymbolChecks();
    RuleSymbolChecks(const json &j);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr, class IPool *pool = nullptr) const override;
};
} // namespace horizon
