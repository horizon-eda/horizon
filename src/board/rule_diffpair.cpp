#include "rule_diffpair.hpp"
#include "block/block.hpp"
#include "util/util.hpp"
#include <sstream>
#include "nlohmann/json.hpp"

namespace horizon {
RuleDiffpair::RuleDiffpair(const UUID &uu) : Rule(uu)
{
}

RuleDiffpair::RuleDiffpair(const UUID &uu, const json &j, const RuleImportMap &import_map)
    : Rule(uu, j, import_map), net_class(import_map.get_net_class(j.at("net_class").get<std::string>())),
      layer(j.at("layer")), track_width(j.at("track_width")), track_gap(j.at("track_gap")), via_gap(j.at("via_gap"))
{
}

json RuleDiffpair::serialize() const
{
    json j = Rule::serialize();
    j["net_class"] = (std::string)net_class;
    j["layer"] = layer;
    j["track_width"] = track_width;
    j["track_gap"] = track_gap;
    j["via_gap"] = via_gap;
    return j;
}

std::string RuleDiffpair::get_brief(const class Block *block, class IPool *pool) const
{
    return "Net class " + (net_class ? block->net_classes.at(net_class).name : "?");
}
} // namespace horizon
