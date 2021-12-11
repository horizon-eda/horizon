#pragma once
#include <string>
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include <vector>
#include "util/file_version.hpp"

namespace horizon {
using json = nlohmann::json;

class PoolInfo {
public:
    static const UUID project_pool_uuid;
    PoolInfo(const std::string &bp);
    PoolInfo();
    PoolInfo(const json &j);
    std::string base_path;
    UUID uuid;
    UUID default_via;
    std::string name;
    std::vector<UUID> pools_included;
    void save() const;
    bool is_project_pool() const;
    bool is_usable() const;
    static unsigned int get_app_version();
    FileVersion version;

private:
    PoolInfo(const json &j, const std::string &bp);
};
} // namespace horizon
