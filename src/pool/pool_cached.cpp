#include "pool_cached.hpp"
#include "pool_manager.hpp"
#include "entity.hpp"
#include "package.hpp"
#include "padstack.hpp"
#include "part.hpp"
#include "symbol.hpp"
#include "unit.hpp"
#include "util/util.hpp"
#include <giomm/file.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

namespace horizon {
PoolCached::PoolCached(const std::string &bp, const std::string &cp) : Pool(bp), cache_path(cp)
{
    if (Glib::file_test(Glib::build_filename(cache_path, "3d_models"), Glib::FILE_TEST_IS_DIR)) {
        bool old_3d_models = false;
        {
            Glib::Dir dir(Glib::build_filename(cache_path, "3d_models"));
            for (const auto &it : dir) {
                old_3d_models = true;
                if (it.find("pool_") == 0) {
                    old_3d_models = false;
                }
            }
        }
        const auto &this_pool = PoolManager::get().get_pools().at(get_base_path());
        main_pool_uuid = this_pool.uuid;
        if (old_3d_models) {
            auto models_path = Glib::build_filename(cache_path, "3d_models");
            auto fn =
                    Glib::build_filename(models_path, "pool_" + static_cast<std::string>(this_pool.uuid), "3d_models");
            auto dest_dir = Gio::File::create_for_path(fn);
            dest_dir->make_directory_with_parents();
            Glib::Dir dir(models_path);
            std::vector<std::string> entries(dir.begin(), dir.end());
            for (const auto &it : entries) {
                if (it.find("pool_") == std::string::npos) {
                    auto src = Gio::File::create_for_path(Glib::build_filename(models_path, it));
                    auto dst = Gio::File::create_for_path(Glib::build_filename(fn, it));
                    src->move(dst);
                }
            }
        }
    }
}

std::string PoolCached::get_filename(ObjectType ty, const UUID &uu, UUID *pool_uuid_out)
{
    auto filename_cached = Glib::build_filename(cache_path, get_flat_filename(ty, uu));
    if (Glib::file_test(filename_cached, Glib::FILE_TEST_IS_REGULAR)) {
        auto key = std::make_pair(ty, uu);
        if (pool_uuid_cache.count(key) == 0) {
            UUID pool_uuid;
            try {                                       // may throw if item is not found
                Pool::get_filename(ty, uu, &pool_uuid); // for pool uuid
            }
            catch (const std::runtime_error &e) {
                pool_uuid = main_pool_uuid;
            }
            pool_uuid_cache[key] = pool_uuid;
        }
        get_pool_uuid(ty, uu, pool_uuid_out);
        return filename_cached;
    }
    else {
        auto fn = Pool::get_filename(ty, uu, pool_uuid_out);
        auto src = Gio::File::create_for_path(fn);
        auto dst = Gio::File::create_for_path(filename_cached);
        src->copy(dst);
        return fn;
    }
}

std::string PoolCached::get_model_filename(const UUID &pkg_uuid, const UUID &model_uuid)
{
    UUID pool_uuid;
    auto pkg = get_package(pkg_uuid, &pool_uuid);
    auto model = pkg->get_model(model_uuid);
    if (!model)
        return "";
    auto pool = PoolManager::get().get_by_uuid(pool_uuid);
    if (!pool)
        return "";
    auto model_filename = model->filename;
    auto model_filename_complete = Glib::build_filename(pool->base_path, model_filename);
    auto filename_cached = Glib::build_filename(cache_path, "3d_models", "pool_" + static_cast<std::string>(pool_uuid),
                                                model_filename);
    if (Glib::file_test(filename_cached, Glib::FILE_TEST_IS_REGULAR)) {
        return filename_cached;
    }
    else if (Glib::file_test(model_filename_complete, Glib::FILE_TEST_IS_REGULAR)) {
        auto model_dirname = Glib::path_get_dirname(model_filename);
        auto dest_dir = Glib::path_get_dirname(filename_cached);
        if (!Glib::file_test(dest_dir, Glib::FILE_TEST_IS_DIR)) {
            Gio::File::create_for_path(dest_dir)->make_directory_with_parents();
        }
        auto src = Gio::File::create_for_path(model_filename_complete);
        auto dst = Gio::File::create_for_path(filename_cached);
        src->copy(dst);
        return src->get_path();
    }
    else {
        return "";
    }
}

const std::string &PoolCached::get_cache_path() const
{
    return cache_path;
}

} // namespace horizon
