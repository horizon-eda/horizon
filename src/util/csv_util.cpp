#include "csv_util.hpp"
#include <algorithm>

namespace horizon {

bool needs_quote(const std::string &s)
{
    return std::count(s.begin(), s.end(), ',') || std::count(s.begin(), s.end(), '"');
}

std::string escape_csv(const std::string &s)
{
    if (s.size() == 0)
        return "\"\"";

    std::string o;
    for (const auto &c : s) {
        if (c == '"')
            o += "\"\"";
        else
            o += c;
    }
    return o;
}
} // namespace horizon
