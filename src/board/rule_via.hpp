#pragma once
#include "common/common.hpp"
#include "parameter/set.hpp"
#include "rules/rule.hpp"
#include "rules/rule_match.hpp"

namespace horizon {
class RuleVia : public Rule {
public:
    RuleVia(const UUID &uu);
    RuleVia(const UUID &uu, const json &j, const RuleImportMap &import_map);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr) const override;
    bool is_match_all() const override;
    bool can_export() const override;

    RuleMatch match;
    UUID padstack;
    ParameterSet parameter_set;
};
} // namespace horizon
