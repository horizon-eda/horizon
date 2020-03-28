#include "pool_cache_monitor.hpp"
#include <glibmm/main.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include "pool/pool_cached.hpp"
#include "pool/pool_manager.hpp"
#include "util/util.hpp"
#include "pool/part.hpp"

namespace horizon {
PoolCacheMonitor::PoolCacheMonitor(const std::string &a_pool_base_path, const std::string &a_pool_cache_path)
    : pool_base_path(a_pool_base_path), pool_cache_path(a_pool_cache_path)
{
    monitor = Gio::File::create_for_path(pool_cache_path)->monitor_directory();
    monitor->signal_changed().connect(sigc::mem_fun(*this, &PoolCacheMonitor::changed));
    dispatcher.connect([this] {
        std::lock_guard<std::mutex> guard(mutex);
        status = status_thread;
        thread.join();
        s_signal_changed.emit();
    });
}

const PoolCacheStatus &PoolCacheMonitor::get_status() const
{
    return status;
}


void PoolCacheMonitor::changed(const Glib::RefPtr<Gio::File> &file, const Glib::RefPtr<Gio::File> &file_other,
                               Gio::FileMonitorEvent event)
{
    update();
}

bool PoolCacheMonitor::really_update()
{
    if (!thread.joinable()) {
        thread = std::thread(&PoolCacheMonitor::worker, this);
    }
    return false;
}

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

void PoolCacheMonitor::worker()
{
    {
        std::lock_guard<std::mutex> guard(mutex);
        status_thread.items.clear();
        status_thread.n_current = 0;
        status_thread.n_total = 0;
        status_thread.n_out_of_date = 0;
        status_thread.n_missing = 0;

        Pool pool(pool_base_path);
        PoolCached pool_cached(pool_base_path, pool_cache_path);


        Glib::Dir dir(pool_cache_path);
        try {
            for (const auto &it : dir) {
                if (endswith(it, ".json")) {
                    auto itempath = Glib::build_filename(pool_cache_path, it);
                    json j_cache;
                    {
                        j_cache = load_json_from_file(itempath);
                        if (j_cache.count("_imp"))
                            j_cache.erase("_imp");
                    }
                    status_thread.items.emplace_back();
                    auto &item = status_thread.items.back();
                    status_thread.n_total++;
                    std::string type_str = j_cache.at("type");
                    item.filename_cached = itempath;
                    if (type_str == "part") {
                        try {
                            auto part = Part::new_from_file(itempath, pool_cached);
                            item.name = part.get_MPN() + " / " + part.get_manufacturer();
                        }
                        catch (...) {
                            item.name = "Part not found";
                        }
                    }
                    else {
                        item.name = j_cache.at("name").get<std::string>();
                    }
                    ObjectType type = object_type_lut.lookup(type_str);
                    item.type = type;

                    UUID uuid(j_cache.at("uuid").get<std::string>());
                    item.uuid = uuid;

                    std::string pool_filename;
                    try {
                        pool_filename = pool.get_filename(type, uuid);
                    }
                    catch (const SQLite::Error &e) {
                        throw;
                    }
                    catch (...) {
                        pool_filename = "";
                    }
                    if (pool_filename.size() && Glib::file_test(pool_filename, Glib::FILE_TEST_IS_REGULAR)) {
                        json j_pool;
                        {
                            j_pool = load_json_from_file(pool_filename);
                            if (j_pool.count("_imp"))
                                j_pool.erase("_imp");
                        }
                        auto delta = json::diff(j_cache, j_pool);
                        item.delta = delta;
                        if (delta.size()) {
                            item.state = PoolCacheStatus::Item::State::OUT_OF_DATE;
                            status_thread.n_out_of_date++;
                        }
                        else {
                            item.state = PoolCacheStatus::Item::State::CURRENT;
                            status_thread.n_current++;
                        }
                    }
                    else {
                        item.state = PoolCacheStatus::Item::State::MISSING_IN_POOL;
                        status_thread.n_missing++;
                    }
                }
            }
            {
                auto models_path = Glib::build_filename(pool_cache_path, "3d_models");
                if (Glib::file_test(models_path, Glib::FILE_TEST_IS_DIR)) {
                    Glib::Dir dir_3d(models_path);
                    for (const auto &it : dir_3d) {
                        if (it.find("pool_") == 0 && it.size() == 41) {
                            UUID pool_uuid(it.substr(5));
                            auto it_pool = PoolManager::get().get_by_uuid(pool_uuid);
                            if (it_pool) {
                                auto &this_pool_base_path = it_pool->base_path;
                                auto this_pool_cache_path = Glib::build_filename(models_path, it);
                                find_files(this_pool_cache_path, [this, &this_pool_base_path,
                                                                  &this_pool_cache_path](const std::string &path) {
                                    status_thread.items.emplace_back();
                                    auto &item = status_thread.items.back();
                                    status_thread.n_total++;
                                    auto filename_cached = Glib::build_filename(this_pool_cache_path, path);
                                    auto filename_pool = Glib::build_filename(this_pool_base_path, path);
                                    item.filename_cached = path;
                                    item.name = Glib::path_get_basename(path);
                                    item.type = ObjectType::MODEL_3D;
                                    item.uuid = UUID::random();
                                    if (Glib::file_test(filename_pool, Glib::FILE_TEST_IS_REGULAR)) {
                                        if (compare_files(filename_cached, filename_pool)) {
                                            item.state = PoolCacheStatus::Item::State::CURRENT;
                                            status_thread.n_current++;
                                        }
                                        else {
                                            item.state = PoolCacheStatus::Item::State::OUT_OF_DATE;
                                            status_thread.n_out_of_date++;
                                        }
                                    }
                                    else {
                                        item.state = PoolCacheStatus::Item::State::MISSING_IN_POOL;
                                        status_thread.n_missing++;
                                    }
                                });
                            }
                        }
                    }
                }
            }
        }
        catch (const SQLite::Error &e) {
            status_thread.items.clear();
            status_thread.n_total = -1;
        }
    }
    dispatcher.emit();
}

void PoolCacheMonitor::update()
{
    timer_connection.disconnect();
    timer_connection =
            Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &PoolCacheMonitor::really_update), 1);
}

void PoolCacheMonitor::update_now()
{
    timer_connection.disconnect();
    really_update();
}

PoolCacheMonitor::~PoolCacheMonitor()
{
    if (thread.joinable())
        thread.join();
}

} // namespace horizon
