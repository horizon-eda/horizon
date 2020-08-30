#pragma once
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include "rules/rules.hpp"
#include "rule_single_pin_net.hpp"

namespace horizon {
using json = nlohmann::json;

class SchematicRules : public Rules {
public:
    SchematicRules();

    void load_from_json(const json &j) override;
    RulesCheckResult check(RuleID id, const class Schematic &sch, class RulesCheckCache &cache) const;
    void apply(RuleID id, class Schematic *sch);
    json serialize() const override;
    std::set<RuleID> get_rule_ids() const override;
    const Rule *get_rule(RuleID id) const override;
    const Rule *get_rule(RuleID id, const UUID &uu) const override;
    std::map<UUID, const Rule *> get_rules(RuleID id) const override;
    void remove_rule(RuleID id, const UUID &uu) override;
    Rule *add_rule(RuleID id) override;

private:
    RuleSinglePinNet rule_single_pin_net;

    RulesCheckResult check_single_pin_net(const class Schematic &sch, class RulesCheckCache &cache) const;
};
} // namespace horizon
