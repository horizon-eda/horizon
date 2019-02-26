#include "pool-update.hpp"
#include "graph.hpp"
#include "pool/package.hpp"
#include "pool/part.hpp"
#include "pool/pool.hpp"
#include "pool/unit.hpp"
#include "pool/symbol.hpp"
#include "util/sqlite.hpp"
#include "util/util.hpp"
#include <giomm/file.h>
#include <giomm/resource.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"
#include "pool/pool_manager.hpp"

namespace horizon {

using json = nlohmann::json;

static void pkg_add_dir_to_graph(PoolUpdateGraph &graph, const std::string &directory, pool_update_cb_t status_cb)
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        auto pkgpath = Glib::build_filename(directory, it);
        auto pkgfilename = Glib::build_filename(pkgpath, "package.json");
        if (Glib::file_test(pkgfilename, Glib::FileTest::FILE_TEST_IS_REGULAR)) {
            std::string filename = Glib::build_filename(pkgpath, "package.json");
            try {
                std::ifstream ifs(filename);
                json j;
                if (!ifs.is_open()) {
                    throw std::runtime_error("package not opened");
                }
                ifs >> j;
                ifs.close();

                std::set<UUID> dependencies;
                UUID pkg_uuid = j.at("uuid").get<std::string>();
                if (j.count("alternate_for")) {
                    dependencies.emplace(j.at("alternate_for").get<std::string>());
                }
                graph.add_node(pkg_uuid, filename, dependencies);
            }
            catch (const std::exception &e) {
                status_cb(PoolUpdateStatus::FILE_ERROR, filename, e.what());
            }
            catch (...) {
                status_cb(PoolUpdateStatus::FILE_ERROR, filename, "unknown exception");
            }
        }
        else if (Glib::file_test(pkgpath, Glib::FILE_TEST_IS_DIR)) {
            pkg_add_dir_to_graph(graph, pkgpath, status_cb);
        }
    }
}

static void part_add_dir_to_graph(PoolUpdateGraph &graph, const std::string &directory, pool_update_cb_t status_cb)
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        std::string filename = Glib::build_filename(directory, it);
        if (endswith(it, ".json")) {
            try {
                std::ifstream ifs(filename);
                json j;
                if (!ifs.is_open()) {
                    throw std::runtime_error("part not opened");
                }
                ifs >> j;
                ifs.close();

                std::set<UUID> dependencies;
                UUID part_uuid = j.at("uuid").get<std::string>();
                if (j.count("base")) {
                    dependencies.emplace(j.at("base").get<std::string>());
                }
                graph.add_node(part_uuid, filename, dependencies);
            }
            catch (const std::exception &e) {
                status_cb(PoolUpdateStatus::FILE_ERROR, filename, e.what());
            }
            catch (...) {
                status_cb(PoolUpdateStatus::FILE_ERROR, filename, "unknown exception");
            }
        }
        else if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
            part_add_dir_to_graph(graph, filename, status_cb);
        }
    }
}

static void status_cb_nop(PoolUpdateStatus st, const std::string msg, const std::string filename)
{
}

class PoolUpdater {
public:
    PoolUpdater(const std::string &bp, pool_update_cb_t status_cb);
    void update(const std::vector<std::string> &base_paths);

private:
    std::unique_ptr<Pool> pool;
    std::string base_path;
    pool_update_cb_t status_cb;
    void update_frames(const std::string &directory, const std::string &prefix = "");
    void update_units(const std::string &directory, const std::string &prefix = "");
    void update_entities(const std::string &directory, const std::string &prefix = "");
    void update_symbols(const std::string &directory, const std::string &prefix = "");
    void update_padstacks(const std::string &directory, const std::string &prefix = "");
    void update_padstacks_global(const std::string &directory, const std::string &prefix = "");
    void update_packages(const std::string &directory);
    void update_package_node(const PoolUpdateNode &node, std::set<UUID> &visited);
    void update_parts(const std::string &directory);
    void update_part_node(const PoolUpdateNode &node, std::set<UUID> &visited);
    void add_dependency(ObjectType type, const UUID &uu, ObjectType dep_type, const UUID &dep_uuid);
    bool exists(ObjectType type, const UUID &uu);

