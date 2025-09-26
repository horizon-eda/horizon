#include "rule_via_definitions.hpp"
#include "util/util.hpp"
#include <sstream>
#include <nlohmann/json.hpp>
#include "logger/log_util.hpp"

namespace horizon {
RuleViaDefinitions::RuleViaDefinitions() : Rule()
{
}

RuleViaDefinitions::RuleViaDefinitions(const json &j, const RuleImportMap &import_map) : Rule(j, import_map)
{
    for (const auto &[k, v] : j.at("via_definitions").items()) {
        const UUID uu(k);
        load_and_log(via_definitions, "Via definition", std::forward_as_tuple(uu, v));
    }
}

json RuleViaDefinitions::serialize() const
{
    json j = Rule::serialize();
    json o = json::object();
    for (const auto &[uu, it] : via_definitions) {
        o[(std::string)uu] = it.serialize();
    }
    j["via_definitions"] = o;
    return j;
}

std::string RuleViaDefinitions::get_brief(const class Block *block, class IPool *pool) const
{
    return "";
}
} // namespace horizon
