#include "pool_updater.hpp"
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include <giomm/file.h>
#include "util/util.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
static void pkg_add_dir_to_graph(PoolUpdateGraph &graph, const std::string &directory, pool_update_cb_t status_cb)
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        auto pkgpath = Glib::build_filename(directory, it);
        auto pkgfilename = Glib::build_filename(pkgpath, "package.json");
        if (Glib::file_test(pkgfilename, Glib::FileTest::FILE_TEST_IS_REGULAR)) {
            std::string filename = Glib::build_filename(pkgpath, "package.json");
            try {
                json j = load_json_from_file(filename);

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

void PoolUpdater::update_packages(const std::string &directory)
{
    PoolUpdateGraph graph;
    pkg_add_dir_to_graph(graph, directory, status_cb);
    {
        auto missing = graph.update_dependants();
        auto uuids_missing = uuids_from_missing(missing);
        for (const auto &it : uuids_missing) { // try to resolve missing from items already added
            if (exists(ObjectType::PACKAGE, it))
                graph.add_node(it, "", {});
        }
    }

    auto missing = graph.update_dependants();
    for (const auto &it : missing) {
        status_cb(PoolUpdateStatus::FILE_ERROR, it.first->filename, "missing dependency " + (std::string)it.second);
    }

    std::set<UUID> visited;
    auto root = graph.get_root();
    update_package_node(root, visited);
    for (const auto node : graph.get_not_visited(visited)) {
        status_cb(PoolUpdateStatus::FILE_ERROR, node->filename,
                  "package not visited (might be due to circular dependency)");
    }
}

void PoolUpdater::update_package_node(const PoolUpdateNode &node, std::set<UUID> &visited)
{
    if (visited.count(node.uuid)) {
        status_cb(PoolUpdateStatus::FILE_ERROR, node.filename, "detected cycle");
        return;
    }
    visited.insert(node.uuid);

    auto filename = node.filename;
    if (filename.size())
        update_package(filename, false);

    for (const auto dependant : node.dependants) {
        update_package_node(*dependant, visited);
    }
}

void PoolUpdater::update_package(const std::string &filename, bool partial)
{
    try {
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
            clear_tags(ObjectType::PACKAGE, package.uuid);
            clear_dependencies(ObjectType::PACKAGE, package.uuid);
            {
                SQLite::Query q(pool->db, "DELETE FROM models WHERE package_uuid = ?");
                q.bind(1, package.uuid);
                q.step();
            }
        }
        if (partial)
            overridden = false;
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
            add_tag(ObjectType::PACKAGE, package.uuid, it_tag);
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
            add_dependency(ObjectType::PACKAGE, package.uuid, ObjectType::PADSTACK, it_pad.second.pool_padstack->uuid);
        }
        if (package.alternate_for) {
            add_dependency(ObjectType::PACKAGE, package.uuid, ObjectType::PACKAGE, package.alternate_for->uuid);
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