    UUID pool_uuid;
    void set_pool_info(const std::string &bp);
};

bool PoolUpdater::exists(ObjectType type, const UUID &uu)
{
    SQLite::Query q(pool->db, "SELECT uuid FROM all_items_view WHERE uuid = ? AND type = ?");
    q.bind(1, uu);
    q.bind(2, object_type_lut.lookup_reverse(type));
    return q.step();
}

void PoolUpdater::set_pool_info(const std::string &bp)
{
    base_path = bp;
    const auto pools = PoolManager::get().get_pools();
    if (pools.count(bp)) {
        auto pool_info = PoolManager::get().get_pools().at(bp);
        pool_uuid = pool_info.uuid;
    }
}

void PoolUpdater::update(const std::vector<std::string> &base_paths)
{
    status_cb(PoolUpdateStatus::INFO, "", "tags");
    pool->db.execute("DELETE FROM tags");
    pool->db.execute("DELETE FROM dependencies");
    pool->db.execute("BEGIN TRANSACTION");
    pool->db.execute("DELETE FROM units");
    for (const auto &bp : base_paths) {
        set_pool_info(bp);
        update_units(Glib::build_filename(bp, "units"));
    }
    pool->db.execute("COMMIT");

    status_cb(PoolUpdateStatus::INFO, "", "entities");
    pool->db.execute("BEGIN TRANSACTION");
    pool->db.execute("DELETE FROM entities");
    for (const auto &bp : base_paths) {
        set_pool_info(bp);
        update_entities(Glib::build_filename(bp, "entities"));
    }
    pool->db.execute("COMMIT");

    status_cb(PoolUpdateStatus::INFO, "", "symbols");
    pool->db.execute("BEGIN TRANSACTION");
    pool->db.execute("DELETE FROM symbols");
    for (const auto &bp : base_paths) {
        set_pool_info(bp);
        update_symbols(Glib::build_filename(bp, "symbols"));
    }
    pool->db.execute("COMMIT");

    status_cb(PoolUpdateStatus::INFO, "", "padstacks");
    pool->db.execute("BEGIN TRANSACTION");
    pool->db.execute("DELETE FROM padstacks");
    for (const auto &bp : base_paths) {
        set_pool_info(bp);
        update_padstacks_global(Glib::build_filename(bp, "padstacks"));
        update_padstacks(Glib::build_filename(bp, "packages"));
    }
    pool->db.execute("COMMIT");

    status_cb(PoolUpdateStatus::INFO, "", "packages");
    pool->db.execute("BEGIN TRANSACTION");
    pool->db.execute("DELETE FROM packages");
    pool->db.execute("DELETE FROM models");
    for (const auto &bp : base_paths) {
        set_pool_info(bp);
        update_packages(Glib::build_filename(bp, "packages"));
    }
    pool->db.execute("COMMIT");

    status_cb(PoolUpdateStatus::INFO, "", "parts");
    pool->db.execute("BEGIN TRANSACTION");
    pool->db.execute("DELETE FROM parts");
    for (const auto &bp : base_paths) {
        set_pool_info(bp);
        update_parts(Glib::build_filename(bp, "parts"));
    }
    pool->db.execute("COMMIT");

    status_cb(PoolUpdateStatus::INFO, "", "frames");
    pool->db.execute("BEGIN TRANSACTION");
    pool->db.execute("DELETE FROM frames");
    for (const auto &bp : base_paths) {
        if (Glib::file_test(Glib::build_filename(bp, "frames"), Glib::FILE_TEST_IS_DIR)) {
            set_pool_info(bp);
            update_frames(Glib::build_filename(bp, "frames"));
        }
    }
    pool->db.execute("COMMIT");
}

