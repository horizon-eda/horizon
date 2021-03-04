#include "fs_util.hpp"
#include <filesystem>
#include <algorithm>

namespace horizon {
namespace fs = std::filesystem;
std::optional<std::string> get_relative_filename(const std::string &path, const std::string &base)
{
    const auto p = fs::u8path(path);
    const auto b = fs::u8path(base);
    const auto x = std::search(p.begin(), p.end(), b.begin(), b.end());
    if (x == p.begin()) // path is in base path
        return fs::relative(p, b).u8string();
    else
        return {};
}
} // namespace horizon
