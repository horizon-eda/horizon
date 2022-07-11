#pragma once
#include <string>

namespace horizon {
bool needs_trim(const std::string &s);
bool check_tag(const std::string &s, class RulesCheckResult &r);
bool check_prefix(const std::string &s, class RulesCheckResult &r);
enum class RulesCheckErrorLevel;
void accumulate_level(RulesCheckErrorLevel &r, RulesCheckErrorLevel l);
} // namespace horizon
