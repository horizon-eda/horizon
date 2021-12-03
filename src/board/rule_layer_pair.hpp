#pragma once
#include "common/common.hpp"
#include "rules/rule.hpp"

namespace horizon {
class RuleLayerPair : public Rule {
public:
    static const auto id = RuleID::LAYER_PAIR;
    RuleID get_id() const override
    {
        return id;
    }

    RuleLayerPair(const UUID &uu);
    RuleLayerPair(const UUID &uu, const json &j, const RuleImportMap &import_map);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr, class IPool *pool = nullptr) const override;
    bool can_export() const override;

    RuleMatch match;
    std::pair<int, int> layers;
};
} // namespace horizon
