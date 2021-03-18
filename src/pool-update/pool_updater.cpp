#include "pool_updater.hpp"
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include <giomm/resource.h>
#include <giomm/file.h>
#include "nlohmann/json.hpp"
#include "pool/pool_manager.hpp"
#include "util/util.hpp"
#include "common/object_descr.hpp"
#include "util/fs_util.hpp"

namespace horizon {
PoolUpdater::PoolUpdater(const std::string &bp, pool_update_cb_t cb) : status_cb(cb)
{
    auto pool_db_path = Glib::build_filename(bp, "pool.db");
    status_cb(PoolUpdateStatus::INFO, "", "start");
    {
        SQLite::Database db(pool_db_path, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE);

        int user_version = db.get_user_version();
        if (user_version != Pool::get_required_schema_version()) {
            // update schema
            auto bytes = Gio::Resource::lookup_data_global("/org/horizon-eda/horizon/pool-update/schema.sql");
            gsize size{bytes->get_size() + 1}; // null byte
            auto data = (const char *)bytes->get_data(size);
            db.execute(data);
            status_cb(PoolUpdateStatus::INFO, "", "created db from schema");
        }
    }
    pool.emplace(bp, false);
    q_exists.emplace(pool->db, "SELECT pool_uuid, last_pool_uuid FROM all_items_view WHERE uuid = ? AND type = ?");
    q_add_dependency.emplace(pool->db, "INSERT INTO dependencies VALUES (?, ?, ?, ?)");
    q_insert_part.emplace(
            pool->db,
            "INSERT INTO parts "
            "(uuid, MPN, manufacturer, entity, package, description, filename, pool_uuid, last_pool_uuid, "
            "parametric_table, base) "
            "VALUES "
            "($uuid, $MPN, $manufacturer, $entity, $package, $description, $filename, $pool_uuid, "
            "$last_pool_uuid, $parametric_table, $base)");
    q_add_tag.emplace(pool->db,
                      "INSERT into tags (tag, uuid, type) "
                      "VALUES ($tag, $uuid, $type)");
    pool->db.execute("PRAGMA journal_mode=WAL");
    set_pool_info(bp);
}

std::optional<std::pair<UUID, UUID>> PoolUpdater::exists(ObjectType type, const UUID &uu)
{
    q_exists->reset();
    q_exists->bind(1, uu);
    q_exists->bind(2, type);
    if (q_exists->step())
        return std::make_pair(UUID(q_exists->get<std::string>(0)), UUID(q_exists->get<std::string>(1)));
    else
        return {};
}

void PoolUpdater::delete_item(ObjectType type, const UUID &uu)
{
    const char *query = nullptr;
    switch (type) {
    case ObjectType::UNIT:
        query = "DELETE FROM units WHERE uuid = ?";
        break;
    case ObjectType::DECAL:
        query = "DELETE FROM decals WHERE uuid = ?";
        break;
    case ObjectType::FRAME:
        query = "DELETE FROM frames WHERE uuid = ?";
        break;
    case ObjectType::PADSTACK:
        query = "DELETE FROM padstacks WHERE uuid = ?";
        break;
    case ObjectType::SYMBOL:
        query = "DELETE FROM symbols WHERE uuid = ?";
        break;
    case ObjectType::PACKAGE:
        query = "DELETE FROM packages WHERE uuid = ?";
        break;
    case ObjectType::PART:
        query = "DELETE FROM parts WHERE uuid = ?";
        break;
    case ObjectType::ENTITY:
        query = "DELETE FROM entities WHERE uuid = ?";
        break;
    default:
        throw std::runtime_error("can't delete " + object_descriptions.at(type).name);
    }
    {
        SQLite::Query q(pool->db, query);
        q.bind(1, uu);
        q.step();
    }
    switch (type) {
    case ObjectType::PACKAGE:
        clear_tags(ObjectType::PACKAGE, uu);
        clear_dependencies(ObjectType::PACKAGE, uu);
        {
            SQLite::Query q(pool->db, "DELETE FROM models WHERE package_uuid = ?");
            q.bind(1, uu);
            q.step();
        }
        break;

    case ObjectType::PART:
        clear_tags(ObjectType::PART, uu);
        clear_dependencies(ObjectType::PART, uu);
        {
            SQLite::Query q(pool->db, "DELETE FROM orderable_MPNs WHERE part = ?");
            q.bind(1, uu);
            q.step();
        }
        break;

    case ObjectType::ENTITY:
        clear_tags(ObjectType::ENTITY, uu);
        clear_dependencies(ObjectType::ENTITY, uu);
        break;

    default:;
    }
}

std::optional<UUID> PoolUpdater::handle_override(ObjectType type, const UUID &uu)
{
    std::optional<UUID> last_pool_uuid;
    if (auto l = exists(type, uu)) {
        const auto &[item_pool_uuid, item_last_pool_uuid] = *l;
        // pool_uuid is pool the item specified by type, uu is in

        if (is_partial_update) {
            if (item_pool_uuid == pool_uuid) {
                // updating existing item, keep last pool UUID
                last_pool_uuid = item_last_pool_uuid;
            }
            else {
                // updating existing item with item from other pool is not allowed
                // skip this item
                return {};
            }
        }
        else {
            if (item_pool_uuid != pool_uuid) {
                // item is overriding, i.e. item is in different pool than current item
                last_pool_uuid = item_pool_uuid;
            }
            else {
                throw std::runtime_error("duplicate UUID in complete pool update");
            }
        }
        delete_item(type, uu);
        return last_pool_uuid;
    }
    return UUID();
}

void PoolUpdater::set_pool_info(const std::string &bp)
{
    base_path = bp;
    const PoolInfo pool_info(bp);
    pool_uuid = pool_info.uuid;
}

std::string PoolUpdater::get_path_rel(const std::string &filename) const
{
    auto r = get_relative_filename(filename, base_path);
    if (!r)
        throw std::runtime_error(filename + " not in base path " + base_path);
    else
        return *r;
}


void PoolUpdater::update_some(const std::vector<std::string> &filenames, std::set<UUID> &all_parts_updated)
{
    is_partial_update = true;
    const auto base_paths = pool->get_actually_included_pools(true);
    pool->db.execute("BEGIN TRANSACTION");
    std::set<UUID> parts_updated;
    for (const auto &filename : filenames) {
        // find out base path
        pool_uuid = UUID();
        for (const auto &[bp, pool_uu] : base_paths) {
            if (get_relative_filename(filename, bp)) { // is in base_path
                base_path = bp;
                pool_uuid = pool_uu;
                break;
            }
        }
        if (!pool_uuid) {
            // not in any included pool, don't care
            continue;
        }

        get_path_rel(filename); // throws if filename not in current base path
        auto j = load_json_from_file(filename);
        auto type = object_type_lut.lookup(j.at("type"));
        switch (type) {
        case ObjectType::FRAME:
            update_frame(filename);
            break;
        case ObjectType::DECAL:
            update_decal(filename);
            break;
        case ObjectType::PART:
            update_part(filename);
            parts_updated.emplace(j.at("uuid").get<std::string>());
            all_parts_updated.emplace(j.at("uuid").get<std::string>());
            break;
        case ObjectType::UNIT:
            update_unit(filename);
            break;
        case ObjectType::SYMBOL:
            update_symbol(filename);
            break;
        case ObjectType::ENTITY:
            update_entity(filename);
            break;
        case ObjectType::PACKAGE:
            update_package(filename);
            break;
        case ObjectType::PADSTACK:
            update_padstack(filename);
            break;
        default:
            status_cb(PoolUpdateStatus::FILE_ERROR, filename, "unsupported type " + object_descriptions.at(type).name);
        }
    }
    pool->db.execute("COMMIT");
    pool->db.execute("BEGIN TRANSACTION");
    for (const auto &part_uu : parts_updated) {
        SQLite::Query q(pool->db,
                        "WITH RECURSIVE where_used(typex, uuidx) AS ( SELECT 'part', "
                        "? UNION "
                        "SELECT type, uuid FROM dependencies, where_used "
                        "WHERE dependencies.dep_type = where_used.typex "
                        "AND dependencies.dep_uuid = where_used.uuidx) "
                        "SELECT where_used.uuidx "
                        "FROM where_used WHERE where_used.typex = 'part';");
        q.bind(1, part_uu);
        q.step();
        while (q.step()) {
            UUID uuid = q.get<std::string>(0);
            all_parts_updated.insert(uuid);
            auto filename = pool->get_filename(ObjectType::PART, uuid);
            update_part(filename);
        }
    }
    pool->db.execute("COMMIT");
}

void PoolUpdater::update(const std::vector<std::string> &base_paths)
{
    status_cb(PoolUpdateStatus::INFO, "", "Tags");
    pool->db.execute("DELETE FROM tags");
    pool->db.execute("DELETE FROM dependencies");

    status_cb(PoolUpdateStatus::INFO, "", "Units");
    pool->db.execute("BEGIN TRANSACTION");
    pool->db.execute("DELETE FROM units");
    for (const auto &bp : base_paths) {
        set_pool_info(bp);
        update_units(Glib::build_filename(bp, "units"));
    }
    pool->db.execute("COMMIT");

    status_cb(PoolUpdateStatus::INFO, "", "Entities");
    pool->db.execute("BEGIN TRANSACTION");
    pool->db.execute("DELETE FROM entities");
    for (const auto &bp : base_paths) {
        set_pool_info(bp);
        update_entities(Glib::build_filename(bp, "entities"));
    }
    pool->db.execute("COMMIT");

    status_cb(PoolUpdateStatus::INFO, "", "Symbols");
    pool->db.execute("BEGIN TRANSACTION");
    pool->db.execute("DELETE FROM symbols");
    for (const auto &bp : base_paths) {
        set_pool_info(bp);
        update_symbols(Glib::build_filename(bp, "symbols"));
    }
    pool->db.execute("COMMIT");

    status_cb(PoolUpdateStatus::INFO, "", "Padstacks");
    pool->db.execute("BEGIN TRANSACTION");
    pool->db.execute("DELETE FROM padstacks");
    for (const auto &bp : base_paths) {
        set_pool_info(bp);
        update_padstacks_global(Glib::build_filename(bp, "padstacks"));
        update_padstacks(Glib::build_filename(bp, "packages"));
    }
    pool->db.execute("COMMIT");

    status_cb(PoolUpdateStatus::INFO, "", "Packages");
    pool->db.execute("BEGIN TRANSACTION");
    pool->db.execute("DELETE FROM packages");
    pool->db.execute("DELETE FROM models");
    for (const auto &bp : base_paths) {
        set_pool_info(bp);
        update_packages(Glib::build_filename(bp, "packages"));
    }
    pool->db.execute("COMMIT");

    status_cb(PoolUpdateStatus::INFO, "", "Parts");
    pool->db.execute("BEGIN TRANSACTION");
    pool->db.execute("DELETE FROM parts");
    pool->db.execute("DELETE FROM orderable_MPNs");
    for (const auto &bp : base_paths) {
        set_pool_info(bp);
        update_parts(Glib::build_filename(bp, "parts"));
    }
    pool->db.execute("COMMIT");

    status_cb(PoolUpdateStatus::INFO, "", "Frames");
    pool->db.execute("BEGIN TRANSACTION");
    pool->db.execute("DELETE FROM frames");
    for (const auto &bp : base_paths) {
        if (Glib::file_test(Glib::build_filename(bp, "frames"), Glib::FILE_TEST_IS_DIR)) {
            set_pool_info(bp);
            update_frames(Glib::build_filename(bp, "frames"));
        }
    }
    pool->db.execute("COMMIT");

    status_cb(PoolUpdateStatus::INFO, "", "Decals");
    pool->db.execute("BEGIN TRANSACTION");
    pool->db.execute("DELETE FROM decals");
    for (const auto &bp : base_paths) {
        if (Glib::file_test(Glib::build_filename(bp, "decals"), Glib::FILE_TEST_IS_DIR)) {
            set_pool_info(bp);
            update_decals(Glib::build_filename(bp, "decals"));
        }
    }
    pool->db.execute("COMMIT");
}

const json &PoolUpdater::load_json(const std::string &filename)
{
    if (!json_cache.count(filename))
        json_cache.emplace(filename, load_json_from_file(filename));
    return json_cache.at(filename);
}

void PoolUpdater::add_dependency(ObjectType type, const UUID &uu, ObjectType dep_type, const UUID &dep_uuid)
{
    q_add_dependency->reset();
    q_add_dependency->bind(1, type);
    q_add_dependency->bind(2, uu);
    q_add_dependency->bind(3, dep_type);
    q_add_dependency->bind(4, dep_uuid);
    q_add_dependency->step();
}

void PoolUpdater::add_tag(ObjectType type, const UUID &uu, const std::string &tag)
{
    q_add_tag->reset();
    q_add_tag->bind("$type", type);
    q_add_tag->bind("$uuid", uu);
    q_add_tag->bind("$tag", tag);
    q_add_tag->step();
}

void PoolUpdater::clear_tags(ObjectType type, const UUID &uu)
{
    {
        SQLite::Query q(pool->db, "DELETE FROM tags WHERE uuid = ? AND type = ?");
        q.bind(1, uu);
        q.bind(2, type);
        q.step();
    }
}

void PoolUpdater::clear_dependencies(ObjectType type, const UUID &uu)
{
    {
        SQLite::Query q(pool->db, "DELETE FROM dependencies WHERE uuid = ? AND type = ?");
        q.bind(1, uu);
        q.bind(2, type);
        q.step();
    }
}

} // namespace horizon
