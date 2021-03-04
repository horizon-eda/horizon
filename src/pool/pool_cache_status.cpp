#include "pool_cache_status.hpp"
#include "project_pool.hpp"
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include "util/util.hpp"
#include "pool_manager.hpp"

namespace horizon {

static void find_files(const std::string &base_path, std::function<void(const std::string &)> cb,
                       const std::string &path = "")
{
    auto this_path = Glib::build_filename(base_path, path);
    Glib::Dir dir(this_path);
    for (const auto &it : dir) {
        auto itempath = Glib::build_filename(this_path, it);
        if (Glib::file_test(itempath, Glib::FILE_TEST_IS_DIR)) {
            find_files(base_path, cb, Glib::build_filename(path, it));
        }
        else if (Glib::file_test(itempath, Glib::FILE_TEST_IS_REGULAR)) {
            cb(Glib::build_filename(path, it));
        }
    }
}

PoolCacheStatus PoolCacheStatus::from_project_pool(class IPool &pool)
{
    PoolCacheStatus status;
    SQLite::Query q(pool.get_db(),
                    "SELECT type, uuid, name, filename, last_pool_uuid FROM all_items_view WHERE pool_uuid = ? AND "
                    "last_pool_uuid != '00000000-0000-0000-0000-000000000000'");
    q.bind(1, PoolInfo::project_pool_uuid);
    std::map<UUID, Pool> other_pools;
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
        std::string type_str = j_cache.at("type");
        item.filename_cached = itempath;
        item.name = name;
        item.type = type;
        item.uuid = uuid;
        item.pool_uuid = last_pool_uuid;

        std::string pool_filename;

        if (!other_pools.count(last_pool_uuid)) {
            if (auto other_pool_info = PoolManager::get().get_by_uuid(last_pool_uuid)) {
                other_pools.emplace(last_pool_uuid, other_pool_info->base_path);
            }
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
    }
    {
        auto models_path = Glib::build_filename(pool.get_base_path(), "3d_models", "cache");
        if (Glib::file_test(models_path, Glib::FILE_TEST_IS_DIR)) {
            Glib::Dir dir_3d(models_path);
            for (const auto &it : dir_3d) {
                UUID pool_uuid(it);
                auto it_pool = PoolManager::get().get_by_uuid(pool_uuid);
                if (it_pool) {
                    auto &this_pool_base_path = it_pool->base_path;
                    auto this_pool_cache_path = Glib::build_filename(models_path, it);
                    find_files(this_pool_cache_path, [&this_pool_base_path, &this_pool_cache_path, &status,
                                                      &pool_uuid](const std::string &path) {
                        status.items.emplace_back();
                        auto &item = status.items.back();
                        status.n_total++;
                        auto filename_cached = Glib::build_filename(this_pool_cache_path, path);
                        auto filename_pool = Glib::build_filename(this_pool_base_path, path);
                        item.filename_cached = filename_cached;
                        item.filename_pool = filename_pool;
                        item.name = Glib::path_get_basename(path);
                        item.type = ObjectType::MODEL_3D;
                        item.uuid = UUID::random();
                        item.pool_uuid = pool_uuid;
                        if (Glib::file_test(filename_pool, Glib::FILE_TEST_IS_REGULAR)) {
                            if (compare_files(filename_cached, filename_pool)) {
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
                    });
                }
            }
        }
    }
    return status;
}
} // namespace horizon