PoolUpdater::PoolUpdater(const std::string &bp, pool_update_cb_t cb) : status_cb(cb)
{
    auto pool_db_path = Glib::build_filename(bp, "pool.db");
    status_cb(PoolUpdateStatus::INFO, "", "start");
    {
        SQLite::Database db(pool_db_path, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE);

        int user_version = db.get_user_version();
        if (user_version != Pool::get_required_schema_version()) {
            // update schema
            auto bytes = Gio::Resource::lookup_data_global("/net/carrotIndustries/horizon/pool-update/schema.sql");
            gsize size{bytes->get_size() + 1}; // null byte
            auto data = (const char *)bytes->get_data(size);
            db.execute(data);
            status_cb(PoolUpdateStatus::INFO, "", "created db from schema");
        }
    }
    pool = std::make_unique<Pool>(bp, false);
}

void PoolUpdater::update_frames(const std::string &directory, const std::string &prefix)
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        std::string filename = Glib::build_filename(directory, it);
        if (endswith(it, ".json")) {
            try {
                status_cb(PoolUpdateStatus::FILE, filename, "");
                auto frame = Frame::new_from_file(filename);
                bool overridden = false;
                if (exists(ObjectType::UNIT, frame.uuid)) {
                    overridden = true;
                    SQLite::Query q(pool->db, "DELETE FROM frames WHERE uuid = ?");
                    q.bind(1, frame.uuid);
                    q.step();
                }

                SQLite::Query q(pool->db,
                                "INSERT INTO frames "
                                "(uuid, name, filename, pool_uuid, overridden) "
                                "VALUES "
                                "($uuid, $name, $filename, $pool_uuid, $overridden)");
                q.bind("$uuid", frame.uuid);
                q.bind("$name", frame.name);
                q.bind("$filename", Glib::build_filename("frames", prefix, it));
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
        else if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
            update_units(filename, Glib::build_filename(prefix, it));
        }
    }
}
void PoolUpdater::update_units(const std::string &directory, const std::string &prefix)
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        std::string filename = Glib::build_filename(directory, it);
        if (endswith(it, ".json")) {
            try {
                status_cb(PoolUpdateStatus::FILE, filename, "");
                auto unit = Unit::new_from_file(filename);
                bool overridden = false;
                if (exists(ObjectType::UNIT, unit.uuid)) {
                    overridden = true;
                    SQLite::Query q(pool->db, "DELETE FROM units WHERE uuid = ?");
                    q.bind(1, unit.uuid);
                    q.step();
                }

                SQLite::Query q(pool->db,
                                "INSERT INTO units "
                                "(uuid, name, manufacturer, filename, pool_uuid, overridden) "
                                "VALUES "
                                "($uuid, $name, $manufacturer, $filename, $pool_uuid, $overridden)");
                q.bind("$uuid", unit.uuid);
                q.bind("$name", unit.name);
                q.bind("$manufacturer", unit.manufacturer);
                q.bind("$filename", Glib::build_filename("units", prefix, it));
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
        else if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
            update_units(filename, Glib::build_filename(prefix, it));
        }
    }
}

void PoolUpdater::update_entities(const std::string &directory, const std::string &prefix)
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        std::string filename = Glib::build_filename(directory, it);
        if (endswith(it, ".json")) {
            try {
                status_cb(PoolUpdateStatus::FILE, filename, "");
                auto entity = Entity::new_from_file(filename, *pool);
                bool overridden = false;
                if (exists(ObjectType::ENTITY, entity.uuid)) {
                    overridden = true;
                    {
                        SQLite::Query q(pool->db, "DELETE FROM entities WHERE uuid = ?");
                        q.bind(1, entity.uuid);
                        q.step();
                    }
                    {
                        SQLite::Query q(pool->db, "DELETE FROM tags WHERE uuid = ? AND type = 'entity'");
                        q.bind(1, entity.uuid);
                        q.step();
                    }
                }
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
                q.bind("$filename", Glib::build_filename("entities", prefix, it));
                q.step();
                for (const auto &it_tag : entity.tags) {
                    SQLite::Query q2(pool->db,
                                     "INSERT into tags (tag, uuid, type) "
                                     "VALUES ($tag, $uuid, 'entity')");
                    q2.bind("$uuid", entity.uuid);
                    q2.bind("$tag", it_tag);
                    q2.step();
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
        else if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
            update_entities(filename, Glib::build_filename(prefix, it));
        }
    }
}

