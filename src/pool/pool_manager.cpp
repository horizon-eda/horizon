#include "pool_manager.hpp"
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include "util/util.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
static PoolManager *the_pool_manager = nullptr;

PoolManager &PoolManager::get()
{
    if (!the_pool_manager)
        throw std::runtime_error("call PoolManager::init() first");
    return *the_pool_manager;
}

void PoolManager::init()
{
    if (the_pool_manager == nullptr) {
        the_pool_manager = new PoolManager();
    }
}

PoolManagerPool::PoolManagerPool(const std::string &bp) : base_path(bp)
{
    auto pool_json = Glib::build_filename(base_path, "pool.json");
    auto j = load_json_from_file(pool_json);
    uuid = j.at("uuid").get<std::string>();
    default_via = j.at("default_via").get<std::string>();
    name = j.at("name").get<std::string>();
    if (j.count("pools_included")) {
        auto &o = j.at("pools_included");
        for (auto &it : o) {
            pools_included.emplace_back(it.get<std::string>());
        }
    }
}

void PoolManagerPool::save()
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


PoolManager::PoolManager()
{
    auto pool_prj_mgr_config = Glib::build_filename(get_config_dir(), "pool-project-manager.json");
    auto pool_config = Glib::build_filename(get_config_dir(), "pools.json");
    if (Glib::file_test(pool_config, Glib::FILE_TEST_IS_REGULAR)) {
        auto j = load_json_from_file(pool_config);
        if (j.count("pools")) {
            auto o = j.at("pools");
            for (auto it = o.cbegin(); it != o.cend(); ++it) {
                std::string pool_base_path = it.key();
                bool enabled = it.value();
                if (Glib::file_test(Glib::build_filename(pool_base_path, "pool.json"), Glib::FILE_TEST_IS_REGULAR)) {
                    pools.emplace(std::piecewise_construct, std::forward_as_tuple(pool_base_path),
                                  std::forward_as_tuple(pool_base_path));
                    set_pool_enabled_no_write(pool_base_path, enabled);
                }
            }
        }
    }
    else if (Glib::file_test(pool_prj_mgr_config, Glib::FILE_TEST_IS_REGULAR)) {
        auto j = load_json_from_file(pool_prj_mgr_config);
        if (j.count("pools")) {
            auto o = j.at("pools");
            for (auto it = o.cbegin(); it != o.cend(); ++it) {
                std::string pool_base_path = Glib::path_get_dirname(it.value());
                if (Glib::file_test(Glib::build_filename(pool_base_path, "pool.json"), Glib::FILE_TEST_IS_REGULAR)) {
                    pools.emplace(std::piecewise_construct, std::forward_as_tuple(pool_base_path),
                                  std::forward_as_tuple(pool_base_path));
                }
            }
            for (auto &it : pools) {
                set_pool_enabled_no_write(it.first, true);
            }
            write();
        }
    }
}

void PoolManager::set_pool_enabled(const std::string &base_path, bool enabled)
{
    set_pool_enabled_no_write(base_path, enabled);
    write();
    s_signal_changed.emit();
}

void PoolManager::set_pool_enabled_no_write(const std::string &base_path, bool enabled)
{
    if (enabled) { // disable conflicting pools
        auto uu = pools.at(base_path).uuid;
        for (auto &it : pools) {
            if (it.second.uuid == uu) {
                it.second.enabled = false;
            }
        }
    }
    pools.at(base_path).enabled = enabled;
}

void PoolManager::add_pool(const std::string &base_path)
{
    if (pools.count(base_path))
        return;
    pools.emplace(std::piecewise_construct, std::forward_as_tuple(base_path), std::forward_as_tuple(base_path));
    set_pool_enabled(base_path, true);
}

void PoolManager::remove_pool(const std::string &base_path)
{
    if (!pools.count(base_path))
        return;
    pools.erase(base_path);
    write();
    s_signal_changed.emit();
}

void PoolManager::write()
{
    json o;
    for (const auto &it : pools) {
        o[it.first] = it.second.enabled;
    }
    json j;
    j["pools"] = o;
    save_json_to_file(Glib::build_filename(get_config_dir(), "pools.json"), j);
}

void PoolManager::update_pool(const std::string &base_path, const PoolManagerPool &settings)
{
    auto &p = pools.at(base_path);
    p.name = settings.name;
    p.default_via = settings.default_via;
    p.pools_included = settings.pools_included;
    p.save();
    s_signal_changed.emit();
}

const std::map<std::string, PoolManagerPool> &PoolManager::get_pools() const
{
    return pools;
}

const PoolManagerPool *PoolManager::get_by_uuid(const UUID &uu) const
{
    for (const auto &it : pools) {
        if (it.second.enabled && it.second.uuid == uu)
            return &it.second;
    }
    return nullptr;
}

} // namespace horizon
