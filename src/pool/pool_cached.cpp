#include "pool_cached.hpp"
#include "entity.hpp"
#include "package.hpp"
#include "padstack.hpp"
#include "part.hpp"
#include "symbol.hpp"
#include "unit.hpp"
#include <giomm/file.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

namespace horizon {
PoolCached::PoolCached(const std::string &bp, const std::string &cp) : Pool(bp), cache_path(cp)
{
}

std::string PoolCached::get_filename(ObjectType ty, const UUID &uu, UUID *pool_uuid_out)
{
    auto filename_cached = Glib::build_filename(cache_path, get_flat_filename(ty, uu));
    if (Glib::file_test(filename_cached, Glib::FILE_TEST_IS_REGULAR)) {
        if (pool_uuid_out) {
            try {                                          // may throw if item is not found
                Pool::get_filename(ty, uu, pool_uuid_out); // for pool uuid
            }
            catch (const std::runtime_error &e) {
                *pool_uuid_out = UUID();
            }
        }
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
    auto pkg = get_package(pkg_uuid);
    auto model = pkg->get_model(model_uuid);
    if (!model)
        return "";
    auto model_filename = model->filename;
    auto filename_cached = Glib::build_filename(cache_path, model_filename);
    if (Glib::file_test(filename_cached, Glib::FILE_TEST_IS_REGULAR)) {
        return filename_cached;
    }
    else {
        auto model_basename = Glib::path_get_basename(model_filename);
        auto model_dirname = Glib::path_get_dirname(model_filename);
        auto dest_dir = Glib::build_filename(cache_path, model_dirname);
        if (!Glib::file_test(dest_dir, Glib::FILE_TEST_IS_DIR)) {
            Gio::File::create_for_path(dest_dir)->make_directory_with_parents();
        }
        auto src = Gio::File::create_for_path(Glib::build_filename(get_base_path(), model_filename));
        auto dst = Gio::File::create_for_path(filename_cached);
        src->copy(dst);
        return src->get_path();
    }
}

} // namespace horizon
