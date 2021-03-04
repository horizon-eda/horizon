#pragma once
#include <string>
#include "util/uuid.hpp"
#include <vector>

namespace horizon {
class PoolInfo {
public:
    PoolInfo(const std::string &bp);
    std::string base_path;
    UUID uuid;
    UUID default_via;
    std::string name;
    std::vector<UUID> pools_included;
    void save() const;
};
} // namespace horizon
