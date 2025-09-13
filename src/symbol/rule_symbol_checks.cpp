#include "rule_symbol_checks.hpp"
#include "util/util.hpp"
#include <sstream>
#include <nlohmann/json.hpp>

namespace horizon {
RuleSymbolChecks::RuleSymbolChecks() : Rule()
{
}

RuleSymbolChecks::RuleSymbolChecks(const json &j) : Rule(j)
{
}

json RuleSymbolChecks::serialize() const
{
    json j = Rule::serialize();
    return j;
}

std::string RuleSymbolChecks::get_brief(const class Block *block, class IPool *pool) const
{
    return "";
}
} // namespace horizon
