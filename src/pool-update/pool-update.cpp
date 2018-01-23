#include "pool-update.hpp"
#include "graph.hpp"
#include "pool/package.hpp"
#include "pool/part.hpp"
#include "pool/pool.hpp"
#include "pool/unit.hpp"
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

namespace horizon {

using json = nlohmann::json;

static void update_units(SQLite::Database &db, const std::string &directory, pool_update_cb_t status_cb,
                         const std::string &prefix = "")
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        std::string filename = Glib::build_filename(directory, it);
        if (endswith(it, ".json")) {
            try {
                status_cb(PoolUpdateStatus::FILE, filename, "");
                auto unit = Unit::new_from_file(filename);
                SQLite::Query q(db,
                                "INSERT INTO units (uuid, name, manufacturer, "
                                "filename) VALUES ($uuid, $name, "
                                "$manufacturer, $filename)");
                q.bind("$uuid", unit.uuid);
                q.bind("$name", unit.name);
                q.bind("$manufacturer", unit.manufacturer);
                q.bind("$filename", Glib::build_filename(prefix, it));
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
            update_units(db, filename, status_cb, Glib::build_filename(prefix, it));
        }
    }
}

static void update_entities(Pool &pool, const std::string &directory, pool_update_cb_t status_cb,
                            const std::string &prefix = "")
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        std::string filename = Glib::build_filename(directory, it);
        if (endswith(it, ".json")) {
            try {
                status_cb(PoolUpdateStatus::FILE, filename, "");
                auto entity = Entity::new_from_file(filename, pool);
                SQLite::Query q(pool.db,
                                "INSERT INTO entities (uuid, name, "
                                "manufacturer, filename, n_gates, prefix) "
                                "VALUES ($uuid, $name, $manufacturer, "
                                "$filename, $n_gates, $prefix)");
                q.bind("$uuid", entity.uuid);
                q.bind("$name", entity.name);
                q.bind("$manufacturer", entity.manufacturer);
                q.bind("$n_gates", entity.gates.size());
                q.bind("$prefix", entity.prefix);
                q.bind("$filename", Glib::build_filename(prefix, it));
                q.step();
                for (const auto &it_tag : entity.tags) {
                    SQLite::Query q2(pool.db,
                                     "INSERT into tags (tag, uuid, type) "
                                     "VALUES ($tag, $uuid, 'entity')");
                    q2.bind("$uuid", entity.uuid);
                    q2.bind("$tag", it_tag);
                    q2.step();
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
            update_entities(pool, filename, status_cb, Glib::build_filename(prefix, it));
        }
    }
}

static void update_symbols(Pool &pool, const std::string &directory, pool_update_cb_t status_cb,
                           const std::string &prefix = "")
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        std::string filename = Glib::build_filename(directory, it);
        if (endswith(it, ".json")) {
            try {
                status_cb(PoolUpdateStatus::FILE, filename, "");
                auto symbol = Symbol::new_from_file(filename, pool);
                SQLite::Query q(pool.db,
                                "INSERT INTO symbols (uuid, name, filename, "
                                "unit) VALUES ($uuid, $name, $filename, "
                                "$unit)");
                q.bind("$uuid", symbol.uuid);
                q.bind("$name", symbol.name);
                q.bind("$unit", symbol.unit->uuid);
                q.bind("$filename", Glib::build_filename(prefix, it));
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
            update_symbols(pool, filename, status_cb, Glib::build_filename(prefix, it));
        }
    }
}

static void update_padstacks(SQLite::Database &db, const std::string &directory, pool_update_cb_t status_cb,
                             const std::string &prefix = "")
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
                std::ifstream ifs(pkg_filename);
                json j;
                if (!ifs.is_open()) {
                    throw std::runtime_error("package not opened");
                }
                ifs >> j;
                ifs.close();
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
                                SQLite::Query q(db,
                                                "INSERT INTO padstacks (uuid, "
                                                "name, filename, package, "
                                                "type) VALUES ($uuid, $name, "
                                                "$filename, $package, $type)");
                                q.bind("$uuid", padstack.uuid);
                                q.bind("$name", padstack.name);
                                q.bind("$type", Padstack::type_lut.lookup_reverse(padstack.type));
                                q.bind("$package", pkg_uuid);
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
            update_padstacks(db, pkgpath, status_cb, Glib::build_filename(prefix, it));
        }
    }
}

