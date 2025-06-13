#pragma once
#include "pool.hpp"
#include <nlohmann/json_fwd.hpp>

namespace horizon {
using json = nlohmann::json;
class ProjectPool : public Pool {
public:
    ProjectPool(const std::string &base, bool cache);
    std::string get_filename(ObjectType type, const UUID &uu, UUID *pool_uuid_out) override;
    std::string get_model_filename(const UUID &pkg_uuid, const UUID &model_uuid) override;

    static void create_directories(const std::string &base_path);
    static std::map<UUID, std::string> patch_package(json &j, const UUID &pool_uuid);

private:
    const bool is_caching;
    void update_model_filename(const UUID &pkg_uuid, const UUID &model_uuid, const std::string &new_filename);
};
} // namespace horizon
