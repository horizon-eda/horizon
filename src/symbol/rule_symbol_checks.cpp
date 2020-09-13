#include "rule_symbol_checks.hpp"
#include "util/util.hpp"
#include <sstream>
#include "nlohmann/json.hpp"

namespace horizon {
RuleSymbolChecks::RuleSymbolChecks() : Rule()
{
    id = RuleID::SYMBOL_CHECKS;
}

RuleSymbolChecks::RuleSymbolChecks(const json &j) : Rule(j)
{
    id = RuleID::SYMBOL_CHECKS;
}

json RuleSymbolChecks::serialize() const
{
    json j = Rule::serialize();
    return j;
}

std::string RuleSymbolChecks::get_brief(const class Block *block) const
{
    return "";
}
} // namespace horizon
