#include "pool_cache_status.hpp"
#include "project_pool.hpp"
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include "util/util.hpp"
#include "pool_manager.hpp"
#include <filesystem>

namespace horizon {
namespace fs = std::filesystem;

PoolCacheStatus PoolCacheStatus::from_project_pool(class IPool &pool)
{
    PoolCacheStatus status;
    std::string query =
            "SELECT type, uuid, name, filename, last_pool_uuid FROM all_items_view WHERE pool_uuid = ? AND "
            "(last_pool_uuid != '00000000-0000-0000-0000-000000000000' ";
    for (const auto &[type, name] : Pool::type_names) {
        query += " OR filename GLOB '" + name + "/cache/*'";
    }
    query += ")";
    SQLite::Query q(pool.get_db(), query);
    q.bind(1, PoolInfo::project_pool_uuid);

    auto included_pools = pool.get_actually_included_pools(false);
    std::map<UUID, Pool> other_pools;

    for (auto &[bp, uu] : included_pools) {
        if (!other_pools.count(uu)) {
            if (auto other_pool_info = PoolManager::get().get_by_uuid(uu)) {
                other_pools.emplace(uu, other_pool_info->base_path);
            }
        }
    }

    while (q.step()) {
        const auto filename = q.get<std::string>(3);
        const auto type = q.get<ObjectType>(0);
        const auto name = q.get<std::string>(2);
        const UUID uuid = q.get<std::string>(1);
        const UUID last_pool_uuid = q.get<std::string>(4);
        auto itempath = Glib::build_filename(pool.get_base_path(), filename);
        json j_cache;
        {
            j_cache = load_json_from_file(itempath);
            if (j_cache.count("_imp"))
                j_cache.erase("_imp");
        }
        status.items.emplace_back();
        auto &item = status.items.back();
        status.n_total++;
        const auto type_str = j_cache.at("type").get<std::string>();
        item.filename_cached = itempath;
        item.name = name;
        item.type = type;
        item.uuid = uuid;
        item.pool_uuid = last_pool_uuid;

        std::string pool_filename;

        if (last_pool_uuid) {
            if (!other_pools.count(last_pool_uuid)) {
                throw std::runtime_error("pool " + (std::string)last_pool_uuid + " not found in included pools");
            }
            auto &other_pool = other_pools.at(last_pool_uuid);
            try {
                pool_filename = other_pool.get_filename(type, uuid);
            }
            catch (const SQLite::Error &e) {
                throw;
            }
            catch (...) {
                pool_filename = "";
            }
        }

        if (pool_filename.size() && Glib::file_test(pool_filename, Glib::FILE_TEST_IS_REGULAR)) {
            item.filename_pool = pool_filename;
            json j_pool;
            {
                j_pool = load_json_from_file(pool_filename);
                if (j_pool.count("_imp"))
                    j_pool.erase("_imp");
                if (type == ObjectType::PACKAGE) {
                    ProjectPool::patch_package(j_pool, last_pool_uuid);
                }
            }
            auto delta = json::diff(j_cache, j_pool);
            item.delta = delta;
            if (delta.size()) {
                item.state = PoolCacheStatus::Item::State::OUT_OF_DATE;
                status.n_out_of_date++;
            }
            else {
                item.state = PoolCacheStatus::Item::State::CURRENT;
                status.n_current++;
            }
        }
        else {
            item.state = PoolCacheStatus::Item::State::MISSING_IN_POOL;
            status.n_missing++;
        }
    }
    {
        const auto models_path = fs::u8path(pool.get_base_path()) / "3d_models" / "cache";
        if (fs::is_directory(models_path)) {
            for (const auto &it_pool_dir : fs::directory_iterator(models_path)) {
                const UUID pool_uuid(it_pool_dir.path().filename().string());
                for (const auto &filename_cached_entry : fs::recursive_directory_iterator(it_pool_dir)) {
                    if (filename_cached_entry.is_regular_file()) {
                        const auto filename_cached = filename_cached_entry.path();
                        status.items.emplace_back();
                        auto &item = status.items.back();
                        status.n_total++;
                        const auto filename_rel = fs::relative(filename_cached, it_pool_dir);
                        item.filename_cached = filename_cached.u8string();
                        item.name = filename_cached.filename().u8string();
                        item.type = ObjectType::MODEL_3D;
                        item.uuid = UUID::random();
                        item.pool_uuid = pool_uuid;

                        if (other_pools.count(pool_uuid)) {
                            const auto this_pool_base_path = fs::u8path(other_pools.at(pool_uuid).get_base_path());
                            const auto filename_pool = this_pool_base_path / filename_rel;
                            item.filename_pool = filename_pool.u8string();
                            if (fs::is_regular_file(filename_pool)) {
                                if (compare_files(filename_cached.u8string(), filename_pool.u8string())) {
                                    item.state = PoolCacheStatus::Item::State::CURRENT;
                                    status.n_current++;
                                }
                                else {
                                    item.state = PoolCacheStatus::Item::State::OUT_OF_DATE;
                                    status.n_out_of_date++;
                                }
                            }
                            else {
                                item.state = PoolCacheStatus::Item::State::MISSING_IN_POOL;
                                status.n_missing++;
                            }
                        }
                        else {
                            item.state = PoolCacheStatus::Item::State::MISSING_IN_POOL;
                            status.n_missing++;
                        }
                    }
                }
            }
        }
    }
    return status;
}
} // namespace horizon
