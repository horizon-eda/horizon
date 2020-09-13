#include "symbol_rules.hpp"
#include "pool/symbol.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
SymbolRules::SymbolRules()
{
}

void SymbolRules::load_from_json(const json &j)
{
    if (j.count("symbol_checks")) {
        const json &o = j["symbol_checks"];
        rule_symbol_checks = RuleSymbolChecks(o);
    }
}

json SymbolRules::serialize() const
{
    json j;
    j["symbol_checks"] = rule_symbol_checks.serialize();

    return j;
}

std::set<RuleID> SymbolRules::get_rule_ids() const
{
    return {RuleID::SYMBOL_CHECKS};
}

const Rule *SymbolRules::get_rule(RuleID id) const
{
    if (id == RuleID::SYMBOL_CHECKS) {
        return &rule_symbol_checks;
    }
    return nullptr;
}

const Rule *SymbolRules::get_rule(RuleID id, const UUID &uu) const
{
    return nullptr;
}

std::map<UUID, const Rule *> SymbolRules::get_rules(RuleID id) const
{
    std::map<UUID, const Rule *> r;
    switch (id) {
    default:;
    }
    return r;
}

void SymbolRules::remove_rule(RuleID id, const UUID &uu)
{
    switch (id) {
    default:;
    }
    fix_order(id);
}

Rule *SymbolRules::add_rule(RuleID id)
{
    Rule *r = nullptr;
    switch (id) {
    default:
        return nullptr;
    }
    return r;
}
} // namespace horizon
