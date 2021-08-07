#include "schematic.hpp"
#include "util/util.hpp"
#include "rules/cache.hpp"
#include "util/accumulator.hpp"

namespace horizon {
RulesCheckResult SchematicRules::check_single_pin_net(const class Schematic &sch, class RulesCheckCache &cache) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;
    auto &rule = rule_single_pin_net;
    auto &c = dynamic_cast<RulesCheckCacheNetPins &>(cache.get_cache(RulesCheckCacheID::NET_PINS));

    for (const auto &[net, pins] : c.get_net_pins()) {
        if (rule.include_unnamed || net->is_named()) {
            if (pins.size() == 1) {
                r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                auto &x = r.errors.back();
                auto &conn = pins.at(0);
                x.comment = "Net \"" + net->name + "\" only connected to " + conn.comp.refdes + conn.gate.suffix + "."
                            + conn.pin.primary_name;
                x.sheet = conn.sheet;
                x.location = conn.location;
                x.has_location = true;
            }
        }
    }
    r.update();
    return r;
}

RulesCheckResult SchematicRules::check(RuleID id, const Schematic &sch, RulesCheckCache &cache) const
{
    switch (id) {
    case RuleID::SINGLE_PIN_NET:
        return check_single_pin_net(sch, cache);

    default:
        return RulesCheckResult();
    }
}
} // namespace horizon
