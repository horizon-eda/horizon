#include "check_item.hpp"
#include "pool/ipool.hpp"
#include "check_unit.hpp"
#include "check_entity.hpp"
#include "check_part.hpp"
#include "pool/symbol.hpp"
#include "pool/package.hpp"
#include "check_util.hpp"

namespace horizon {

RulesCheckResult check_item(IPool &pool, ObjectType type, const UUID &uu)
{
    switch (type) {
    case ObjectType::UNIT:
        return check_unit(*pool.get_unit(uu));

    case ObjectType::ENTITY:
        return check_entity(*pool.get_entity(uu));

    case ObjectType::PART:
        return check_part(*pool.get_part(uu));

    case ObjectType::SYMBOL: {
        auto sym = *pool.get_symbol(uu);
        sym.expand();
        sym.apply_placement(Placement());
        return sym.rules.check(RuleID::SYMBOL_CHECKS, sym);
    }

    case ObjectType::PACKAGE: {
        auto pkg = *pool.get_package(uu);
        pkg.expand();
        pkg.apply_parameter_set({});
        RulesCheckResult result;

        const auto package_check_result = pkg.rules.check(RuleID::PACKAGE_CHECKS, pkg);
        result.errors = package_check_result.errors;
        accumulate_level(result.level, package_check_result.level);

        auto &rule_cl = dynamic_cast<RuleClearancePackage &>(*pkg.rules.get_rule_nc(RuleID::CLEARANCE_PACKAGE));
        rule_cl.clearance_silkscreen_cu = 0.2_mm;
        rule_cl.clearance_silkscreen_pkg = 0.2_mm;
        const auto r = pkg.rules.check(RuleID::CLEARANCE_PACKAGE, pkg);
        accumulate_level(result.level, r.level);
        result.errors.insert(result.errors.end(), r.errors.begin(), r.errors.end());

        return result;
    }

    default:
        return {};
    }
}
} // namespace horizon
