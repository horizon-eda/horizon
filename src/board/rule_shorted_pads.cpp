#include "rule_shorted_pads.hpp"
#include "common/lut.hpp"
#include "util/util.hpp"
#include <sstream>
#include "nlohmann/json.hpp"

namespace horizon {

RuleShortedPads::RuleShortedPads(const UUID &uu) : Rule(uu)
{
}

RuleShortedPads::RuleShortedPads(const UUID &uu, const json &j)
    : Rule(uu, j), match(j.at("match")), match_component(j.at("match_component"))
{
}

json RuleShortedPads::serialize() const
{
    json j = Rule::serialize();
    j["match"] = match.serialize();
    j["match_component"] = match_component.serialize();
    return j;
}

bool RuleShortedPads::matches(const class Component *component, const class Net *net) const
{
    return enabled && match_component.matches(component) && match.match(net);
}

std::string RuleShortedPads::get_brief(const class Block *block, class IPool *pool) const
{
    std::stringstream ss;
    ss << "Match " << match_component.get_brief(block, pool) << "\n";
    ss << match.get_brief(block);
    return ss.str();
}

bool RuleShortedPads::can_export() const
{
    return match.can_export() && match_component.can_export();
}

} // namespace horizon
