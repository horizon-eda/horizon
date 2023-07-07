#include "pool_updater.hpp"
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include "pool/padstack.hpp"

namespace horizon {

void PoolUpdater::add_padstack(const Padstack &padstack, const UUID &pkg_uuid, const UUID &last_pool_uuid,
                               const std::string &filename, const std::string &filename_abs)
{
    SQLite::Query q(
            pool->db,
            "INSERT INTO padstacks "
            "(uuid, name, well_known_name, filename, mtime, package, type, pool_uuid, last_pool_uuid) "
            "VALUES "
            "($uuid, $name, $well_known_name, $filename, $mtime, $package, $type, $pool_uuid, $last_pool_uuid)");
    q.bind("$uuid", padstack.uuid);
    q.bind("$name", padstack.name);
    q.bind("$well_known_name", padstack.well_known_name);
    q.bind("$type", Padstack::type_lut.lookup_reverse(padstack.type));
    q.bind("$package", pkg_uuid);
    q.bind("$pool_uuid", pool_uuid);
    q.bind("$last_pool_uuid", last_pool_uuid);
    q.bind("$filename", filename);
    q.bind_int64("$mtime", get_mtime(filename_abs));
    q.step();
}

void PoolUpdater::update_padstacks(const std::string &directory, const std::string &prefix)
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        auto pkgpath = Glib::build_filename(directory, it);
        auto pkgfilename = Glib::build_filename(pkgpath, "package.json");
        if (Glib::file_test(pkgfilename, Glib::FileTest::FILE_TEST_IS_REGULAR)) {
            auto pkg_filename = Glib::build_filename(pkgpath, "package.json");
            UUID pkg_uuid;

            // we'll have to parse the package manually, since we don't have
            // padstacks yet
            try {
                json j = load_json_from_file(pkg_filename);
                pkg_uuid = j.at("uuid").get<std::string>();
            }
            catch (const std::exception &e) {
                status_cb(PoolUpdateStatus::FILE_ERROR, pkg_filename, std::string(e.what()) + " skipping padstacks");
            }
            catch (...) {
                status_cb(PoolUpdateStatus::FILE_ERROR, pkg_filename, "unknown exception, skipping padstacks");
            }
            if (pkg_uuid) {
                auto padstacks_path = Glib::build_filename(pkgpath, "padstacks");
                if (Glib::file_test(padstacks_path, Glib::FileTest::FILE_TEST_IS_DIR)) {
                    Glib::Dir dir2(padstacks_path);
                    for (const auto &it2 : dir2) {
                        if (endswith(it2, ".json")) {
                            std::string filename = Glib::build_filename(padstacks_path, it2);
                            try {
                                status_cb(PoolUpdateStatus::FILE, filename, "");
                                auto padstack = Padstack::new_from_file(filename);
                                const auto rel = Glib::build_filename("packages", prefix, it, "padstacks", it2);
                                const auto last_pool_uuid = handle_override(ObjectType::PADSTACK, padstack.uuid, rel);
                                if (!last_pool_uuid)
                                    throw std::runtime_error("shouldn't happen in complete pool update");
                                add_padstack(padstack, pkg_uuid, *last_pool_uuid, rel, filename);
                            }
                            catch (const std::exception &e) {
                                status_cb(PoolUpdateStatus::FILE_ERROR, filename, e.what());
                            }
                            catch (...) {
                                status_cb(PoolUpdateStatus::FILE_ERROR, filename, "unknown exception");
                            }
                        }
                    }
                }
            }
        }
        else if (Glib::file_test(pkgpath, Glib::FILE_TEST_IS_DIR)) {
            update_padstacks(pkgpath, Glib::build_filename(prefix, it));
        }
    }
}

void PoolUpdater::update_padstacks_global(const std::string &directory, const std::string &prefix)
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        std::string filename = Glib::build_filename(directory, it);
        if (endswith(it, ".json")) {
            try {
                status_cb(PoolUpdateStatus::FILE, filename, "");
                const auto rel = Glib::build_filename("padstacks", prefix, it);
                auto padstack = Padstack::new_from_file(filename);
                const auto last_pool_uuid = handle_override(ObjectType::PADSTACK, padstack.uuid, rel);
                if (!last_pool_uuid)
                    throw std::runtime_error("shouldn't happen in complete pool update");
                add_padstack(padstack, UUID(), *last_pool_uuid, rel, filename);
            }
            catch (const std::exception &e) {
                status_cb(PoolUpdateStatus::FILE_ERROR, filename, e.what());
            }
            catch (...) {
                status_cb(PoolUpdateStatus::FILE_ERROR, filename, "unknown exception");
            }
        }
        else if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
            update_padstacks_global(filename, Glib::build_filename(prefix, it));
        }
    }
}

void PoolUpdater::update_padstack(const std::string &filename)
{
    try {
        status_cb(PoolUpdateStatus::FILE, filename, "");
        auto padstack = Padstack::new_from_file(filename);
        UUID package_uuid;
        auto ps_dir = Glib::path_get_dirname(filename);
        if (Glib::path_get_basename(ps_dir) == "padstacks") {
            auto pkg_dir = Glib::path_get_dirname(ps_dir);
            auto pkg_json = Glib::build_filename(pkg_dir, "package.json");
            if (Glib::file_test(pkg_json, Glib::FILE_TEST_IS_REGULAR)) {
                auto j = load_json_from_file(pkg_json);
                package_uuid = j.at("uuid").get<std::string>();
            }
        }
        const auto rel = get_path_rel(filename);
        const auto last_pool_uuid = handle_override(ObjectType::PADSTACK, padstack.uuid, rel);
        if (!last_pool_uuid)
            return;
        add_padstack(padstack, package_uuid, *last_pool_uuid, rel, filename);
    }
    catch (const std::exception &e) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, e.what());
    }
    catch (const CompletePoolUpdateRequiredException &e) {
        throw;
    }
    catch (...) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, "unknown exception");
    }
}

} // namespace horizon
