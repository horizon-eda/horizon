#pragma once
#include "common/common.hpp"
#include "rules/rule.hpp"
#include "via_definition.hpp"

namespace horizon {
class RuleViaDefinitions : public Rule {
public:
    static const auto id = RuleID::VIA_DEFINITIONS;
    RuleID get_id() const override
    {
        return id;
    }

    RuleViaDefinitions();
    RuleViaDefinitions(const json &j, const RuleImportMap &import_map);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr, class IPool *pool = nullptr) const override;
    bool can_export() const override
    {
        return true;
    }

    std::map<UUID, ViaDefinition> via_definitions;
};
} // namespace horizon
