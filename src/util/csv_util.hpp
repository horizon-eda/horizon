#pragma once
#include <string>

namespace horizon {
bool needs_quote(const std::string &s);
std::string escape_csv(const std::string &s);
} // namespace horizon
