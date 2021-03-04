#include "pool_info.hpp"
#include "util/util.hpp"
#include <glibmm/miscutils.h>
#include "nlohmann/json.hpp"

namespace horizon {

const UUID PoolInfo::project_pool_uuid = UUID("466088f9-3f15-420d-af8a-fff902537aed");

PoolInfo::PoolInfo(const std::string &bp) : base_path(bp)
{
    auto pool_json = Glib::build_filename(base_path, "pool.json");
    auto j = load_json_from_file(pool_json);
    from_json(j);
}

void PoolInfo::from_json(const json &j)
{
    uuid = j.at("uuid").get<std::string>();
    default_via = j.at("default_via").get<std::string>();
    name = j.at("name");
    if (j.count("pools_included")) {
        auto &o = j.at("pools_included");
        for (auto &it : o) {
            pools_included.emplace_back(it.get<std::string>());
        }
    }
}

PoolInfo::PoolInfo()
{
}

bool PoolInfo::is_project_pool() const
{
    return uuid == project_pool_uuid;
}

void PoolInfo::save() const
{
    json j;
    j["uuid"] = (std::string)uuid;
    j["default_via"] = (std::string)default_via;
    j["name"] = name;
    j["type"] = "pool";
    json o = json::array();
    for (const auto &it : pools_included) {
        o.push_back((std::string)it);
    }
    j["pools_included"] = o;
    save_json_to_file(Glib::build_filename(base_path, "pool.json"), j);
}

} // namespace horizon
