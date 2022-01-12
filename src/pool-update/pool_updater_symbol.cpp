#include "pool_updater.hpp"
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include "util/util.hpp"
#include "pool/symbol.hpp"

namespace horizon {
void PoolUpdater::update_symbols(const std::string &directory, const std::string &prefix)
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        std::string filename = Glib::build_filename(directory, it);
        if (endswith(it, ".json")) {
            update_symbol(filename);
        }
        else if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
            update_symbols(filename, Glib::build_filename(prefix, it));
        }
    }
}

void PoolUpdater::update_symbol(const std::string &filename)
{
    try {
        status_cb(PoolUpdateStatus::FILE, filename, "");
        const auto rel = get_path_rel(filename);
        auto symbol = Symbol::new_from_file(filename, *pool);
        const auto last_pool_uuid = handle_override(ObjectType::SYMBOL, symbol.uuid);
        if (!last_pool_uuid)
            return;
        SQLite::Query q(pool->db,
                        "INSERT INTO symbols "
                        "(uuid, name, filename, mtime, unit, pool_uuid, last_pool_uuid) "
                        "VALUES "
                        "($uuid, $name, $filename, $mtime, $unit, $pool_uuid, $last_pool_uuid)");
        q.bind("$uuid", symbol.uuid);
        q.bind("$name", symbol.name);
        q.bind("$unit", symbol.unit->uuid);
        q.bind("$pool_uuid", pool_uuid);
        q.bind("$last_pool_uuid", *last_pool_uuid);
        q.bind("$filename", rel);
        q.bind_int64("$mtime", get_mtime(filename));
        q.step();
        add_dependency(ObjectType::SYMBOL, symbol.uuid, ObjectType::UNIT, symbol.unit->uuid);
    }
    catch (const std::exception &e) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, e.what());
    }
    catch (...) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, "unknown exception");
    }
}
} // namespace horizon