static void update_padstacks_global(SQLite::Database &db, const std::string &directory, pool_update_cb_t status_cb,
                                    const std::string &prefix = "")
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        std::string filename = Glib::build_filename(directory, it);
        if (endswith(it, ".json")) {
            try {
                status_cb(PoolUpdateStatus::FILE, filename, "");
                auto padstack = Padstack::new_from_file(filename);
                SQLite::Query q(db,
                                "INSERT INTO padstacks (uuid, name, filename, "
                                "package, type) VALUES ($uuid, $name, "
                                "$filename, $package, $type)");
                q.bind("$uuid", padstack.uuid);
                q.bind("$name", padstack.name);
                q.bind("$type", Padstack::type_lut.lookup_reverse(padstack.type));
                q.bind("$package", UUID());
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
            update_padstacks(db, filename, status_cb, Glib::build_filename(prefix, it));
        }
    }
}

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

static void update_package_node(Pool &pool, const PoolUpdateNode &node, std::set<UUID> &visited,
                                pool_update_cb_t status_cb)
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
            auto package = Package::new_from_file(filename, pool);
            SQLite::Query q(pool.db,
                            "INSERT INTO packages (uuid, name, manufacturer, "
                            "filename, n_pads, alternate_for) VALUES ($uuid, "
                            "$name, $manufacturer, $filename, $n_pads, "
                            "$alt_for)");
            q.bind("$uuid", package.uuid);
            q.bind("$name", package.name);
            q.bind("$manufacturer", package.manufacturer);
            q.bind("$n_pads", std::count_if(package.pads.begin(), package.pads.end(), [](const auto &x) {
                       return x.second.padstack.type != Padstack::Type::MECHANICAL;
                   }));
            q.bind("$alt_for", package.alternate_for ? package.alternate_for->uuid : UUID());

            auto base_path = Gio::File::create_for_path(Glib::build_filename(pool.get_base_path(), "packages"));
            auto rel = base_path->get_relative_path(Gio::File::create_for_path(filename));
            q.bind("$filename", rel);
            q.step();
            for (const auto &it_tag : package.tags) {
                SQLite::Query q2(pool.db,
                                 "INSERT into tags (tag, uuid, type) VALUES "
                                 "($tag, $uuid, 'package')");
                q2.bind("$uuid", package.uuid);
                q2.bind("$tag", it_tag);
                q2.step();
            }
            for (const auto &it_model : package.models) {
                SQLite::Query q2(pool.db,
                                 "INSERT INTO models (package_uuid, "
                                 "model_uuid, model_filename) VALUES (?, ?, "
                                 "?)");
                q2.bind(1, package.uuid);
                q2.bind(2, it_model.first);
                q2.bind(3, it_model.second.filename);
                q2.step();
            }
        }

        for (const auto dependant : node.dependants) {
            update_package_node(pool, *dependant, visited, status_cb);
        }
    }
    catch (const std::exception &e) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, e.what());
    }
    catch (...) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, "unknown exception");
    }
}

static void update_packages(Pool &pool, const std::string &directory, pool_update_cb_t status_cb)
{
    PoolUpdateGraph graph;
    pkg_add_dir_to_graph(graph, directory, status_cb);
    auto missing = graph.update_dependants();
    for (const auto &it : missing) {
        status_cb(PoolUpdateStatus::FILE_ERROR, it.first->filename, "missing dependency " + (std::string)it.second);
    }

    std::set<UUID> visited;
    auto root = graph.get_root();
    update_package_node(pool, root, visited, status_cb);
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

static void update_part_node(Pool &pool, const PoolUpdateNode &node, std::set<UUID> &visited,
                             pool_update_cb_t status_cb)
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
            auto part = Part::new_from_file(filename, pool);
            SQLite::Query q(pool.db,
                            "INSERT INTO parts (uuid, MPN, manufacturer, "
                            "entity, package, description, filename) VALUES "
                            "($uuid, $MPN, $manufacturer, $entity, $package, "
                            "$description, $filename)");
            q.bind("$uuid", part.uuid);
            q.bind("$MPN", part.get_MPN());
            q.bind("$manufacturer", part.get_manufacturer());
            q.bind("$package", part.package->uuid);
            q.bind("$entity", part.entity->uuid);
            q.bind("$description", part.get_description());
            auto base_path = Gio::File::create_for_path(Glib::build_filename(pool.get_base_path(), "parts"));
            auto rel = base_path->get_relative_path(Gio::File::create_for_path(filename));
            q.bind("$filename", rel);
            q.step();

            for (const auto &it_tag : part.get_tags()) {
                SQLite::Query q2(pool.db,
                                 "INSERT into tags (tag, uuid, type) VALUES "
                                 "($tag, $uuid, 'part')");
                q2.bind("$uuid", part.uuid);
                q2.bind("$tag", it_tag);
                q2.step();
            }
        }

        for (const auto dependant : node.dependants) {
            update_part_node(pool, *dependant, visited, status_cb);
        }
    }
    catch (const std::exception &e) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, e.what());
    }
    catch (...) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, "unknown exception");
    }
}

