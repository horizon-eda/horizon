#include "pool_manager.hpp"
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <giomm/file.h>
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include <filesystem>
#include "util/fs_util.hpp"

namespace horizon {
namespace fs = std::filesystem;
static PoolManager *the_pool_manager = nullptr;

static std::string get_abs_path(const std::string &rel)
{
    return fs::absolute(fs::u8path(rel)).u8string();
}

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

PoolManager::PoolManager()
{
    auto pool_prj_mgr_config = Glib::build_filename(get_config_dir(), "pool-project-manager.json");
    auto pool_config = Glib::build_filename(get_config_dir(), "pools.json");
    if (Glib::file_test(pool_config, Glib::FILE_TEST_IS_REGULAR)) {
        auto j = load_json_from_file(pool_config);
        if (j.count("pools")) {
            auto o = j.at("pools");
            for (const auto &[pool_base_path, value] : o.items()) {
                const auto enabled = value.get<bool>();
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
                std::string pool_base_path = Glib::path_get_dirname(it.value().get<std::string>());
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
    auto path = get_abs_path(base_path);
    set_pool_enabled_no_write(path, enabled);
    write();
}

void PoolManager::set_pool_enabled_no_write(const std::string &base_path, bool enabled)
{
    auto path = get_abs_path(base_path);
    if (enabled) { // disable conflicting pools
        auto uu = pools.at(path).uuid;
        for (auto &it : pools) {
            if (it.second.uuid == uu) {
                it.second.enabled = false;
            }
        }
    }
    pools.at(path).enabled = enabled;
}

void PoolManager::add_pool(const std::string &base_path)
{
    auto path = get_abs_path(base_path);
    if (pools.count(path))
        return;
    pools.emplace(std::piecewise_construct, std::forward_as_tuple(path), std::forward_as_tuple(path));
    set_pool_enabled(path, true);
}

void PoolManager::remove_pool(const std::string &base_path)
{
    auto path = get_abs_path(base_path);
    if (!pools.count(path))
        return;
    pools.erase(path);
    write();
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

void PoolManager::update_pool(const std::string &base_path)
{
    auto path = get_abs_path(base_path);
    auto &p = pools.at(path);
    PoolInfo settings(base_path);
    p.name = settings.name;
    p.default_via = settings.default_via;
    p.default_frame = settings.default_frame;
    p.pools_included = settings.pools_included;
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

const PoolManagerPool *PoolManager::get_for_file(const std::string &filename) const
{
    for (const auto &it : pools) {
        if (get_relative_filename(filename, it.second.base_path))
            return &it.second;
    }
    return nullptr;
}

} // namespace horizon
