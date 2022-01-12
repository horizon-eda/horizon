#include "schematic.hpp"
#include "util/util.hpp"
#include "rules/cache.hpp"
#include "util/accumulator.hpp"

namespace horizon {
RulesCheckResult SchematicRules::check_connectivity(const class Schematic &sch, class RulesCheckCache &cache) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;
    auto &rule = rule_connectivity;
    auto &c = dynamic_cast<RulesCheckCacheNetPins &>(cache.get_cache(RulesCheckCacheID::NET_PINS));

    for (const auto &[net_uu, it] : c.get_net_pins()) {
        if (rule.include_unnamed || it.name.size()) {
            if (it.pins.size() == 1 && !it.is_nc) {
                r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                auto &x = r.errors.back();
                auto &conn = it.pins.front();
                std::string refdes;
                if (conn.instance_path.size())
                    refdes = sch.block->get_component_info(conn.comp, conn.instance_path).refdes;
                else
                    refdes = sch.block->components.at(conn.comp).refdes;
                x.comment = "Net \"" + it.name + "\" only connected to " + refdes + conn.gate.suffix + "."
                            + conn.pin.primary_name;
                x.sheet = conn.sheet;
                x.instance_path = conn.instance_path;
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
    case RuleID::CONNECTIVITY:
        return check_connectivity(sch, cache);

    default:
        return RulesCheckResult();
    }
}
} // namespace horizon
