#pragma once
#include "nlohmann/json_fwd.hpp"
#include "rule_symbol_checks.hpp"
#include "rules/rules.hpp"
#include "util/uuid.hpp"

namespace horizon {
using json = nlohmann::json;

class SymbolRules : public Rules {
public:
    SymbolRules();

    void load_from_json(const json &j) override;
    RulesCheckResult check(RuleID id, const class Symbol &sym) const;
    json serialize() const override;
    std::set<RuleID> get_rule_ids() const override;
    const Rule *get_rule(RuleID id) const override;
    const Rule *get_rule(RuleID id, const UUID &uu) const override;
    std::map<UUID, const Rule *> get_rules(RuleID id) const override;
    void remove_rule(RuleID id, const UUID &uu) override;
    Rule *add_rule(RuleID id) override;

private:
    RuleSymbolChecks rule_symbol_checks;

    RulesCheckResult check_symbol(const class Symbol &sym) const;
};
} // namespace horizon
