#include "check_util.hpp"
#include <glibmm/regex.h>
#include "rules/rule.hpp"

namespace horizon {
bool needs_trim(const std::string &s)
{
    return s.size() && (isspace(s.front()) || isspace(s.back()));
}

bool check_tag(const std::string &s)
{
    Glib::ustring us(s);
    static auto re = Glib::Regex::create("^[a-z-0-9.]+$");
    return re->match(us);
}

bool check_prefix(const std::string &s)
{
    const Glib::ustring up(s);
    return Glib::Regex::match_simple("^[A-Z]+$", up);
}

void accumulate_level(RulesCheckErrorLevel &r, RulesCheckErrorLevel l)
{
    r = static_cast<RulesCheckErrorLevel>(std::max(static_cast<int>(r), static_cast<int>(l)));
}

} // namespace horizon
