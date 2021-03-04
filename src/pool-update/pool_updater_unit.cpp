#include "pool_updater.hpp"
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
            update_unit(filename, false);
        }
        else if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
            update_units(filename, Glib::build_filename(prefix, it));
        }
    }
}

void PoolUpdater::update_unit(const std::string &filename, bool partial)
{
    try {
        status_cb(PoolUpdateStatus::FILE, filename, "");
        auto unit = Unit::new_from_file(filename);
        bool overridden = false;
        if (exists(ObjectType::UNIT, unit.uuid)) {
            overridden = true;
            delete_item(ObjectType::UNIT, unit.uuid);
        }
        if (partial)
            overridden = false;

        SQLite::Query q(pool->db,
                        "INSERT INTO units "
                        "(uuid, name, manufacturer, filename, pool_uuid, overridden) "
                        "VALUES "
                        "($uuid, $name, $manufacturer, $filename, $pool_uuid, $overridden)");
        q.bind("$uuid", unit.uuid);
        q.bind("$name", unit.name);
        q.bind("$manufacturer", unit.manufacturer);
        q.bind("$filename", get_path_rel(filename));
        q.bind("$pool_uuid", pool_uuid);
        q.bind("$overridden", overridden);
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
