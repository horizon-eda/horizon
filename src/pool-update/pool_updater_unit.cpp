#include "pool_updater.hpp"
#include "pool/unit.hpp"
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include "util/util.hpp"

namespace horizon {
void PoolUpdater::update_units(const std::string &directory, const std::string &prefix)
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        std::string filename = Glib::build_filename(directory, it);
        if (endswith(it, ".json")) {
            update_unit(filename);
        }
        else if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
            update_units(filename, Glib::build_filename(prefix, it));
        }
    }
}

void PoolUpdater::update_unit(const std::string &filename)
{
    try {
        status_cb(PoolUpdateStatus::FILE, filename, "");
        auto unit = Unit::new_from_file(filename);
        const auto last_pool_uuid = handle_override(ObjectType::UNIT, unit.uuid);
        if (!last_pool_uuid)
            return;
        SQLite::Query q(pool->db,
                        "INSERT INTO units "
                        "(uuid, name, manufacturer, filename, pool_uuid, last_pool_uuid) "
                        "VALUES "
                        "($uuid, $name, $manufacturer, $filename, $pool_uuid, $last_pool_uuid)");
        q.bind("$uuid", unit.uuid);
        q.bind("$name", unit.name);
        q.bind("$manufacturer", unit.manufacturer);
        q.bind("$filename", get_path_rel(filename));
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