void PoolUpdater::update_symbols(const std::string &directory, const std::string &prefix)
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        std::string filename = Glib::build_filename(directory, it);
        if (endswith(it, ".json")) {
            try {
                status_cb(PoolUpdateStatus::FILE, filename, "");
                auto symbol = Symbol::new_from_file(filename, *pool);
                bool overridden = false;
                if (exists(ObjectType::SYMBOL, symbol.uuid)) {
                    overridden = true;
                    SQLite::Query q(pool->db, "DELETE FROM symbols WHERE uuid = ?");
                    q.bind(1, symbol.uuid);
                    q.step();
                }
                SQLite::Query q(pool->db,
                                "INSERT INTO symbols "
                                "(uuid, name, filename, unit, pool_uuid, overridden) "
                                "VALUES "
                                "($uuid, $name, $filename, $unit, $pool_uuid, $overridden)");
                q.bind("$uuid", symbol.uuid);
                q.bind("$name", symbol.name);
                q.bind("$unit", symbol.unit->uuid);
                q.bind("$pool_uuid", pool_uuid);
                q.bind("$overridden", overridden);
                q.bind("$filename", Glib::build_filename("symbols", prefix, it));
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
        else if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
            update_symbols(filename, Glib::build_filename(prefix, it));
        }
    }
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
                                bool overridden = false;
                                if (exists(ObjectType::PADSTACK, padstack.uuid)) {
                                    overridden = true;
                                    SQLite::Query q(pool->db, "DELETE FROM padstacks WHERE uuid = ?");
                                    q.bind(1, padstack.uuid);
                                    q.step();
                                }
                                SQLite::Query q(pool->db,
                                                "INSERT INTO padstacks "
                                                "(uuid, name, filename, package, type, pool_uuid, overridden) "
                                                "VALUES "
                                                "($uuid, $name, $filename, $package, $type, $pool_uuid, $overridden)");
                                q.bind("$uuid", padstack.uuid);
                                q.bind("$name", padstack.name);
                                q.bind("$type", Padstack::type_lut.lookup_reverse(padstack.type));
                                q.bind("$package", pkg_uuid);
                                q.bind("$pool_uuid", pool_uuid);
                                q.bind("$overridden", overridden);
                                q.bind("$filename", Glib::build_filename("packages", prefix, it, "padstacks", it2));
                                q.step();
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
                auto padstack = Padstack::new_from_file(filename);
                bool overridden = false;
                if (exists(ObjectType::PADSTACK, padstack.uuid)) {
                    overridden = true;
                    SQLite::Query q(pool->db, "DELETE FROM padstacks WHERE uuid = ?");
                    q.bind(1, padstack.uuid);
                    q.step();
                }
                SQLite::Query q(pool->db,
                                "INSERT INTO padstacks "
                                "(uuid, name, filename, package, type, pool_uuid, overridden) "
                                "VALUES "
                                "($uuid, $name, $filename, $package, $type, $pool_uuid, $overridden)");
                q.bind("$uuid", padstack.uuid);
                q.bind("$name", padstack.name);
                q.bind("$type", Padstack::type_lut.lookup_reverse(padstack.type));
                q.bind("$package", UUID());
                q.bind("$pool_uuid", pool_uuid);
                q.bind("$overridden", overridden);
                q.bind("$filename", Glib::build_filename("padstacks", prefix, it));
                q.step();
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

void PoolUpdater::update_packages(const std::string &directory)
{
    PoolUpdateGraph graph;
    pkg_add_dir_to_graph(graph, directory, status_cb);
    auto missing = graph.update_dependants();
    for (const auto &it : missing) {
        status_cb(PoolUpdateStatus::FILE_ERROR, it.first->filename, "missing dependency " + (std::string)it.second);
    }

    std::set<UUID> visited;
    auto root = graph.get_root();
    update_package_node(root, visited);
}

