#pragma once
#include <string>
#include "util/uuid.hpp"
#include <vector>

namespace horizon {
class PoolInfo {
public:
    static const UUID project_pool_uuid;
    PoolInfo(const std::string &bp);
    PoolInfo();
    std::string base_path;
    UUID uuid;
    UUID default_via;
    std::string name;
    std::vector<UUID> pools_included;
    void save() const;
    bool is_project_pool() const;
};
} // namespace horizon
