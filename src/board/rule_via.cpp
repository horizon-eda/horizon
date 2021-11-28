#include "rule_via.hpp"
#include "util/util.hpp"
#include <sstream>
#include "nlohmann/json.hpp"

namespace horizon {
RuleVia::RuleVia(const UUID &uu) : Rule(uu)
{
    id = RuleID::VIA;
    parameter_set[ParameterID::VIA_DIAMETER] = .5_mm;
    parameter_set[ParameterID::HOLE_DIAMETER] = .2_mm;
}

RuleVia::RuleVia(const UUID &uu, const json &j, const RuleImportMap &import_map)
    : Rule(uu, j, import_map), match(j.at("match"), import_map), padstack(j.at("padstack").get<std::string>()),
      parameter_set(parameter_set_from_json(j.at("parameter_set")))
{
    id = RuleID::VIA;
}

json RuleVia::serialize() const
{
    json j = Rule::serialize();
    j["match"] = match.serialize();
    j["padstack"] = (std::string)padstack;
    j["parameter_set"] = parameter_set_serialize(parameter_set);
    return j;
}

std::string RuleVia::get_brief(const class Block *block, class IPool *pool) const
{
    return "Match " + match.get_brief(block);
}

bool RuleVia::is_match_all() const
{
    return match.mode == RuleMatch::Mode::ALL;
}

bool RuleVia::can_export() const
{
    return match.can_export();
}

} // namespace horizon