void PoolUpdater::update_package_node(const PoolUpdateNode &node, std::set<UUID> &visited)
{
    if (visited.count(node.uuid)) {
        status_cb(PoolUpdateStatus::FILE_ERROR, node.filename, "detected cycle");
        return;
    }
    visited.insert(node.uuid);

    auto filename = node.filename;
    try {
        if (filename.size()) {
            status_cb(PoolUpdateStatus::FILE, filename, "");
            auto package = Package::new_from_file(filename, *pool);
            bool overridden = false;
            if (exists(ObjectType::PACKAGE, package.uuid)) {
                overridden = true;
                {
                    SQLite::Query q(pool->db, "DELETE FROM packages WHERE uuid = ?");
                    q.bind(1, package.uuid);
                    q.step();
                }
                {
                    SQLite::Query q(pool->db, "DELETE FROM tags WHERE uuid = ? AND type = 'package'");
                    q.bind(1, package.uuid);
                    q.step();
                }
                {
                    SQLite::Query q(pool->db, "DELETE FROM models WHERE package_uuid = ?");
                    q.bind(1, package.uuid);
                    q.step();
                }
            }
            SQLite::Query q(pool->db,
                            "INSERT INTO packages "
                            "(uuid, name, manufacturer, filename, n_pads, alternate_for, pool_uuid, overridden) "
                            "VALUES "
                            "($uuid, $name, $manufacturer, $filename, $n_pads, $alt_for, $pool_uuid, $overridden)");
            q.bind("$uuid", package.uuid);
            q.bind("$name", package.name);
            q.bind("$manufacturer", package.manufacturer);
            q.bind("$n_pads", std::count_if(package.pads.begin(), package.pads.end(), [](const auto &x) {
                       return x.second.padstack.type != Padstack::Type::MECHANICAL;
                   }));
            q.bind("$alt_for", package.alternate_for ? package.alternate_for->uuid : UUID());

            auto bp = Gio::File::create_for_path(base_path);
            auto rel = bp->get_relative_path(Gio::File::create_for_path(filename));
            q.bind("$filename", rel);
            q.bind("$pool_uuid", pool_uuid);
            q.bind("$overridden", overridden);
            q.step();
            for (const auto &it_tag : package.tags) {
                SQLite::Query q2(pool->db,
                                 "INSERT into tags (tag, uuid, type) VALUES "
                                 "($tag, $uuid, 'package')");
                q2.bind("$uuid", package.uuid);
                q2.bind("$tag", it_tag);
                q2.step();
            }
            for (const auto &it_model : package.models) {
                SQLite::Query q2(pool->db,
                                 "INSERT INTO models (package_uuid, "
                                 "model_uuid, model_filename) VALUES (?, ?, "
                                 "?)");
                q2.bind(1, package.uuid);
                q2.bind(2, it_model.first);
                q2.bind(3, it_model.second.filename);
                q2.step();
            }
            for (const auto &it_pad : package.pads) {
                add_dependency(ObjectType::PACKAGE, package.uuid, ObjectType::PADSTACK,
                               it_pad.second.pool_padstack->uuid);
            }
            if (package.alternate_for) {
                add_dependency(ObjectType::PACKAGE, package.uuid, ObjectType::PACKAGE, package.alternate_for->uuid);
            }
        }

        for (const auto dependant : node.dependants) {
            update_package_node(*dependant, visited);
        }
    }
    catch (const std::exception &e) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, e.what());
    }
    catch (...) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, "unknown exception");
    }
}


