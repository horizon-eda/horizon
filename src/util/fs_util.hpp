#pragma once
#include <string>
#include <optional>

namespace horizon {
std::optional<std::string> get_relative_filename(const std::string &path, const std::string &base);
}
