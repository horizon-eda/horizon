#include "check_util.hpp"
#include <glibmm/regex.h>
#include "rules/rule.hpp"
#include "rules/rules.hpp"

namespace horizon {
bool needs_trim(const std::string &s)
{
    return s.size() && (isspace(s.front()) || isspace(s.back()));
}

bool check_tag(const std::string &s, RulesCheckResult &r)
{
    Glib::ustring us(s);
    static auto re = Glib::Regex::create("^[a-z-0-9.]+$");
    if (!re->match(us)) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL,
                              "Tag \"" + s + "\" must only contain lowercase letters, digits, dots or dashes");
        return false;
    }
    return true;
}

bool check_prefix(const std::string &s, RulesCheckResult &r)
{
    const Glib::ustring up(s);
    if (!Glib::Regex::match_simple("^[A-Z]+$", up)) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Prefix must be one or more capital letters");
        return false;
    }
    return true;
}

void accumulate_level(RulesCheckErrorLevel &r, RulesCheckErrorLevel l)
{
    r = static_cast<RulesCheckErrorLevel>(std::max(static_cast<int>(r), static_cast<int>(l)));
}

} // namespace horizon
