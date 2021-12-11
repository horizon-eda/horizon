#include "pool_info.hpp"
#include "util/util.hpp"
#include <glibmm/miscutils.h>
#include "nlohmann/json.hpp"
#include "logger/logger.hpp"
#include "pool/pool.hpp"
#include "util/installation_uuid.hpp"

namespace horizon {

const UUID PoolInfo::project_pool_uuid = UUID("466088f9-3f15-420d-af8a-fff902537aed");

const unsigned int app_version = 0;

unsigned int PoolInfo::get_app_version()
{
    return app_version;
}

PoolInfo::PoolInfo(const std::string &bp) : PoolInfo(load_json_from_file(Glib::build_filename(bp, "pool.json")), bp)
{
}

PoolInfo::PoolInfo(const json &j) : PoolInfo(j, "")
{
}

PoolInfo::PoolInfo(const json &j, const std::string &bp)
    : base_path(bp), uuid(j.at("uuid").get<std::string>()), default_via(j.at("default_via").get<std::string>()),
      name(j.at("name")), version(app_version, j)
{
    if (j.count("pools_included")) {
        auto &o = j.at("pools_included");
        for (auto &it : o) {
            pools_included.emplace_back(it.get<std::string>());
        }
    }
}

PoolInfo::PoolInfo() : version(app_version)
{
}

bool PoolInfo::is_project_pool() const
{
    return uuid == project_pool_uuid;
}

void PoolInfo::save() const
{
    if (version.get_file() > version.get_app()) {
        Logger::log_warning("Won't save pool info", Logger::Domain::VERSION);
        return;
    }
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
    version.serialize(j);
    save_json_to_file(Glib::build_filename(base_path, "pool.json"), j);
}

bool PoolInfo::is_usable() const
{
    try {
        Pool pool(base_path);
        return pool.db.get_user_version() == Pool::get_required_schema_version()
               && pool.get_installation_uuid() == InstallationUUID::get();
    }
    catch (SQLite::Error &) {
        return false;
    }
}


} // namespace horizon
