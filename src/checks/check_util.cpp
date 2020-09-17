#include "check_util.hpp"
#include <glibmm/regex.h>

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
} // namespace horizon
