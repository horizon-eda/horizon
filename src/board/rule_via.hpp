#pragma once
#include "common/common.hpp"
#include "parameter/set.hpp"
#include "rules/rule.hpp"
#include "rules/rule_match.hpp"

namespace horizon {
class RuleVia : public Rule {
public:
    static const auto id = RuleID::VIA;
    RuleID get_id() const override
    {
        return id;
    }

    RuleVia(const UUID &uu);
    RuleVia(const UUID &uu, const json &j, const RuleImportMap &import_map);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr, class IPool *pool = nullptr) const override;
    bool is_match_all() const override;
    bool can_export() const override;

    RuleMatch match;

    enum class Mode { SET_PADSTACK_AND_PARAMETERS, CHECK_DEFINITION };

    Mode mode = Mode::SET_PADSTACK_AND_PARAMETERS;

    UUID padstack;
    ParameterSet parameter_set;

    std::set<UUID> definitions;
};
} // namespace horizon
