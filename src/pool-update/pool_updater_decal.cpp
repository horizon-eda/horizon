#include "pool_updater.hpp"
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include "util/util.hpp"

namespace horizon {
void PoolUpdater::update_decals(const std::string &directory, const std::string &prefix)
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        std::string filename = Glib::build_filename(directory, it);
        if (endswith(it, ".json")) {
            update_decal(filename);
        }
        else if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
            update_decals(filename, Glib::build_filename(prefix, it));
        }
    }
}

void PoolUpdater::update_decal(const std::string &filename)
{
    try {
        status_cb(PoolUpdateStatus::FILE, filename, "");
        const auto rel = get_path_rel(filename);
        auto decal = Decal::new_from_file(filename);
        const auto last_pool_uuid = handle_override(ObjectType::DECAL, decal.uuid);
        if (!last_pool_uuid)
            return;
        SQLite::Query q(pool->db,
                        "INSERT INTO DECALS "
                        "(uuid, name, filename, mtime, pool_uuid, last_pool_uuid) "
                        "VALUES "
                        "($uuid, $name, $filename, $mtime, $pool_uuid, $last_pool_uuid)");
        q.bind("$uuid", decal.uuid);
        q.bind("$name", decal.name);
        q.bind("$filename", rel);
        q.bind_int64("$mtime", get_mtime(filename));
        q.bind("$pool_uuid", pool_uuid);
        q.bind("$last_pool_uuid", *last_pool_uuid);
        q.step();
    }
    catch (const std::exception &e) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, e.what());
    }
    catch (...) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, "unknown exception");
    }
}
} // namespace horizon