static void update_parts(Pool &pool, const std::string &directory, pool_update_cb_t status_cb)
{
    PoolUpdateGraph graph;
    part_add_dir_to_graph(graph, directory, status_cb);
    auto missing = graph.update_dependants();
    for (const auto &it : missing) {
        status_cb(PoolUpdateStatus::FILE_ERROR, it.first->filename, "missing dependency " + (std::string)it.second);
    }

    std::set<UUID> visited;
    auto root = graph.get_root();
    update_part_node(pool, root, visited, status_cb);
}

void status_cb_nop(PoolUpdateStatus st, const std::string msg, const std::string filename)
{
}

void pool_update(const std::string &pool_base_path, pool_update_cb_t status_cb)
{
    auto pool_db_path = Glib::build_filename(pool_base_path, "pool.db");
    if (!status_cb)
        status_cb = &status_cb_nop;

    status_cb(PoolUpdateStatus::INFO, "", "start");

    {
        SQLite::Database db(pool_db_path, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE);
        {
            int user_version = db.get_user_version();
            if (user_version < Pool::get_required_schema_version()) {
                // update schema
                auto bytes = Gio::Resource::lookup_data_global("/net/carrotIndustries/horizon/pool-update/schema.sql");
                gsize size{bytes->get_size() + 1}; // null byte
                auto data = (const char *)bytes->get_data(size);
                db.execute(data);
                status_cb(PoolUpdateStatus::INFO, "", "created db from schema");
            }
        }
    }

    Pool pool(pool_base_path, false);

    status_cb(PoolUpdateStatus::INFO, "", "tags");
    pool.db.execute("DELETE FROM tags");
    pool.db.execute("BEGIN TRANSACTION");
    pool.db.execute("DELETE FROM units");
    update_units(pool.db, Glib::build_filename(pool_base_path, "units"), status_cb);
    pool.db.execute("COMMIT");

    status_cb(PoolUpdateStatus::INFO, "", "entities");
    pool.db.execute("BEGIN TRANSACTION");
    pool.db.execute("DELETE FROM entities");
    update_entities(pool, Glib::build_filename(pool_base_path, "entities"), status_cb);
    pool.db.execute("COMMIT");

    status_cb(PoolUpdateStatus::INFO, "", "symbols");
    pool.db.execute("BEGIN TRANSACTION");
    pool.db.execute("DELETE FROM symbols");
    update_symbols(pool, Glib::build_filename(pool_base_path, "symbols"), status_cb);
    pool.db.execute("COMMIT");

    status_cb(PoolUpdateStatus::INFO, "", "padstacks");
    pool.db.execute("BEGIN TRANSACTION");
    pool.db.execute("DELETE FROM padstacks");
    update_padstacks_global(pool.db, Glib::build_filename(pool_base_path, "padstacks"), status_cb);
    update_padstacks(pool.db, Glib::build_filename(pool_base_path, "packages"), status_cb);
    pool.db.execute("COMMIT");

    status_cb(PoolUpdateStatus::INFO, "", "packages");
    pool.db.execute("BEGIN TRANSACTION");
    pool.db.execute("DELETE FROM packages");
    pool.db.execute("DELETE FROM models");
    update_packages(pool, Glib::build_filename(pool_base_path, "packages"), status_cb);
    pool.db.execute("COMMIT");

    status_cb(PoolUpdateStatus::INFO, "", "parts");
    pool.db.execute("BEGIN TRANSACTION");
    pool.db.execute("DELETE FROM parts");
    update_parts(pool, Glib::build_filename(pool_base_path, "parts"), status_cb);
    pool.db.execute("COMMIT");
    status_cb(PoolUpdateStatus::DONE, "done", "");
}
} // namespace horizon
