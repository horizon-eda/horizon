#pragma once
#include "common/common.hpp"
#include "rules/rule.hpp"

namespace horizon {
class RuleLayerPair : public Rule {
public:
    RuleLayerPair(const UUID &uu);
    RuleLayerPair(const UUID &uu, const json &j);
    json serialize() const override;

    std::string get_brief(const class Block *block = nullptr) const override;

    RuleMatch match;
    std::pair<int, int> layers;
};
} // namespace horizon
