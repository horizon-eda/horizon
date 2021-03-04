#include "pool_updater.hpp"
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include "util/util.hpp"
#include "pool/entity.hpp"

namespace horizon {

void PoolUpdater::update_entities(const std::string &directory, const std::string &prefix)
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        std::string filename = Glib::build_filename(directory, it);
        if (endswith(it, ".json")) {
            update_entity(filename, false);
        }
        else if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
            update_entities(filename, Glib::build_filename(prefix, it));
        }
    }
}

void PoolUpdater::update_entity(const std::string &filename, bool partial)
{
    try {
        status_cb(PoolUpdateStatus::FILE, filename, "");
        auto entity = Entity::new_from_file(filename, *pool);
        bool overridden = false;
        if (exists(ObjectType::ENTITY, entity.uuid)) {
            overridden = true;
            delete_item(ObjectType::ENTITY, entity.uuid);
        }
        if (partial)
            overridden = false;
        SQLite::Query q(pool->db,
                        "INSERT INTO entities "
                        "(uuid, name, manufacturer, filename, n_gates, prefix, pool_uuid, overridden) "
                        "VALUES "
                        "($uuid, $name, $manufacturer, $filename, $n_gates, $prefix, $pool_uuid, $overridden)");
        q.bind("$uuid", entity.uuid);
        q.bind("$name", entity.name);
        q.bind("$manufacturer", entity.manufacturer);
        q.bind("$n_gates", entity.gates.size());
        q.bind("$prefix", entity.prefix);
        q.bind("$pool_uuid", pool_uuid);
        q.bind("$overridden", overridden);
        q.bind("$filename", get_path_rel(filename));
        q.step();
        for (const auto &it_tag : entity.tags) {
            add_tag(ObjectType::ENTITY, entity.uuid, it_tag);
        }
        for (const auto &it_gate : entity.gates) {
            add_dependency(ObjectType::ENTITY, entity.uuid, ObjectType::UNIT, it_gate.second.unit->uuid);
        }
    }
    catch (const std::exception &e) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, e.what());
    }
    catch (...) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, "unknown exception");
    }
}
} // namespace horizon
