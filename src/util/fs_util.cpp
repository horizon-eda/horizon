#include "fs_util.hpp"
#include <filesystem>
#include <algorithm>

namespace horizon {
namespace fs = std::filesystem;
std::optional<std::string> get_relative_filename(const std::string &path, const std::string &base)
{
    const auto p = fs::u8path(path);
    const auto b = fs::u8path(base);
    for (auto ip = p.begin(), ib = b.begin(); ib != b.end(); ip++, ib++) {
        if (ip == p.end() || *ip != *ib)
            return {};
    }
    return fs::relative(p, b).u8string();
}
} // namespace horizon
