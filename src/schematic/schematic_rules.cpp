#include "schematic_rules.hpp"
#include "schematic.hpp"
#include "util/util.hpp"
#include "rules/cache.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
SchematicRules::SchematicRules()
{
}

void SchematicRules::load_from_json(const json &j)
{
    if (j.count("single_pin_net")) {
        const json &o = j["single_pin_net"];
        rule_single_pin_net = RuleSinglePinNet(o);
    }
}

void SchematicRules::apply(RuleID id, Schematic *sch)
{
}

json SchematicRules::serialize() const
{
    json j;
    j["single_pin_net"] = rule_single_pin_net.serialize();

    return j;
}

std::vector<RuleID> SchematicRules::get_rule_ids() const
{
    return {RuleID::SINGLE_PIN_NET};
}

const Rule &SchematicRules::get_rule(RuleID id) const
{
    if (id == RuleID::SINGLE_PIN_NET) {
        return rule_single_pin_net;
    }
    throw std::runtime_error("rule does not exist");
}

const Rule &SchematicRules::get_rule(RuleID id, const UUID &uu) const
{
    throw std::runtime_error("rule does not exist");
}

std::map<UUID, const Rule *> SchematicRules::get_rules(RuleID id) const
{
    std::map<UUID, const Rule *> r;
    switch (id) {
    default:;
    }
    return r;
}

void SchematicRules::remove_rule(RuleID id, const UUID &uu)
{
    switch (id) {
    default:;
    }
    fix_order(id);
}

Rule &SchematicRules::add_rule(RuleID id)
{
    throw std::runtime_error("not implemented");
}
} // namespace horizon
