#include "rule_layer_pair.hpp"
#include "util/util.hpp"
#include <sstream>
#include "nlohmann/json.hpp"

namespace horizon {

RuleLayerPair::RuleLayerPair(const UUID &uu) : Rule(uu)
{
    id = RuleID::LAYER_PAIR;
}

RuleLayerPair::RuleLayerPair(const UUID &uu, const json &j) : Rule(uu, j), match(j.at("match")), layers(j.at("layers"))
{
    id = RuleID::LAYER_PAIR;
}

json RuleLayerPair::serialize() const
{
    json j = Rule::serialize();
    j["match"] = match.serialize();
    j["layers"] = layers;
    return j;
}

std::string RuleLayerPair::get_brief(const class Block *block) const
{
    return "Match " + match.get_brief(block);
}

} // namespace horizon