void PoolUpdater::update_part_node(const PoolUpdateNode &node, std::set<UUID> &visited)
{
    if (visited.count(node.uuid)) {
        status_cb(PoolUpdateStatus::FILE_ERROR, node.filename, "detected cycle");
        return;
    }
    visited.insert(node.uuid);

    auto filename = node.filename;
    try {
        if (filename.size()) {
            status_cb(PoolUpdateStatus::FILE, filename, "");
            auto part = Part::new_from_file(filename, *pool);
            bool overridden = false;
            if (exists(ObjectType::PART, part.uuid)) {
                overridden = true;
                {
                    SQLite::Query q(pool->db, "DELETE FROM parts WHERE uuid = ?");
                    q.bind(1, part.uuid);
                    q.step();
                }
                {
                    SQLite::Query q(pool->db, "DELETE FROM tags WHERE uuid = ? AND type = 'part'");
                    q.bind(1, part.uuid);
                    q.step();
                }
            }
            std::string table;
            if (part.parametric.count("table"))
                table = part.parametric.at("table");
            SQLite::Query q(pool->db,
                            "INSERT INTO parts "
                            "(uuid, MPN, manufacturer, entity, package, description, filename, pool_uuid, overridden, "
                            "parametric_table) "
                            "VALUES "
                            "($uuid, $MPN, $manufacturer, $entity, $package, $description, $filename, $pool_uuid, "
                            "$overridden, $parametric_table)");
            q.bind("$uuid", part.uuid);
            q.bind("$MPN", part.get_MPN());
            q.bind("$manufacturer", part.get_manufacturer());
            q.bind("$package", part.package->uuid);
            q.bind("$entity", part.entity->uuid);
            q.bind("$description", part.get_description());
            q.bind("$pool_uuid", pool_uuid);
            q.bind("$overridden", overridden);
            q.bind("$parametric_table", table);
            auto bp = Gio::File::create_for_path(base_path);
            auto rel = bp->get_relative_path(Gio::File::create_for_path(filename));
            q.bind("$filename", rel);
            q.step();

            for (const auto &it_tag : part.get_tags()) {
                SQLite::Query q2(pool->db,
                                 "INSERT into tags (tag, uuid, type) VALUES "
                                 "($tag, $uuid, 'part')");
                q2.bind("$uuid", part.uuid);
                q2.bind("$tag", it_tag);
                q2.step();
            }
            if (part.base) {
                add_dependency(ObjectType::PART, part.uuid, ObjectType::PART, part.base->uuid);
            }
            else {
                add_dependency(ObjectType::PART, part.uuid, ObjectType::ENTITY, part.entity->uuid);
                add_dependency(ObjectType::PART, part.uuid, ObjectType::PACKAGE, part.package->uuid);
            }
        }

        for (const auto dependant : node.dependants) {
            update_part_node(*dependant, visited);
        }
    }
    catch (const std::exception &e) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, e.what());
    }
    catch (...) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, "unknown exception");
    }
}

void PoolUpdater::update_parts(const std::string &directory)
{
    PoolUpdateGraph graph;
    part_add_dir_to_graph(graph, directory, status_cb);
    auto missing = graph.update_dependants();
    for (const auto &it : missing) {
        status_cb(PoolUpdateStatus::FILE_ERROR, it.first->filename, "missing dependency " + (std::string)it.second);
    }

    std::set<UUID> visited;
    auto root = graph.get_root();
    update_part_node(root, visited);
}

void PoolUpdater::add_dependency(ObjectType type, const UUID &uu, ObjectType dep_type, const UUID &dep_uuid)
{
    SQLite::Query q(pool->db, "INSERT INTO dependencies VALUES (?, ?, ?, ?)");
    q.bind(1, object_type_lut.lookup_reverse(type));
    q.bind(2, uu);
    q.bind(3, object_type_lut.lookup_reverse(dep_type));
    q.bind(4, dep_uuid);
    q.step();
}

void pool_update(const std::string &pool_base_path, pool_update_cb_t status_cb, bool parametric)
{
    if (!status_cb)
        status_cb = &status_cb_nop;
    PoolUpdater updater(pool_base_path, status_cb);
    const auto pools = PoolManager::get().get_pools();
    std::vector<std::string> paths;
    if (pools.count(pool_base_path)) {
        const auto &pool_info = pools.at(pool_base_path);
        for (const auto &it : pool_info.pools_included) {
            auto inc = PoolManager::get().get_by_uuid(it);
            if (inc) {
                paths.push_back(inc->base_path);
            }
            else {
                status_cb(PoolUpdateStatus::FILE_ERROR, pool_base_path, "pool " + (std::string)it + " not found");
            }
        }
    }
    paths.push_back(pool_base_path);
    updater.update(paths);

    if (parametric) {
        pool_update_parametric(pool_base_path, status_cb);
    }
    status_cb(PoolUpdateStatus::DONE, "done", "");
}

} // namespace horizon
