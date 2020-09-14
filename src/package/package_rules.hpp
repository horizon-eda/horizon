#pragma once
#include "nlohmann/json_fwd.hpp"
#include "rule_package_checks.hpp"
#include "rules/rules.hpp"
#include "util/uuid.hpp"

namespace horizon {
using json = nlohmann::json;

class PackageRules : public Rules {
public:
    PackageRules();

    void load_from_json(const json &j) override;
    RulesCheckResult check(RuleID id, const class Package &pkg) const;
    json serialize() const override;
    std::set<RuleID> get_rule_ids() const override;
    const Rule *get_rule(RuleID id) const override;
    const Rule *get_rule(RuleID id, const UUID &uu) const override;
    std::map<UUID, const Rule *> get_rules(RuleID id) const override;
    void remove_rule(RuleID id, const UUID &uu) override;
    Rule *add_rule(RuleID id) override;

private:
    RulePackageChecks rule_package_checks;

    RulesCheckResult check_package(const class Package &pkg) const;
};
} // namespace horizon
