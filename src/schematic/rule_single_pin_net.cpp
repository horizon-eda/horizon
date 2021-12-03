#include "rule_single_pin_net.hpp"
#include "util/util.hpp"
#include <sstream>
#include "nlohmann/json.hpp"

namespace horizon {
RuleSinglePinNet::RuleSinglePinNet() : Rule()
{
}

RuleSinglePinNet::RuleSinglePinNet(const json &j) : Rule(j)
{
    include_unnamed = j.at("include_unnamed").get<bool>();
}

json RuleSinglePinNet::serialize() const
{
    json j = Rule::serialize();
    j["include_unnamed"] = include_unnamed;
    return j;
}

std::string RuleSinglePinNet::get_brief(const class Block *block, class IPool *pool) const
{
    return "";
}
} // namespace horizon
